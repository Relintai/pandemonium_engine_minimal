import os, json

from SCons.Util import WhereIs


def run_closure_compiler(target, source, env, for_signature):
    closure_bin = os.path.join(os.path.dirname(WhereIs("emcc")), "node_modules", ".bin", "google-closure-compiler")
    cmd = [WhereIs("node"), closure_bin]
    cmd.extend(["--compilation_level", "ADVANCED_OPTIMIZATIONS"])
    for f in env["JSEXTERNS"]:
        cmd.extend(["--externs", f.get_abspath()])
    for f in source:
        cmd.extend(["--js", f.get_abspath()])
    cmd.extend(["--js_output_file", target[0].get_abspath()])
    return " ".join(cmd)


def get_build_version():
    import version

    name = "custom_build"
    if os.getenv("BUILD_NAME") != None:
        name = os.getenv("BUILD_NAME")
    v = "%d.%d" % (version.major, version.minor)
    if version.patch > 0:
        v += ".%d" % version.patch
    status = version.status
    if os.getenv("PANDEMONIUM_VERSION_STATUS") != None:
        status = str(os.getenv("PANDEMONIUM_VERSION_STATUS"))
    v += ".%s.%s" % (status, name)
    return v


def create_engine_file(env, target, source, externs):
    if env["use_closure_compiler"]:
        return env.BuildJS(target, source, JSEXTERNS=externs)
    return env.Textfile(target, [env.File(s) for s in source])


def create_template_zip(env, js, wasm, extra):
    binary_name = "pandemonium"
    zip_dir = env.Dir("#bin/.javascript_zip")
    in_files = [
        js,
        wasm,
        "#platform/javascript/js/libs/audio.worklet.js",
    ]
    out_files = [
        zip_dir.File(binary_name + ".js"),
        zip_dir.File(binary_name + ".wasm"),
        zip_dir.File(binary_name + ".audio.worklet.js"),
    ]
    # GDNative/Threads specific
    if env["threads_enabled"]:
        in_files.append(extra)  # Worker
        out_files.append(zip_dir.File(binary_name + ".worker.js"))

    service_worker = "#misc/dist/html/service-worker.js"

    # HTML
    in_files.append("#misc/dist/html/full-size.html")
    out_files.append(zip_dir.File(binary_name + ".html"))
    in_files.append(service_worker)
    out_files.append(zip_dir.File(binary_name + ".service.worker.js"))
    in_files.append("#misc/dist/html/offline-export.html")
    out_files.append(zip_dir.File("pandemonium.offline.html"))

    zip_files = env.InstallAs(out_files, in_files)
    env.Zip(
        "#bin/pandemonium",
        zip_files,
        ZIPROOT=zip_dir,
        ZIPSUFFIX="${PROGSUFFIX}${ZIPSUFFIX}",
        ZIPCOMSTR="Archiving $SOURCES as $TARGET",
    )


def add_js_libraries(env, libraries):
    env.Append(JS_LIBS=env.File(libraries))


def add_js_pre(env, js_pre):
    env.Append(JS_PRE=env.File(js_pre))


def add_js_externs(env, externs):
    env.Append(JS_EXTERNS=env.File(externs))
