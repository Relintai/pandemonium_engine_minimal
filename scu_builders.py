"""Functions used to generate scu build source files during build time
"""
import glob, os
import math
from pathlib import Path
from os.path import normpath, basename

base_folder_path = str(Path(__file__).parent) + "/"
base_folder_only = os.path.basename(os.path.normpath(base_folder_path))
_verbose = False
_is_release_build = False
_scu_folders = set()


def clear_out_existing_files(output_folder, extension):
    output_folder = os.path.abspath(output_folder)
    # print("clear_out_existing_files from folder: " + output_folder)

    if not os.path.isdir(output_folder):
        # folder does not exist or has not been created yet,
        # no files to clearout. (this is not an error)
        return

    os.chdir(output_folder)

    for file in glob.glob("*." + extension):
        # print("removed pre-existing file: " + file)
        os.remove(file)


def folder_not_found(folder):
    abs_folder = base_folder_path + folder + "/"
    return not os.path.isdir(abs_folder)


def find_files_in_folder(folder, sub_folder, include_list, extension, sought_exceptions, found_exceptions):
    abs_folder = base_folder_path + folder + "/" + sub_folder

    if not os.path.isdir(abs_folder):
        print("ERROR " + abs_folder + " not found.")
        return include_list, found_exceptions

    os.chdir(abs_folder)

    sub_folder_slashed = ""
    if sub_folder != "":
        sub_folder_slashed = sub_folder + "/"

    for file in glob.glob("*." + extension):

        simple_name = Path(file).stem

        if file.endswith(".gen.cpp"):
            continue

        li = '#include "' + folder + "/" + sub_folder_slashed + file + '"'

        if not simple_name in sought_exceptions:
            include_list.append(li)
        else:
            found_exceptions.append(li)

    return include_list, found_exceptions


def write_output_file(file_count, include_list, start_line, end_line, output_folder, output_filename_prefix, extension):

    output_folder = os.path.abspath(output_folder)

    if not os.path.isdir(output_folder):
        # create
        os.mkdir(output_folder)
        if not os.path.isdir(output_folder):
            print("ERROR " + output_folder + " could not be created.")
            return
        print("CREATING folder " + output_folder)

    file_text = ""

    for l in range(start_line, end_line):
        if l < len(include_list):
            line = include_list[l]
            li = line + "\n"
            file_text += li

    num_string = ""
    if file_count > 0:
        num_string = "_" + str(file_count)

    short_filename = output_filename_prefix + num_string + ".gen." + extension
    output_filename = output_folder + "/" + short_filename
    if _verbose:
        print("generating: " + short_filename)

    output_path = Path(output_filename)
    output_path.write_text(file_text, encoding="utf8")


def write_exception_output_file(file_count, exception_string, output_folder, output_filename_prefix, extension):
    output_folder = os.path.abspath(output_folder)
    if not os.path.isdir(output_folder):
        print("ERROR " + output_folder + " does not exist.")
        return

    file_text = exception_string + "\n"

    num_string = ""
    if file_count > 0:
        num_string = "_" + str(file_count)

    short_filename = output_filename_prefix + "_exception" + num_string + ".gen." + extension
    output_filename = output_folder + "/" + short_filename

    if _verbose:
        print("generating: " + short_filename)

    output_path = Path(output_filename)
    output_path.write_text(file_text, encoding="utf8")


def find_section_name(sub_folder):
    # Construct a useful name for the section from the path for debug logging
    section_path = os.path.abspath(base_folder_path + sub_folder) + "/"

    folders = []
    folder = ""

    for i in range(8):
        folder = os.path.dirname(section_path)
        folder = os.path.basename(folder)
        if folder == base_folder_only:
            break
        folders.append(folder)
        section_path += "../"
        section_path = os.path.abspath(section_path) + "/"

    section_name = ""
    for n in range(len(folders)):
        section_name += folders[len(folders) - n - 1]
        if n != (len(folders) - 1):
            section_name += "_"

    return section_name


# "folders" is a list of folders to add all the files from to add to the SCU
# "section (like a module)". The name of the scu file will be derived from the first folder
# (thus e.g. scene/3d becomes scu_scene_3d.gen.cpp)

# "includes_per_scu" limits the number of includes in a single scu file.
# This allows the module to be built in several translation units instead of just 1.
# This will usually be slower to compile but will use less memory per compiler instance, which
# is most relevant in release builds.

# "sought_exceptions" are a list of files (without extension) that contain
# e.g. naming conflicts, and are therefore not suitable for the scu build.
# These will automatically be placed in their own separate scu file,
# which is slow like a normal build, but prevents the naming conflicts.
# Ideally in these situations, the source code should be changed to prevent naming conflicts.

