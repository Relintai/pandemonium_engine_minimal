
/*  register_types.cpp                                                   */


#include "register_types.h"
#include "noise_texture.h"
#include "open_simplex_noise.h"

void register_opensimplex_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SCENE) {
		ClassDB::register_class<OpenSimplexNoise>();
		ClassDB::register_class<NoiseTexture>();
	}
}

void unregister_opensimplex_types(ModuleRegistrationLevel p_level) {
}
