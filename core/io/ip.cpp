
/*  ip.cpp                                                               */


#include "ip.h"

#include "core/containers/hash_map.h"
#include "core/os/semaphore.h"
#include "core/os/thread.h"

VARIANT_ENUM_CAST(IP::ResolverStatus);

/************* RESOLVER ******************/

struct _IP_ResolverPrivate {
	struct QueueItem {
		SafeNumeric<IP::ResolverStatus> status;
		List<IP_Address> response;
		String hostname;
		IP::Type type;

		void clear() {
			status.set(IP::RESOLVER_STATUS_NONE);
			response.clear();
			type = IP::TYPE_NONE;
			hostname = "";
		};

		QueueItem() {
			clear();
		};
	};

	QueueItem queue[IP::RESOLVER_MAX_QUERIES];

	IP::ResolverID find_empty_id() const {
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() == IP::RESOLVER_STATUS_NONE) {
				return i;
			}
		}
		return IP::RESOLVER_INVALID_ID;
	}

	Mutex mutex;
	Semaphore sem;

	Thread thread;
	//Semaphore* semaphore;
	bool thread_abort;

	void resolve_queues() {
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}

			mutex.lock();
			List<IP_Address> response;
			String hostname = queue[i].hostname;
			IP::Type type = queue[i].type;
			mutex.unlock();

			// We should not lock while resolving the hostname,
			// only when modifying the queue.
			IP::get_singleton()->_resolve_hostname(response, hostname, type);

			MutexLock lock(mutex);
			// Could have been completed by another function, or deleted.
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}
			// We might be overriding another result, but we don't care as long as the result is valid.
			if (response.size()) {
				String key = get_cache_key(hostname, type);
				cache[key] = response;
			}
			queue[i].response = response;
			queue[i].status.set(response.empty() ? IP::RESOLVER_STATUS_ERROR : IP::RESOLVER_STATUS_DONE);
		}
	}

	static void _thread_function(void *self) {
		_IP_ResolverPrivate *ipr = (_IP_ResolverPrivate *)self;

		while (!ipr->thread_abort) {
			ipr->sem.wait();
			ipr->resolve_queues();
		}
	}

	HashMap<String, List<IP_Address>> cache;

	static String get_cache_key(String p_hostname, IP::Type p_type) {
		return itos(p_type) + p_hostname;
	}
};

IP_Address IP::resolve_hostname(const String &p_hostname, IP::Type p_type) {
	const Array addresses = resolve_hostname_addresses(p_hostname, p_type);
	return addresses.size() ? addresses[0].operator IP_Address() : IP_Address();
}

Array IP::resolve_hostname_addresses(const String &p_hostname, Type p_type) {
	List<IP_Address> res;
	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);

	resolver->mutex.lock();
	if (resolver->cache.has(key)) {
		res = resolver->cache[key];
	} else {
		// This should be run unlocked so the resolver thread can keep resolving
		// other requests.
		resolver->mutex.unlock();
		_resolve_hostname(res, p_hostname, p_type);
		resolver->mutex.lock();
		// We might be overriding another result, but we don't care as long as the result is valid.
		if (res.size()) {
			resolver->cache[key] = res;
		}
	}
	resolver->mutex.unlock();

	Array result;
	for (int i = 0; i < res.size(); ++i) {
		result.push_back(String(res[i]));
	}
	return result;
}

IP::ResolverID IP::resolve_hostname_queue_item(const String &p_hostname, IP::Type p_type) {
	MutexLock lock(resolver->mutex);

	ResolverID id = resolver->find_empty_id();

	if (id == RESOLVER_INVALID_ID) {
		WARN_PRINT("Out of resolver queries");
		return id;
	}

	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);
	resolver->queue[id].hostname = p_hostname;
	resolver->queue[id].type = p_type;
	if (resolver->cache.has(key)) {
		resolver->queue[id].response = resolver->cache[key];
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_DONE);
	} else {
		resolver->queue[id].response = List<IP_Address>();
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_WAITING);
		if (resolver->thread.is_started()) {
			resolver->sem.post();
		} else {
			resolver->resolve_queues();
		}
	}

	return id;
}

IP::ResolverStatus IP::get_resolve_item_status(ResolverID p_id) const {
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IP::RESOLVER_STATUS_NONE, vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try performing less network requests at once.", p_id, IP::RESOLVER_MAX_QUERIES));

	IP::ResolverStatus res = resolver->queue[p_id].status.get();
	if (res == IP::RESOLVER_STATUS_NONE) {
		ERR_PRINT("Condition status == IP::RESOLVER_STATUS_NONE");
		return IP::RESOLVER_STATUS_NONE;
	}
	return res;
}

IP_Address IP::get_resolve_item_address(ResolverID p_id) const {
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IP_Address(), vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try performing less network requests at once.", p_id, IP::RESOLVER_MAX_QUERIES));

	MutexLock lock(resolver->mutex);

	if (resolver->queue[p_id].status.get() != IP::RESOLVER_STATUS_DONE) {
		ERR_PRINT("Resolve of '" + resolver->queue[p_id].hostname + "'' didn't complete yet.");
		return IP_Address();
	}

	List<IP_Address> res = resolver->queue[p_id].response;

	for (int i = 0; i < res.size(); ++i) {
		if (res[i].is_valid()) {
			return res[i];
		}
	}
	return IP_Address();
}

