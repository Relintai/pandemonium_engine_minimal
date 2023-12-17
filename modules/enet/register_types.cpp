
/*  register_types.cpp                                                   */


#include "register_types.h"
#include "core/error/error_macros.h"
#include "networked_multiplayer_enet.h"

static bool enet_ok = false;

void register_enet_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SINGLETON) {
		if (enet_initialize() != 0) {
			ERR_PRINT("ENet initialization failure");
		} else {
			enet_ok = true;
		}
	}

	if (p_level == MODULE_REGISTRATION_LEVEL_SCENE) {
		ClassDB::register_class<NetworkedMultiplayerENet>();
	}
}

void unregister_enet_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_SINGLETON) {
		if (enet_ok) {
			enet_deinitialize();
		}
	}
}
