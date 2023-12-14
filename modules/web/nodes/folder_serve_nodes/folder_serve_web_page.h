#ifndef FOLDER_SERVE_WEB_PAGE_H
#define FOLDER_SERVE_WEB_PAGE_H

#include "core/object/reference.h"
#include "core/string/ustring.h"

#include "../../http/web_node.h"

class WebServerRequest;
class FileCache;

// This class will serve the files from the folder set to it's serve_folder property.
// It will cache the folder's contents on ENTER_TREE, and will match against the cached list,
// this means directory walking (for example sending http://webapp.com/files/../../../etc/passwd),
// and other techniques like it should not be possible.

class FolderServeWebPage : public WebNode {
	GDCLASS(FolderServeWebPage, WebNode);

public:
	String get_serve_folder();
	void set_serve_folder(const String &val);

	void _handle_request_main(Ref<WebServerRequest> request);

	virtual void load();

	FolderServeWebPage();
	~FolderServeWebPage();

protected:
	void _notification(const int what);
	static void _bind_methods();

	String _serve_folder;

	Ref<FileCache> _file_cache;
};

#endif
