#include "user_logout_web_page.h"

#include "../../singleton/user_db.h"
#include "../../users/user.h"

#include "core/variant/variant.h"
#include "modules/web/html/form_validator.h"
#include "modules/web/html/html_builder.h"
#include "modules/web/http/http_server_enums.h"
#include "modules/web/http/http_session.h"
#include "modules/web/http/http_session_manager.h"
#include "modules/web/http/web_permission.h"
#include "modules/web/http/web_server.h"
#include "modules/web/http/web_server_cookie.h"
#include "modules/web/http/web_server_request.h"

void UserLogoutWebPage::_render_index(Ref<WebServerRequest> request) {
	Ref<User> user = request->get_meta("user");

	ERR_FAIL_COND(!user.is_valid());

	request->response_remove_cookie_simple("session_id");

	user->save();

	HTTPSessionManager *sess_man = request->get_server()->get_session_manager();
	sess_man->delete_session(request->get_session()->get_session_id());
	request->set_session(Ref<HTTPSession>());

	emit_signal("user_logged_out", request, user);

	if (has_method("_render_user_page")) {
		Dictionary d;

		d["user"] = user;

		call("_render_user_page", request, d);
	} else {
		HTMLBuilder b;
		b.w("Logout successful!");
		request->body += b.result;

		request->compile_and_send_body();
	}
}

UserLogoutWebPage::UserLogoutWebPage() {
}

UserLogoutWebPage::~UserLogoutWebPage() {
}

void UserLogoutWebPage::_bind_methods() {
	ADD_SIGNAL(MethodInfo("user_logged_out", PropertyInfo(Variant::OBJECT, "request", PROPERTY_HINT_RESOURCE_TYPE, "WebServerRequest"), PropertyInfo(Variant::OBJECT, "user", PROPERTY_HINT_RESOURCE_TYPE, "User")));
}