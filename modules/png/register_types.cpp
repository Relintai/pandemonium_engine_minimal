
/*  register_types.cpp                                                   */


#include "register_types.h"

#include "image_loader_png.h"
#include "resource_saver_png.h"

static ImageLoaderPNG *image_loader_png = NULL;
static Ref<ResourceSaverPNG> resource_saver_png;

void register_png_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_CORE) {
		image_loader_png = memnew(ImageLoaderPNG);
		ImageLoader::add_image_format_loader(image_loader_png);

		resource_saver_png.instance();
		ResourceSaver::add_resource_format_saver(resource_saver_png);
	}
}

void unregister_png_types(ModuleRegistrationLevel p_level) {
	if (p_level == MODULE_REGISTRATION_LEVEL_CORE) {
		if (image_loader_png) {
			memdelete(image_loader_png);
		}

		ResourceSaver::remove_resource_format_saver(resource_saver_png);
		resource_saver_png.unref();
	}
}
