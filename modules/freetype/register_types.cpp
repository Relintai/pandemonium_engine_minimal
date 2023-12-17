
/*  register_types.cpp                                                   */


#include "register_types.h"

#include "dynamic_font.h"
#include "scene/main/scene_tree.h"

static Ref<ResourceFormatLoaderDynamicFont> resource_loader_dynamic_font;

void register_freetype_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SINGLETON) {
		resource_loader_dynamic_font.instance();
		ResourceLoader::add_resource_format_loader(resource_loader_dynamic_font);
	}

	if (p_level == MODULE_REGISTRATION_LEVEL_SCENE) {
		ClassDB::register_class<DynamicFontData>();
		ClassDB::register_class<DynamicFont>();

		DynamicFont::initialize_dynamic_fonts();
	}

    if (p_level == MODULE_REGISTRATION_LEVEL_FINALIZE) {
        if (SceneTree::get_singleton()) {
            SceneTree::get_singleton()->connect("update_font_oversampling_request", resource_loader_dynamic_font.ptr(), "_scene_tree_update_font_oversampling");
        }
    }
}

void unregister_freetype_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SINGLETON) {
		ResourceLoader::remove_resource_format_loader(resource_loader_dynamic_font);
		resource_loader_dynamic_font.unref();

		DynamicFont::finish_dynamic_fonts();
	}
}
