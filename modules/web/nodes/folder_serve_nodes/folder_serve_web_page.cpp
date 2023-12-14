#include "folder_serve_web_page.h"

#include "../../file_cache.h"
#include "../../http/web_permission.h"
#include "../../http/web_server_request.h"

String FolderServeWebPage::get_serve_folder() {
	return _serve_folder;
}
void FolderServeWebPage::set_serve_folder(const String &val) {
	_serve_folder = val;
}

void FolderServeWebPage::_handle_request_main(Ref<WebServerRequest> request) {
	if (_web_permission.is_valid()) {
		if (_web_permission->activate(request)) {
			return;
		}
	}

	const String &rp = request->get_current_path_segment();

	if (rp == "") {
		if (!try_route_request_to_children(request)) {
			handle_request(request);
		}
		return;
	}

	String file_name = request->get_path(true, false);
	file_name = file_name.to_lower();

	int file_indx = _file_cache->wwwroot_get_file_index(file_name);

	if (file_indx != -1) {
		request->send_file(_file_cache->wwwroot_get_file_orig_path_abs(file_indx));
		return;
	}

	if (!try_route_request_to_children(request)) {
		handle_request(request);
	}
}

void FolderServeWebPage::load() {
	_file_cache->clear();

	if (_serve_folder == "") {
		_file_cache->set_wwwroot(_serve_folder);
		_file_cache->clear();
	} else {
		_file_cache->set_wwwroot(_serve_folder);
		_file_cache->wwwroot_refresh_cache();
	}
}

void FolderServeWebPage::_notification(const int what) {
	switch (what) {
		case NOTIFICATION_ENTER_TREE:
			load();
			break;
		default:
			break;
	}
}

FolderServeWebPage::FolderServeWebPage() {
	_file_cache.instance();
}

FolderServeWebPage::~FolderServeWebPage() {
}

void FolderServeWebPage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_serve_folder"), &FolderServeWebPage::get_serve_folder);
	ClassDB::bind_method(D_METHOD("set_serve_folder", "val"), &FolderServeWebPage::set_serve_folder);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "serve_folder"), "set_serve_folder", "get_serve_folder");

	ClassDB::bind_method(D_METHOD("load"), &FolderServeWebPage::load);
}
