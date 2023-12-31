"""Functions used to generate source files during build time

All such functions are invoked in a subprocess on Windows to prevent build flakiness.

"""
import os
import os.path
import shutil
import subprocess
import tempfile
import uuid
from platform_methods import subprocess_main
from compat import encode_utf8, byte_to_str, open_utf8


def make_doc_header(target, source, env):

    dst = target[0]
    g = open_utf8(dst, "w")
    buf = ""
    docbegin = ""
    docend = ""
    for src in source:
        if not src.endswith(".xml"):
            continue
        with open_utf8(src, "r") as f:
            content = f.read()
        buf += content

    buf = encode_utf8(docbegin + buf + docend)
    decomp_size = len(buf)
    import zlib

    buf = zlib.compress(buf)

    g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    g.write("#ifndef _DOC_DATA_RAW_H\n")
    g.write("#define _DOC_DATA_RAW_H\n")
    g.write("static const int _doc_data_compressed_size = " + str(len(buf)) + ";\n")
    g.write("static const int _doc_data_uncompressed_size = " + str(decomp_size) + ";\n")
    g.write("static const unsigned char _doc_data_compressed[] = {\n")
    for i in range(len(buf)):
        g.write("\t" + byte_to_str(buf[i]) + ",\n")
    g.write("};\n")

    g.write("#endif")

    g.close()

if __name__ == "__main__":
    subprocess_main(globals())