# "extension" will usually be cpp, but can also be set to c (for e.g. third party libraries that use c)
def process_folder(folders, sought_exceptions=[], includes_per_scu=0, extension_list="cpp"):
    if len(folders) == 0:
        return

    extensions = extension_list.split(" ")
    extension = extensions[0]

    # Construct the filename prefix from the FIRST folder name
    # e.g. "scene_3d"
    out_filename = find_section_name(folders[0])

    found_includes = []
    found_exceptions = []

    main_folder = folders[0]
    abs_main_folder = base_folder_path + main_folder

    # Keep a record of all folders that have been processed for SCU,
    # this enables deciding what to do when we call "add_source_files()"
    global _scu_folders
    _scu_folders.add(main_folder)

    # main folder (first)
    for ext in extensions:
        found_includes, found_exceptions = find_files_in_folder(
            main_folder, "", found_includes, ext, sought_exceptions, found_exceptions
        )

        # sub folders
        for d in range(1, len(folders)):
            found_includes, found_exceptions = find_files_in_folder(
                main_folder, folders[d], found_includes, ext, sought_exceptions, found_exceptions
            )

    found_includes = sorted(found_includes)

    # calculate how many lines to write in each file
    total_lines = len(found_includes)

    # adjust number of output files according to whether DEV or release
    num_output_files = 1
    #if _is_release_build:
    #    # always have a maximum in release
    #    includes_per_scu = 8
    #    num_output_files = max(math.ceil(total_lines / includes_per_scu), 1)
    #else:
    #    if includes_per_scu > 0:
    #        num_output_files = max(math.ceil(total_lines / includes_per_scu), 1)

    # error condition
    if total_lines == 0:
        return

    lines_per_file = math.ceil(total_lines / num_output_files)
    lines_per_file = max(lines_per_file, 1)

    start_line = 0
    file_number = 0

    # These do not vary throughout the loop
    output_folder = abs_main_folder + "/.scu/"
    output_filename_prefix = "scu_" + out_filename

    # Clear out any existing files (usually we will be overwriting,
    # but we want to remove any that are pre-existing that will not be
    # overwritten, so as to not compile anything stale)
    clear_out_existing_files(output_folder, extension)

    for file_count in range(0, num_output_files):
        end_line = start_line + lines_per_file

        # special case to cover rounding error in final file
        if file_count == (num_output_files - 1):
            end_line = len(found_includes)

        write_output_file(
            file_count, found_includes, start_line, end_line, output_folder, output_filename_prefix, extension
        )

        start_line = end_line

    # Write the exceptions each in their own scu gen file,
    # so they can effectively compile in "old style / normal build".
    for exception_count in range(len(found_exceptions)):
        write_exception_output_file(
            exception_count, found_exceptions[exception_count], output_folder, output_filename_prefix, extension
        )


