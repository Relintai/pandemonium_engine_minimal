#ifndef USER_SETTINGS_WEB_PAGE_H
#define USER_SETTINGS_WEB_PAGE_H

#include "core/containers/vector.h"
#include "core/object/reference.h"
#include "core/string/ustring.h"

#include "user_web_page.h"

class WebServerRequest;
class FormValidator;
class User;

class UserSettingsWebPage : public UserWebPage {
	GDCLASS(UserSettingsWebPage, UserWebPage);

public:
	struct SettingsRequestData {
		String error_str;

		String uname_val;
		String email_val;
		String pass_val;
		String pass_check_val;
	};

	void _render_index(Ref<WebServerRequest> request);
	virtual void render_settings_request(Ref<User> &user, Ref<WebServerRequest> request, SettingsRequestData *data);

	UserSettingsWebPage();
	~UserSettingsWebPage();

protected:
	static void _bind_methods();

	Ref<FormValidator> _profile_validator;
};

#endif
