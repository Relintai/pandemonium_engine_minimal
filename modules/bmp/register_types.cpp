
/*  register_types.cpp                                                   */


#include "register_types.h"

#include "image_loader_bmp.h"

static ImageLoaderBMP *image_loader_bmp = nullptr;

void register_bmp_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_CORE) {
		image_loader_bmp = memnew(ImageLoaderBMP);
		ImageLoader::add_image_format_loader(image_loader_bmp);
	}
}

void unregister_bmp_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_CORE) {
		memdelete(image_loader_bmp);
	}
}
