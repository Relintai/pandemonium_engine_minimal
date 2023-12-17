
/*  register_types.cpp                                                   */


#include "register_types.h"

#include "audio_stream_mp3.h"

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#include "resource_importer_mp3.h"
#endif

void register_minimp3_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SCENE) {
		ClassDB::register_class<AudioStreamMP3>();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_REGISTRATION_LEVEL_EDITOR) {
		if (Engine::get_singleton()->is_editor_hint()) {
			Ref<ResourceImporterMP3> mp3_import;
			mp3_import.instance();
			ResourceFormatImporter::get_singleton()->add_importer(mp3_import);
		}
	}
#endif
}

void unregister_minimp3_types(ModuleRegistrationLevel p_level) {
}
