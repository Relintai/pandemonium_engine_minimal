#include "user_manager_file.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/variant/variant.h"

#include "../users/user.h"

String UserManagerFile::get_save_folder_path() {
	return _save_folder_path;
}
void UserManagerFile::set_save_folder_path(const String &val) {
	_save_folder_path = val;
}

Ref<User> UserManagerFile::_get_user(const int id) {
	_rw_lock.read_lock();

	if (id < 0 || id >= _users.size()) {
		_rw_lock.read_unlock();

		ERR_PRINT("INDEX ERROR");

		return Ref<User>();
	}

	Ref<User> u = _users[id];

	_rw_lock.read_unlock();
	return u;
}
Ref<User> UserManagerFile::_get_user_name(const String &user_name) {
	_rw_lock.read_lock();

	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_user_name() == user_name) {
				_rw_lock.read_unlock();

				return u;
			}
		}
	}

	_rw_lock.read_unlock();
	return Ref<User>();
}
Ref<User> UserManagerFile::_get_user_email(const String &user_email) {
	_rw_lock.read_lock();

	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_email() == user_email) {
				_rw_lock.read_unlock();

				return u;
			}
		}
	}

	_rw_lock.read_unlock();
	return Ref<User>();
}

void UserManagerFile::_save_user(Ref<User> user) {
	_save_queued = true;
}
Ref<User> UserManagerFile::_create_user() {
	_rw_lock.write_lock();

	Ref<User> u;
	u.instance();
	u->set_user_id(_users.size());
	u->connect("changed", this, "_on_user_changed");

	_users.push_back(u);

	_rw_lock.write_unlock();

	_save_queued = true;

	return u;
}
bool UserManagerFile::_is_username_taken(const String &user_name) {
	_rw_lock.read_lock();
	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_user_name() == user_name) {
				_rw_lock.read_unlock();
				return true;
			}
		}
	}

	_rw_lock.read_unlock();
	return false;
}
bool UserManagerFile::_is_email_taken(const String &email) {
	_rw_lock.read_lock();
	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_email() == email) {
				_rw_lock.read_unlock();
				return true;
			}
		}
	}

	_rw_lock.read_unlock();
	return false;
}

Vector<Ref<User>> UserManagerFile::get_all() {
	return _users;
}

UserManagerFile::UserManagerFile() {
	set_process_internal(true);
	_save_folder_path = "users";
}

UserManagerFile::~UserManagerFile() {
}

void UserManagerFile::load() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	String fpath = "user://" + _save_folder_path;

	DirAccess *dir = DirAccess::open(fpath);

	if (!dir) {
		return;
	}

	dir->list_dir_begin();
	_rw_lock.write_lock();

	String file = dir->get_next();

	while (file != "") {
		if (!dir->current_is_dir()) {
			if (!file.ends_with(".tres")) {
				file = dir->get_next();
				continue;
			}

			String uids = file.get_slice(".", 0);

			if (!uids.is_valid_unsigned_integer()) {
				continue;
			}

			int id = uids.to_int();

			//Overflow
			ERR_CONTINUE(id < 0);

			//TODO Users should be able to serialize themselves to json, could work similarly to Entity
			//need to think, maybe it can be done better.
			//Also this is not a super safe way to do this, this will definitely be changed, relatively soon
			Ref<User> u = ResourceLoader::load(fpath.plus_file(file), "User", true);
			//Unset script, just for good measure
			u->set_script(RefPtr());

			u->connect("changed", this, "_on_user_changed");

			if (_users.size() <= id) {
				_users.resize(id + 1);
			}

			_users.write[id] = u;
		}

		file = dir->get_next();
	}

	dir->list_dir_end();

	_rw_lock.write_unlock();
}

void UserManagerFile::save() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	String fpath = "user://" + _save_folder_path;

	DirAccess *dir = DirAccess::open(fpath);

	if (!dir) {
		DirAccess *diru = DirAccess::open("user://");
		diru->make_dir_recursive(fpath);

		memdelete(diru);

		dir = DirAccess::open(fpath);
	}

	ERR_FAIL_COND(!dir);

	dir->list_dir_begin();

	String file = dir->get_next();

	while (file != "") {
		if (!dir->current_is_dir()) {
			if (!file.ends_with(".tres")) {
				file = dir->get_next();
				continue;
			}

			dir->remove(file);
		}

		file = dir->get_next();
	}

	dir->list_dir_end();

	memdelete(dir);

	_rw_lock.read_lock();

	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			u->read_lock();
			ResourceSaver::save(fpath.plus_file(itos(i) + ".tres"), u);
			u->read_unlock();
		}
	}

	_rw_lock.read_unlock();
}

void UserManagerFile::_on_user_changed() {
	_save_queued = true;
}

void UserManagerFile::_notification(int p_what) {
	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		if (_save_queued) {
			_save_queued = false;
			save();
		}
	} else if (p_what == NOTIFICATION_POST_ENTER_TREE) {
		load();
	}
}

void UserManagerFile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_save_folder_path"), &UserManagerFile::get_save_folder_path);
	ClassDB::bind_method(D_METHOD("set_save_folder_path", "val"), &UserManagerFile::set_save_folder_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "save_folder_path"), "set_save_folder_path", "get_save_folder_path");

	ClassDB::bind_method(D_METHOD("_on_user_changed"), &UserManagerFile::_on_user_changed);
}
