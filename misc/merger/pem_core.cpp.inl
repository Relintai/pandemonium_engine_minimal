
#ifndef PANDEMONIUM_MINIMAL_H
#include "pem_core.h"
#endif

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <ostream>
#include <functional>
#include <math.h>

#include <limits.h>

#include <wchar.h>
#include <cstdint>

{{FILE:core/variant/variant.cpp}}
{{FILE:core/variant/variant_call.cpp}}

{{FILE:core/variant/dictionary.cpp}}
{{FILE:core/variant/array.cpp}}
{{FILE:core/variant/variant_op.cpp}}
{{FILE:core/variant/variant_parser.cpp}}

{{FILE:core/log/logger_backend.cpp}}
{{FILE:core/log/logger.cpp}}

{{FILE:core/containers/rid.cpp}}
{{FILE:core/containers/pool_vector.cpp}}
{{FILE:core/containers/rid_handle.cpp}}
{{FILE:core/containers/command_queue_mt.cpp}}
{{FILE:core/containers/packed_data_container.cpp}}
{{FILE:core/containers/bitfield_dynamic.cpp}}

{{FILE:core/register_core_types.cpp}}

{{FILE:core/input/input_event.cpp}}
{{FILE:core/input/shortcut.cpp}}
{{FILE:core/input/default_controller_mappings.gen.cpp}}
{{FILE:core/input/input_map.cpp}}
{{FILE:core/input/input.cpp}}

{{FILE:core/script_encryption_key.gen.cpp}}

{{FILE:core/error/error_macros.cpp}}

{{FILE:core/core_string_names.cpp}}

{{FILE:core/os/time.cpp}}
{{FILE:core/os/file_access.cpp}}
{{FILE:core/os/os.cpp}}
{{FILE:core/os/mutex.cpp}}
{{FILE:core/os/memory.cpp}}
{{FILE:core/os/safe_refcount.cpp}}
{{FILE:core/os/main_loop.cpp}}
{{FILE:core/os/thread_pool.cpp}}
{{FILE:core/os/thread.cpp}}
{{FILE:core/os/thread_pool_job.cpp}}
{{FILE:core/os/pool_allocator.cpp}}
{{FILE:core/os/thread_work_pool.cpp}}
{{FILE:core/os/keyboard.cpp}}
{{FILE:core/os/sub_process.cpp}}
{{FILE:core/os/dir_access.cpp}}
{{FILE:core/os/thread_pool_execute_job.cpp}}
{{FILE:core/os/midi_driver.cpp}}

{{FILE:core/thirdparty/misc/clipper.cpp}}
{{FILE:core/thirdparty/misc/pcg.cpp}}
{{FILE:core/thirdparty/misc/hq2x.cpp}}
{{FILE:core/thirdparty/misc/triangulator.cpp}}

{{FILE:core/version_hash.gen.cpp}}

{{FILE:core/io/resource_format_binary.cpp}}
{{FILE:core/io/packet_peer_udp.cpp}}
{{FILE:core/io/stream_peer_ssl.cpp}}
{{FILE:core/io/tcp_server.cpp}}
{{FILE:core/io/file_access_memory.cpp}}
{{FILE:core/io/file_access_network.cpp}}
{{FILE:core/io/image_loader.cpp}}
{{FILE:core/io/udp_server.cpp}}
{{FILE:core/io/http_client.cpp}}
{{FILE:core/io/config_file.cpp}}
{{FILE:core/io/ip_address.cpp}}
{{FILE:core/io/packet_peer_dtls.cpp}}
{{FILE:core/io/xml_parser.cpp}}
{{FILE:core/io/networked_multiplayer_custom.cpp}}
{{FILE:core/io/ip.cpp}}
{{FILE:core/io/logger.cpp}}
{{FILE:core/io/multiplayer_api.cpp}}
{{FILE:core/io/image.cpp}}
{{FILE:core/io/stream_peer_tcp.cpp}}
{{FILE:core/io/packet_peer.cpp}}
{{FILE:core/io/translation_loader_po.cpp}}
{{FILE:core/io/marshalls.cpp}}
{{FILE:core/io/json.cpp}}
{{FILE:core/io/dtls_server.cpp}}
{{FILE:core/io/networked_multiplayer_peer.cpp}}
{{FILE:core/io/resource_importer.cpp}}
{{FILE:core/io/stream_peer.cpp}}
{{FILE:core/io/resource_saver.cpp}}
{{FILE:core/io/net_socket.cpp}}
{{FILE:core/io/resource_loader.cpp}}
{{FILE:core/io/file_access_encrypted.cpp}}

