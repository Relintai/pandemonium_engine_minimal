
/*  register_types.cpp                                                   */


#include "register_types.h"

#include "core/io/file_access_encrypted.h"
#include "core/io/resource_loader.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "gdscript.h"
#include "gdscript_tokenizer.h"

GDScriptLanguage *script_language_gd = nullptr;
Ref<ResourceFormatLoaderGDScript> resource_loader_gd;
Ref<ResourceFormatSaverGDScript> resource_saver_gd;

void register_gdscript_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SINGLETON) {
		script_language_gd = memnew(GDScriptLanguage);
		ScriptServer::register_language(script_language_gd);

		resource_loader_gd.instance();
		ResourceLoader::add_resource_format_loader(resource_loader_gd);

		resource_saver_gd.instance();
		ResourceSaver::add_resource_format_saver(resource_saver_gd);
	}

	if (p_level == MODULE_REGISTRATION_LEVEL_SCENE) {
		ClassDB::register_class<GDScript>();
		ClassDB::register_virtual_class<GDScriptFunctionState>();
	}
}

void unregister_gdscript_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SINGLETON) {
		ScriptServer::unregister_language(script_language_gd);

		if (script_language_gd) {
			memdelete(script_language_gd);
		}

		ResourceLoader::remove_resource_format_loader(resource_loader_gd);
		resource_loader_gd.unref();

		ResourceSaver::remove_resource_format_saver(resource_saver_gd);
		resource_saver_gd.unref();
	}
}
