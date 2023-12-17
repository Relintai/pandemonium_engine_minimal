
/*  register_types.cpp                                                   */


#include "register_types.h"

#include "audio_stream_ogg_vorbis.h"

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#include "resource_importer_ogg_vorbis.h"
#endif

void register_stb_vorbis_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SCENE) {
		ClassDB::register_class<AudioStreamOGGVorbis>();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_REGISTRATION_LEVEL_EDITOR) {
		if (Engine::get_singleton()->is_editor_hint()) {
			Ref<ResourceImporterOGGVorbis> ogg_import;
			ogg_import.instance();
			ResourceFormatImporter::get_singleton()->add_importer(ogg_import);
		}
	}
#endif
}

void unregister_stb_vorbis_types(ModuleRegistrationLevel p_level) {
}
