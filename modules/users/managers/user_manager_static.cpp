#include "user_manager_static.h"

#include "../users/user.h"

Ref<User> UserManagerStatic::_get_user(const int id) {
	ERR_FAIL_INDEX_V(id, _users.size(), Ref<User>());

	return _users[id];
}
Ref<User> UserManagerStatic::_get_user_name(const String &user_name) {
	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_user_name() == user_name) {
				return u;
			}
		}
	}

	return Ref<User>();
}
Ref<User> UserManagerStatic::_get_user_email(const String &user_email) {
	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_email() == user_email) {
				return u;
			}
		}
	}

	return Ref<User>();
}

void UserManagerStatic::_save_user(Ref<User> user) {
	//With this class Users are serialized via editor properties, ignore
}
Ref<User> UserManagerStatic::_create_user() {
	Ref<User> u;
	u.instance();

	u->set_user_id(_users.size());

	_users.push_back(u);

	return u;
}
bool UserManagerStatic::_is_username_taken(const String &user_name) {
	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_user_name() == user_name) {
				return true;
			}
		}
	}

	return false;
}
bool UserManagerStatic::_is_email_taken(const String &email) {
	for (int i = 0; i < _users.size(); ++i) {
		Ref<User> u = _users[i];

		if (u.is_valid()) {
			if (u->get_email() == email) {
				return true;
			}
		}
	}

	return false;
}

Vector<Ref<User>> UserManagerStatic::get_all() {
	return _users;
}

Vector<Variant> UserManagerStatic::get_users() {
	Vector<Variant> r;
	for (int i = 0; i < _users.size(); i++) {
		r.push_back(_users[i].get_ref_ptr());
	}
	return r;
}
void UserManagerStatic::set_users(const Vector<Variant> &users) {
	_users.clear();
	for (int i = 0; i < users.size(); i++) {
		Ref<User> u = Ref<User>(users.get(i));

		_users.push_back(u);
	}
}

String UserManagerStatic::get_create_user_name_bind() {
	return _create_user_name;
}
void UserManagerStatic::set_create_user_name_bind(const String &val) {
	_create_user_name = val;
}

String UserManagerStatic::get_create_user_email_bind() {
	return _create_user_email;
}
void UserManagerStatic::set_create_user_email_bind(const String &val) {
	_create_user_email = val;
}

String UserManagerStatic::get_create_user_password_bind() {
	return _create_user_password;
}
void UserManagerStatic::set_create_user_password_bind(const String &val) {
	_create_user_password = val;
}

bool UserManagerStatic::get_create_user_bind() {
	return false;
}
void UserManagerStatic::set_create_user_bind(const bool val) {
	if (val) {
		Ref<User> u = create_user();

		u->set_user_name(_create_user_name);
		u->set_email(_create_user_email);
		u->create_password(_create_user_password);
		u->save();

		_create_user_password = "";
		_create_user_email = "";
		_create_user_name = "";
	}
}

UserManagerStatic::UserManagerStatic() {
}

UserManagerStatic::~UserManagerStatic() {
}

void UserManagerStatic::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_users"), &UserManagerStatic::get_users);
	ClassDB::bind_method(D_METHOD("set_users", "users"), &UserManagerStatic::set_users);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "users", PROPERTY_HINT_NONE, "23/20:User", PROPERTY_USAGE_DEFAULT, "User"), "set_users", "get_users");

	ClassDB::bind_method(D_METHOD("get_create_user_name"), &UserManagerStatic::get_create_user_name_bind);
	ClassDB::bind_method(D_METHOD("set_create_user_name", "val"), &UserManagerStatic::set_create_user_name_bind);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "create_user_name"), "set_create_user_name", "get_create_user_name");

	ClassDB::bind_method(D_METHOD("get_create_user_email"), &UserManagerStatic::get_create_user_email_bind);
	ClassDB::bind_method(D_METHOD("set_create_user_email", "val"), &UserManagerStatic::set_create_user_email_bind);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "create_user_email"), "set_create_user_email", "get_create_user_email");

	ClassDB::bind_method(D_METHOD("get_create_user_password"), &UserManagerStatic::get_create_user_password_bind);
	ClassDB::bind_method(D_METHOD("set_create_user_password", "val"), &UserManagerStatic::set_create_user_password_bind);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "create_user_password"), "set_create_user_password", "get_create_user_password");

	ClassDB::bind_method(D_METHOD("get_create_user"), &UserManagerStatic::get_create_user_bind);
	ClassDB::bind_method(D_METHOD("set_create_user", "val"), &UserManagerStatic::set_create_user_bind);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_user"), "set_create_user", "get_create_user");
}