def generate_scu_files(verbose, is_release_build, env):

    print("=============================")
    print("Single Compilation Unit Build")
    print("=============================")
    print("Generating SCU build files")
    global _verbose
    _verbose = verbose
    global _is_release_build
    _is_release_build = is_release_build

    curr_folder = os.path.abspath("./")

    # check we are running from the correct folder
    if folder_not_found("core") or folder_not_found("platform") or folder_not_found("scene"):
        raise RuntimeError("scu_builders.py must be run from the godot folder.")
        return

    #process_folder(["core"])
    #process_folder(["core/bind"])
    #process_folder(["core/config"])
    #process_folder(["core/containers"])
    #process_folder(["core/error"])
    #process_folder(["core/input"])
    #process_folder(["core/log"])
    #process_folder(["core/math"])
    #process_folder(["core/object"])
    #process_folder(["core/os"])
    #process_folder(["core/string"])
    #process_folder(["core/variant"])
    #process_folder(["core/io"])
    #process_folder(["core/crypto"])

    has_mbedtyls_module = False
    
    try:
        has_mbedtyls_module = env["module_mbedtls_enabled"]
    except:
        pass

    if has_mbedtyls_module:
        process_folder(["core", "bind", "config", "containers", "error", "input", "log", "math", "object",
                        "os", "string", "variant", "io", "crypto", "thirdparty/misc", 
                        "thirdparty/zlib"])
    else:
        process_folder(["core", "bind", "config", "containers", "error", "input", "log", "math", "object",
                        "os", "string", "variant", "io", "crypto", "crypto/mbedtls/library", "thirdparty/misc", 
                        "thirdparty/zlib"], [], 0, "cpp")

        process_folder(["core", "bind", "config", "containers", "error", "input", "log", "math", "object",
                        "os", "string", "variant", "io", "crypto", "crypto/mbedtls/library", "thirdparty/misc", 
                        "thirdparty/zlib"], [], 0, "c")

    #process_folder(["drivers/gles2"], [], 0, "cpp c")
    #process_folder(["drivers/unix"], [], 0, "cpp c")
    #process_folder(["drivers/png"], [], 0, "cpp c")

    process_folder(["drivers","png", "png/libpng"], [], 0, "cpp")

    process_folder([ "drivers", "png", "png/libpng" ], [], 0, "c")


    #process_folder(["drivers", "alsa", "coreaudio", "dummy", "gl_context", "gl_context/glad", "gles_common", "gles2",
    #                "png", "png/libpng", "pulseaudio", "unix", "wasapi", "windows", "xaudio2"], [], 0, "cpp")

    #process_folder(["drivers", "alsa", "coreaudio", "dummy", "gl_context", "gl_context/glad", "gles_common", "gles2",
    #                "png", "png/libpng", "pulseaudio", "unix", "wasapi", "windows", "xaudio2"], [], 0, "c")

    #process_folder(["drivers", "alsa", "coreaudio", "dummy", "gl_context", "gles2", "gles_common", "png", "pulseaudio", "unix", 
    #                    "wasapi", "windows", "xaudio2" ], [], 0, "cpp c")

    process_folder(["main"])

    process_folder(
        [
            "platform",
            #"android/export",
            #"iphone/export",
            #"javascript/export",
            #"osx/export",
            #"uwp/export",
            #"windows/export",
            #"x11/export",
        ]
    )

    #process_folder(["platform/x11"])
    #process_folder(["platform/windows"])
    #process_folder(["platform/server"])
    #process_folder(["platform/osx"])

    #TODO These should be moved to module conpig.py s

    #process_folder(["modules/bmp"])

    process_folder(["modules/bmp"])

    process_folder(["modules/gdscript"])

    process_folder(["modules/freetype", "brotli/common", "brotli/dec", "freetype/src/autofit", "freetype/src/base", 
                    "freetype/src/bdf", "freetype/src/bzip2", "freetype/src/cache", "freetype/src/cff", "freetype/src/cid", "freetype/src/gxvalid",
                    "freetype/src/gzip", "freetype/src/lzw", "freetype/src/otvalid", "freetype/src/pcf",
                    "freetype/src/pfr", "freetype/src/psaux", "freetype/src/pshinter",
                    "freetype/src/psnames", "freetype/src/raster", "freetype/src/sdf", "freetype/src/svg", "freetype/src/smooth", "freetype/src/truetype",
                    "freetype/src/type1", "freetype/src/type42", "freetype/src/winfonts"], [], 0, "cpp")

    process_folder(["modules/freetype", "brotli/common", "brotli/dec", "freetype/src/autofit", "freetype/src/base", 
                    "freetype/src/bdf", "freetype/src/bzip2", "freetype/src/cache", "freetype/src/cff", "freetype/src/cid", "freetype/src/gxvalid",
                    "freetype/src/gzip", "freetype/src/lzw", "freetype/src/otvalid", "freetype/src/pcf",
                    "freetype/src/pfr", "freetype/src/psaux", "freetype/src/pshinter",
                    "freetype/src/psnames", "freetype/src/raster", "freetype/src/sdf", "freetype/src/svg", "freetype/src/smooth", "freetype/src/truetype",
                    "freetype/src/type1", "freetype/src/type42", "freetype/src/winfonts"], [], 0, "c")

    process_folder(["modules/enet", "enet" ], [], 0, "c")
    process_folder(["modules/enet", "enet" ], [], 0, "c")
    #process_folder(["modules/enet" ], [], 0, "cpp c")
    #process_folder(["modules/enet/enet" ], [], 0, "cpp c")

    process_folder(["modules/minimp3"])
    process_folder(["modules/opensimplex", "thirdparty"])
    process_folder(["modules/gdscript"])
    process_folder(["modules/stb_vorbis", "stb_vorbis"])

    #process_folder(["modules/mbedtls"])

    #process_folder(["modules/regex"])
    #process_folder(["modules/regex", "pcre2/src", "pcre2/src/sljit" ], [], 0, "cpp c")

    #process_folder(["scene"])
    #process_folder(["scene/audio"])
    #process_folder(["scene/debugger"])
    #process_folder(["scene/2d"])
    #process_folder(["scene/animation"])
    #process_folder(["scene/gui", "resources"])
    #process_folder(["scene/main"])
    #process_folder(["scene/resources", "default_theme", "font", "material", "mesh", "shapes_2d"])

    process_folder(["scene", "audio", "2d", "animation", "gui", "gui/resources",  "main", "resources", "resources/default_theme", "resources/font", "resources/material", "resources/mesh", "resources/mesh/thirdparty", "resources/shapes_2d" ])

    #process_folder(["servers"])
    #process_folder(["servers/rendering"])
    #process_folder(["servers/physics_2d"])
    #process_folder(["servers/audio"])
    #process_folder(["servers/audio/effects"])

    process_folder(["servers", "rendering", "physics_2d", "audio", "audio/effects" ])

    # Finally change back the path to the calling folder
    os.chdir(curr_folder)

    return _scu_folders