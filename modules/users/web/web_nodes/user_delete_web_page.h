#ifndef USER_DELETE_WEB_PAGE_H
#define USER_DELETE_WEB_PAGE_H

#include "core/containers/vector.h"
#include "core/object/reference.h"
#include "core/string/ustring.h"

#include "user_web_page.h"

class WebServerRequest;
class FormValidator;
class User;

class UserDeleteWebPage : public UserWebPage {
	GDCLASS(UserDeleteWebPage, UserWebPage);

public:
	virtual void handle_delete_request(Ref<User> &user, Ref<WebServerRequest> request);

	UserDeleteWebPage();
	~UserDeleteWebPage();

protected:
	static void _bind_methods();
};

#endif
