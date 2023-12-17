#ifndef DIR_ACCESS_H
#define DIR_ACCESS_H

/*  dir_access.h                                                         */


#include "core/typedefs.h"
#include "core/string/ustring.h"

//@ TODO, excellent candidate for THREAD_SAFE MACRO, should go through all these and add THREAD_SAFE where it applies
class DirAccess {
public:
	enum AccessType {
		ACCESS_RESOURCES,
		ACCESS_USERDATA,
		ACCESS_FILESYSTEM,
		ACCESS_MAX
	};

	typedef DirAccess *(*CreateFunc)();

private:
	AccessType _access_type;
	static CreateFunc create_func[ACCESS_MAX]; ///< set this to instance a filesystem object

	Error _copy_dir(DirAccess *p_target_da, String p_to, int p_chmod_flags, bool p_copy_links);

protected:
	String _get_root_path() const;
	virtual String _get_root_string() const;

	AccessType get_access_type() const;
	String fix_path(String p_path) const;
	bool next_is_dir;

	template <class T>
	static DirAccess *_create_builtin() {
		return memnew(T);
	}

public:
	virtual Error list_dir_begin() = 0; ///< This starts dir listing
	virtual String get_next() = 0;
	virtual bool current_is_dir() const = 0;
	virtual bool current_is_hidden() const = 0;

	virtual void list_dir_end() = 0; ///<

	virtual int get_drive_count() = 0;
	virtual String get_drive(int p_drive) = 0;
	virtual int get_current_drive();
	virtual bool drives_are_shortcuts();

	virtual Error change_dir(String p_dir) = 0; ///< can be relative or absolute, return false on success
	virtual String get_current_dir() = 0; ///< return current dir location
	virtual String get_current_dir_without_drive();
	virtual Error make_dir(String p_dir) = 0;
	virtual Error make_dir_recursive(String p_dir);
	virtual Error erase_contents_recursive(); //super dangerous, use with care!

	virtual bool file_exists(String p_file) = 0;
	virtual bool dir_exists(String p_dir) = 0;
	static bool exists(String p_dir);
	virtual uint64_t get_space_left() = 0;

	Error copy_dir(String p_from, String p_to, int p_chmod_flags = -1, bool p_copy_links = false);
	virtual Error copy(String p_from, String p_to, int p_chmod_flags = -1);
	virtual Error rename(String p_from, String p_to) = 0;
	virtual Error remove(String p_name) = 0;

	virtual bool is_link(String p_file) = 0;
	virtual String read_link(String p_file) = 0;
	virtual Error create_link(String p_source, String p_target) = 0;

	// Meant for editor code when we want to quickly remove a file without custom
	// handling (e.g. removing a cache file).
	static void remove_file_or_error(String p_path) {
		DirAccess *da = create(ACCESS_FILESYSTEM);
		if (da->file_exists(p_path)) {
			if (da->remove(p_path) != OK) {
				ERR_FAIL_MSG("Cannot remove file or directory: " + p_path);
			}
		}
		memdelete(da);
	}

	virtual String get_filesystem_type() const = 0;
	static String get_full_path(const String &p_path, AccessType p_access);
	static DirAccess *create_for_path(const String &p_path);

	/*
	enum DirType {

		FILE_TYPE_INVALID,
		FILE_TYPE_FILE,
		FILE_TYPE_DIR,
	};

	//virtual DirType get_file_type() const=0;
*/
	static DirAccess *create(AccessType p_access);

	template <class T>
	static void make_default(AccessType p_access) {
		create_func[p_access] = _create_builtin<T>;
	}

	static DirAccess *open(const String &p_path, Error *r_error = nullptr);

	static String get_filesystem_abspath_for(String p_path);

	DirAccess();
	virtual ~DirAccess();
};

struct DirAccessRef {
	DirAccess *f;

	_FORCE_INLINE_ bool is_null() const { return f == nullptr; }
	_FORCE_INLINE_ bool is_valid() const { return f != nullptr; }

	_FORCE_INLINE_ operator bool() const { return f != nullptr; }
	_FORCE_INLINE_ operator DirAccess *() { return f; }

	_FORCE_INLINE_ DirAccess *operator->() {
		return f;
	}

	DirAccessRef(DirAccess *fa) { f = fa; }
	DirAccessRef(DirAccessRef &&other) {
		f = other.f;
		other.f = nullptr;
	}
	~DirAccessRef() {
		if (f) {
			memdelete(f);
		}
	}
};

#endif
