#ifndef DOC_DATA_H
#define DOC_DATA_H

/*  doc_data.h                                                           */


#include "core/containers/rb_map.h"
#include "core/variant/variant.h"
#include "core/object/reference.h"
#include "core/error/error_list.h"
#include "core/string/ustring.h"
#include "core/containers/vector.h"

class XMLParser;

class DocData {
public:
	struct ArgumentDoc {
		String name;
		String type;
		String enumeration;
		String default_value;
	};

	struct MethodDoc {
		String name;
		String return_type;
		String return_enum;
		String qualifiers;
		String description;
		Vector<ArgumentDoc> arguments;
		bool operator<(const MethodDoc &p_md) const {
			return name < p_md.name;
		}
	};

	struct ConstantDoc {
		String name;
		String value;
		bool is_value_valid;
		String enumeration;
		String description;
	};

	struct PropertyDoc {
		String name;
		String type;
		String enumeration;
		String description;
		String setter, getter;
		String default_value;
		bool overridden;
		String overrides;
		bool operator<(const PropertyDoc &p_prop) const {
			return name < p_prop.name;
		}
		PropertyDoc() {
			overridden = false;
		}
	};

	struct ThemeItemDoc {
		String name;
		String type;
		String data_type;
		String description;
		String default_value;
		bool operator<(const ThemeItemDoc &p_theme_item) const {
			// First sort by the data type, then by name.
			if (data_type == p_theme_item.data_type) {
				return name < p_theme_item.name;
			}
			return data_type < p_theme_item.data_type;
		}
	};

	struct TutorialDoc {
		String link;
		String title;
	};

	struct ClassDoc {
		String name;
		String inherits;
		String category;
		String brief_description;
		String description;
		Vector<TutorialDoc> tutorials;
		Vector<MethodDoc> methods;
		Vector<MethodDoc> signals;
		Vector<ConstantDoc> constants;
		Vector<PropertyDoc> properties;
		Vector<ThemeItemDoc> theme_properties;
	};

	String version;

	RBMap<String, ClassDoc> class_list;
	Error _load(Ref<XMLParser> parser);

public:
	void merge_from(const DocData &p_data);
	void remove_from(const DocData &p_data);
	void generate(bool p_basic_types = false);
	Error load_classes(const String &p_dir);
	static Error erase_classes(const String &p_dir);
	Error save_classes(const String &p_default_path, const RBMap<String, String> &p_class_path);
};

#endif // DOC_DATA_H