{{FILE:core/config/engine.cpp}}
{{FILE:core/config/project_settings.cpp}}

{{FILE:core/string/print_string.cpp}}
{{FILE:core/string/string_name.cpp}}
{{FILE:core/string/node_path.cpp}}
{{FILE:core/string/ustring.cpp}}
{{FILE:core/string/string_builder.cpp}}
{{FILE:core/string/translation.cpp}}

{{FILE:core/math/aabb.cpp}}
{{FILE:core/math/expression.cpp}}
{{FILE:core/math/vector3i.cpp}}
{{FILE:core/math/transform_2d.cpp}}
{{FILE:core/math/projection.cpp}}
{{FILE:core/math/vector3.cpp}}
{{FILE:core/math/vector2.cpp}}
{{FILE:core/math/random_number_generator.cpp}}
{{FILE:core/math/basis.cpp}}
{{FILE:core/math/a_star.cpp}}
{{FILE:core/math/face3.cpp}}
{{FILE:core/math/triangle_mesh.cpp}}
{{FILE:core/math/vector4i.cpp}}
{{FILE:core/math/transform.cpp}}
{{FILE:core/math/geometry.cpp}}
{{FILE:core/math/convex_hull.cpp}}
{{FILE:core/math/disjoint_set.cpp}}
{{FILE:core/math/bsp_tree.cpp}}
{{FILE:core/math/transform_interpolator.cpp}}
{{FILE:core/math/color.cpp}}
{{FILE:core/math/math_fieldwise.cpp}}
{{FILE:core/math/quaternion.cpp}}
{{FILE:core/math/plane.cpp}}
{{FILE:core/math/vector2i.cpp}}
{{FILE:core/math/triangulate.cpp}}
{{FILE:core/math/rect2.cpp}}
{{FILE:core/math/audio_frame.cpp}}
{{FILE:core/math/rect2i.cpp}}
{{FILE:core/math/random_pcg.cpp}}
{{FILE:core/math/vector4.cpp}}
{{FILE:core/math/math_funcs.cpp}}
{{FILE:core/math/quick_hull.cpp}}

{{FILE:core/object/resource.cpp}}
{{FILE:core/object/script_language.cpp}}
{{FILE:core/object/reference.cpp}}
{{FILE:core/object/object.cpp}}
{{FILE:core/object/method_bind.cpp}}
{{FILE:core/object/message_queue.cpp}}
{{FILE:core/object/ref_ptr.cpp}}
{{FILE:core/object/class_db.cpp}}
{{FILE:core/object/undo_redo.cpp}}
{{FILE:core/object/script_debugger_local.cpp}}
{{FILE:core/object/func_ref.cpp}}

{{FILE:core/bind/logger_bind.cpp}}
{{FILE:core/bind/core_bind.cpp}}

{{FILE:core/global_constants.cpp}}

{{FILE:core/crypto/crypto_core.cpp}}
{{FILE:core/crypto/crypto.cpp}}
{{FILE:core/crypto/aes_context.cpp}}
{{FILE:core/crypto/hashing_context.cpp}}


{{FILE:core/thirdparty/misc/fastlz.c}}
{{FILE:core/thirdparty/zlib/inffast.c}}
{{FILE:core/thirdparty/zlib/zutil.c}}
{{FILE:core/thirdparty/zlib/infback.c}}
{{FILE:core/thirdparty/zlib/gzlib.c}}
{{FILE:core/thirdparty/zlib/compress.c}}
{{FILE:core/thirdparty/zlib/inftrees.c}}
{{FILE:core/thirdparty/zlib/gzwrite.c}}
{{FILE:core/thirdparty/zlib/uncompr.c}}
{{FILE:core/thirdparty/zlib/deflate.c}}
{{FILE:core/thirdparty/zlib/inflate.c}}
{{FILE:core/thirdparty/zlib/adler32.c}}
{{FILE:core/thirdparty/zlib/crc32.c}}
{{FILE:core/thirdparty/zlib/gzclose.c}}
{{FILE:core/thirdparty/zlib/gzread.c}}
{{FILE:core/thirdparty/zlib/trees.c}}

{{FILE:core/crypto/mbedtls/library/base64.c}}
{{FILE:core/crypto/mbedtls/library/sha256.c}}
{{FILE:core/crypto/mbedtls/library/pandemonium_core_mbedtls_platform.c}}
{{FILE:core/crypto/mbedtls/library/sha1.c}}
{{FILE:core/crypto/mbedtls/library/constant_time.c}}
{{FILE:core/crypto/mbedtls/library/aes.c}}
{{FILE:core/crypto/mbedtls/library/md5.c}}



