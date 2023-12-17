#ifndef REGISTER_MODULE_TYPES_H
#define REGISTER_MODULE_TYPES_H

/*  register_module_types.h                                              */


// Note: The engine will call register_module_types in this order, 
// and in reverse order (except for start and finalize) when it goes through unregister_module_types.

enum ModuleRegistrationLevel {
	// Starting initialization, on uninitialization
	MODULE_REGISTRATION_LEVEL_START = 0,

	// Set up your singletons here.
	MODULE_REGISTRATION_LEVEL_SINGLETON,

	// Set up things like resource loaders here.
	MODULE_REGISTRATION_LEVEL_CORE,

	// Set up driver level things here.
    MODULE_REGISTRATION_LEVEL_DRIVER,

	// Set up platform level things here.
	MODULE_REGISTRATION_LEVEL_PLATFORM,

	// Set up servers here
	MODULE_REGISTRATION_LEVEL_SERVER,

	// Set up scene related things here. (Mostly normal class registrations.)
	MODULE_REGISTRATION_LEVEL_SCENE,

	// Set up scene related things here. (Mostly editor class registrations.)
	MODULE_REGISTRATION_LEVEL_EDITOR,

	// Set up testing related things here. Will only get called if necessary. (Mostly test registrations.)
    MODULE_REGISTRATION_LEVEL_TEST,

	// After everything have been set up, or uninitialized.
	// Good place to change some settings, or maybe to do something like disabling an another modules's editor plugin when necessary.
	MODULE_REGISTRATION_LEVEL_FINALIZE,
};

void register_module_types(ModuleRegistrationLevel p_level);
void unregister_module_types(ModuleRegistrationLevel p_level);

#endif // REGISTER_MODULE_TYPES_H
