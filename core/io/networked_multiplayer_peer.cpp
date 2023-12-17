
/*  networked_multiplayer_peer.cpp                                       */


#include "networked_multiplayer_peer.h"

#include "core/os/os.h"

uint32_t NetworkedMultiplayerPeer::generate_unique_id() const {
	uint32_t hash = 0;

	while (hash == 0 || hash == 1) {
		hash = hash_murmur3_one_32(
				(uint32_t)OS::get_singleton()->get_ticks_usec());
		hash = hash_murmur3_one_32(
				(uint32_t)OS::get_singleton()->get_unix_time(), hash);
		hash = hash_murmur3_one_32(
				(uint32_t)OS::get_singleton()->get_user_data_dir().hash64(), hash);
		hash = hash_murmur3_one_32(
				(uint32_t)((uint64_t)this), hash); // Rely on ASLR heap
		hash = hash_murmur3_one_32(
				(uint32_t)((uint64_t)&hash), hash); // Rely on ASLR stack

		hash = hash_fmix32(hash);
		hash = hash & 0x7FFFFFFF; // Make it compatible with unsigned, since negative ID is used for exclusion
	}

	return hash;
}

void NetworkedMultiplayerPeer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_transfer_mode", "mode"), &NetworkedMultiplayerPeer::set_transfer_mode);
	ClassDB::bind_method(D_METHOD("get_transfer_mode"), &NetworkedMultiplayerPeer::get_transfer_mode);
	ClassDB::bind_method(D_METHOD("set_target_peer", "id"), &NetworkedMultiplayerPeer::set_target_peer);

	ClassDB::bind_method(D_METHOD("get_packet_peer"), &NetworkedMultiplayerPeer::get_packet_peer);

	ClassDB::bind_method(D_METHOD("poll"), &NetworkedMultiplayerPeer::poll);

	ClassDB::bind_method(D_METHOD("get_connection_status"), &NetworkedMultiplayerPeer::get_connection_status);
	ClassDB::bind_method(D_METHOD("get_unique_id"), &NetworkedMultiplayerPeer::get_unique_id);

	ClassDB::bind_method(D_METHOD("set_refuse_new_connections", "enable"), &NetworkedMultiplayerPeer::set_refuse_new_connections);
	ClassDB::bind_method(D_METHOD("is_refusing_new_connections"), &NetworkedMultiplayerPeer::is_refusing_new_connections);

	ClassDB::bind_method(D_METHOD("generate_unique_id"), &NetworkedMultiplayerPeer::generate_unique_id);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "refuse_new_connections"), "set_refuse_new_connections", "is_refusing_new_connections");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "transfer_mode", PROPERTY_HINT_ENUM, "Unreliable,Unreliable Ordered,Reliable"), "set_transfer_mode", "get_transfer_mode");

	BIND_ENUM_CONSTANT(TRANSFER_MODE_UNRELIABLE);
	BIND_ENUM_CONSTANT(TRANSFER_MODE_UNRELIABLE_ORDERED);
	BIND_ENUM_CONSTANT(TRANSFER_MODE_RELIABLE);

	BIND_ENUM_CONSTANT(CONNECTION_DISCONNECTED);
	BIND_ENUM_CONSTANT(CONNECTION_CONNECTING);
	BIND_ENUM_CONSTANT(CONNECTION_CONNECTED);

	BIND_CONSTANT(TARGET_PEER_BROADCAST);
	BIND_CONSTANT(TARGET_PEER_SERVER);

	ADD_SIGNAL(MethodInfo("peer_connected", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("peer_disconnected", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("server_disconnected"));
	ADD_SIGNAL(MethodInfo("connection_succeeded"));
	ADD_SIGNAL(MethodInfo("connection_failed"));
}

NetworkedMultiplayerPeer::NetworkedMultiplayerPeer() {
}