Array IP::get_resolve_item_addresses(ResolverID p_id) const {
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, Array(), vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try performing less network requests at once.", p_id, IP::RESOLVER_MAX_QUERIES));

	MutexLock lock(resolver->mutex);

	if (resolver->queue[p_id].status.get() != IP::RESOLVER_STATUS_DONE) {
		ERR_PRINT("Resolve of '" + resolver->queue[p_id].hostname + "'' didn't complete yet.");
		return Array();
	}

	List<IP_Address> res = resolver->queue[p_id].response;

	Array result;
	for (int i = 0; i < res.size(); ++i) {
		if (res[i].is_valid()) {
			result.push_back(String(res[i]));
		}
	}
	return result;
}

void IP::erase_resolve_item(ResolverID p_id) {
	ERR_FAIL_INDEX_MSG(p_id, IP::RESOLVER_MAX_QUERIES, vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try performing less network requests at once.", p_id, IP::RESOLVER_MAX_QUERIES));

	resolver->queue[p_id].status.set(IP::RESOLVER_STATUS_NONE);
}

void IP::clear_cache(const String &p_hostname) {
	MutexLock lock(resolver->mutex);

	if (p_hostname.empty()) {
		resolver->cache.clear();
	} else {
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_NONE));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV4));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV6));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_ANY));
	}
}

Array IP::_get_local_addresses() const {
	Array addresses;
	List<IP_Address> ip_addresses;
	get_local_addresses(&ip_addresses);
	for (List<IP_Address>::Element *E = ip_addresses.front(); E; E = E->next()) {
		addresses.push_back(E->get());
	}

	return addresses;
}

Array IP::_get_local_interfaces() const {
	Array results;
	RBMap<String, Interface_Info> interfaces;
	get_local_interfaces(&interfaces);
	for (RBMap<String, Interface_Info>::Element *E = interfaces.front(); E; E = E->next()) {
		Interface_Info &c = E->get();
		Dictionary rc;
		rc["name"] = c.name;
		rc["friendly"] = c.name_friendly;
		rc["index"] = c.index;

		Array ips;
		for (const List<IP_Address>::Element *F = c.ip_addresses.front(); F; F = F->next()) {
			ips.push_front(F->get());
		}
		rc["addresses"] = ips;

		results.push_front(rc);
	}

	return results;
}

void IP::get_local_addresses(List<IP_Address> *r_addresses) const {
	RBMap<String, Interface_Info> interfaces;
	get_local_interfaces(&interfaces);
	for (RBMap<String, Interface_Info>::Element *E = interfaces.front(); E; E = E->next()) {
		for (const List<IP_Address>::Element *F = E->get().ip_addresses.front(); F; F = F->next()) {
			r_addresses->push_front(F->get());
		}
	}
}

void IP::_bind_methods() {
	ClassDB::bind_method(D_METHOD("resolve_hostname", "host", "ip_type"), &IP::resolve_hostname, DEFVAL(IP::TYPE_ANY));
	ClassDB::bind_method(D_METHOD("resolve_hostname_addresses", "host", "ip_type"), &IP::resolve_hostname_addresses, DEFVAL(IP::TYPE_ANY));
	ClassDB::bind_method(D_METHOD("resolve_hostname_queue_item", "host", "ip_type"), &IP::resolve_hostname_queue_item, DEFVAL(IP::TYPE_ANY));
	ClassDB::bind_method(D_METHOD("get_resolve_item_status", "id"), &IP::get_resolve_item_status);
	ClassDB::bind_method(D_METHOD("get_resolve_item_address", "id"), &IP::get_resolve_item_address);
	ClassDB::bind_method(D_METHOD("get_resolve_item_addresses", "id"), &IP::get_resolve_item_addresses);
	ClassDB::bind_method(D_METHOD("erase_resolve_item", "id"), &IP::erase_resolve_item);
	ClassDB::bind_method(D_METHOD("get_local_addresses"), &IP::_get_local_addresses);
	ClassDB::bind_method(D_METHOD("get_local_interfaces"), &IP::_get_local_interfaces);
	ClassDB::bind_method(D_METHOD("clear_cache", "hostname"), &IP::clear_cache, DEFVAL(""));

	BIND_ENUM_CONSTANT(RESOLVER_STATUS_NONE);
	BIND_ENUM_CONSTANT(RESOLVER_STATUS_WAITING);
	BIND_ENUM_CONSTANT(RESOLVER_STATUS_DONE);
	BIND_ENUM_CONSTANT(RESOLVER_STATUS_ERROR);

	BIND_CONSTANT(RESOLVER_MAX_QUERIES);
	BIND_CONSTANT(RESOLVER_INVALID_ID);

	BIND_ENUM_CONSTANT(TYPE_NONE);
	BIND_ENUM_CONSTANT(TYPE_IPV4);
	BIND_ENUM_CONSTANT(TYPE_IPV6);
	BIND_ENUM_CONSTANT(TYPE_ANY);
}

IP *IP::singleton = nullptr;

IP *IP::get_singleton() {
	return singleton;
}

IP *(*IP::_create)() = nullptr;

IP *IP::create() {
	ERR_FAIL_COND_V_MSG(singleton, nullptr, "IP singleton already exist.");
	ERR_FAIL_COND_V(!_create, nullptr);
	return _create();
}

IP::IP() {
	singleton = this;
	resolver = memnew(_IP_ResolverPrivate);

	resolver->thread_abort = false;
	resolver->thread.start(_IP_ResolverPrivate::_thread_function, resolver);
}

IP::~IP() {
	resolver->thread_abort = true;
	resolver->sem.post();
	resolver->thread.wait_to_finish();

	memdelete(resolver);
}
