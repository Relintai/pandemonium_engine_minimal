
/*  register_server_types.cpp                                            */


#include "register_server_types.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"

#include "audio/audio_effect.h"
#include "audio/audio_stream.h"
#include "audio/effects/audio_effect_amplify.h"
#include "audio/effects/audio_effect_capture.h"
#include "audio/effects/audio_effect_chorus.h"
#include "audio/effects/audio_effect_compressor.h"
#include "audio/effects/audio_effect_delay.h"
#include "audio/effects/audio_effect_distortion.h"
#include "audio/effects/audio_effect_eq.h"
#include "audio/effects/audio_effect_filter.h"
#include "audio/effects/audio_effect_limiter.h"
#include "audio/effects/audio_effect_panner.h"
#include "audio/effects/audio_effect_phaser.h"
#include "audio/effects/audio_effect_pitch_shift.h"
#include "audio/effects/audio_effect_record.h"
#include "audio/effects/audio_effect_reverb.h"
#include "audio/effects/audio_effect_spectrum_analyzer.h"
#include "audio/effects/audio_effect_stereo_enhance.h"
#include "audio/effects/audio_stream_generator.h"
#include "audio_server.h"
#include "physics_2d/physics_2d_server_sw.h"
#include "physics_2d/physics_2d_server_wrap_mt.h"
#include "rendering/shader_types.h"
#include "servers/physics_2d_server.h"
#include "servers/rendering_server.h"

ShaderTypes *shader_types = nullptr;

Physics2DServer *_createPandemoniumPhysics2DCallback() {
	return Physics2DServerWrapMT::init_server<Physics2DServerSW>();
}

static bool has_server_feature_callback(const String &p_feature) {
	if (RenderingServer::get_singleton()) {
		if (RenderingServer::get_singleton()->has_os_feature(p_feature)) {
			return true;
		}
	}

	return false;
}

void register_server_types() {
	OS::get_singleton()->set_has_server_feature_callback(has_server_feature_callback);

	ClassDB::register_virtual_class<RenderingServer>();
	
	ClassDB::register_class<AudioServer>();

	ClassDB::register_virtual_class<Physics2DServer>();

	shader_types = memnew(ShaderTypes);

	ClassDB::register_virtual_class<AudioStream>();
	ClassDB::register_virtual_class<AudioStreamPlayback>();
	ClassDB::register_virtual_class<AudioStreamPlaybackResampled>();
	ClassDB::register_class<AudioStreamMicrophone>();
	ClassDB::register_class<AudioStreamRandomPitch>();
	ClassDB::register_virtual_class<AudioEffect>();
	ClassDB::register_virtual_class<AudioEffectInstance>();
	ClassDB::register_class<AudioEffectEQ>();
	ClassDB::register_class<AudioEffectFilter>();
	ClassDB::register_class<AudioBusLayout>();

	ClassDB::register_class<AudioStreamGenerator>();
	ClassDB::register_virtual_class<AudioStreamGeneratorPlayback>();

	{
		//audio effects
		ClassDB::register_class<AudioEffectAmplify>();

		ClassDB::register_class<AudioEffectReverb>();

		ClassDB::register_class<AudioEffectLowPassFilter>();
		ClassDB::register_class<AudioEffectHighPassFilter>();
		ClassDB::register_class<AudioEffectBandPassFilter>();
		ClassDB::register_class<AudioEffectNotchFilter>();
		ClassDB::register_class<AudioEffectBandLimitFilter>();
		ClassDB::register_class<AudioEffectLowShelfFilter>();
		ClassDB::register_class<AudioEffectHighShelfFilter>();

		ClassDB::register_class<AudioEffectEQ6>();
		ClassDB::register_class<AudioEffectEQ10>();
		ClassDB::register_class<AudioEffectEQ21>();

		ClassDB::register_class<AudioEffectDistortion>();

		ClassDB::register_class<AudioEffectStereoEnhance>();

		ClassDB::register_class<AudioEffectPanner>();
		ClassDB::register_class<AudioEffectChorus>();
		ClassDB::register_class<AudioEffectDelay>();
		ClassDB::register_class<AudioEffectCompressor>();
		ClassDB::register_class<AudioEffectLimiter>();
		ClassDB::register_class<AudioEffectPitchShift>();
		ClassDB::register_class<AudioEffectPhaser>();

		ClassDB::register_class<AudioEffectRecord>();
		ClassDB::register_class<AudioEffectSpectrumAnalyzer>();
		ClassDB::register_virtual_class<AudioEffectSpectrumAnalyzerInstance>();

		ClassDB::register_class<AudioEffectCapture>();
	}

	ClassDB::register_virtual_class<Physics2DDirectBodyState>();
	ClassDB::register_virtual_class<Physics2DDirectSpaceState>();
	ClassDB::register_class<Physics2DTestMotionResult>();
	ClassDB::register_class<Physics2DShapeQueryParameters>();

	// Physics 2D
	GLOBAL_DEF(Physics2DServerManager::setting_property_name, "DEFAULT");
	ProjectSettings::get_singleton()->set_custom_property_info(Physics2DServerManager::setting_property_name, PropertyInfo(Variant::STRING, Physics2DServerManager::setting_property_name, PROPERTY_HINT_ENUM, "DEFAULT"));

	Physics2DServerManager::register_server("PandemoniumPhysics", &_createPandemoniumPhysics2DCallback);
	Physics2DServerManager::set_default_server("PandemoniumPhysics");
}

void unregister_server_types() {
	memdelete(shader_types);
}

void register_server_singletons() {
	Engine::get_singleton()->add_singleton(Engine::Singleton("RenderingServer", RenderingServer::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Physics2DServer", Physics2DServer::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("AudioServer", AudioServer::get_singleton()));
}
