#ifndef PANDEMONIUM_MINIMAL_H
#define PANDEMONIUM_MINIMAL_H

// https://github.com/Relintai/pandemonium_engine_minimal

/*
Copyright (c) 2022-present Péter Magyar.
Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).
Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <atomic>
#include <functional>
#include <type_traits>
#include <string.h>
#include <typeinfo>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fcntl.h>
#include <assert.h>
#include <vector>
#include <list>
#include <set>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <ostream>
#include <queue>
//#include <pkcs11-helper-1.0/pkcs11h-certificate.h>
#include <time.h>
//#include <sal.h>
#include <pthread.h>


{{FILE:core/typedefs.h}}
{{FILE:core/int_types.h}}

{{FILE:core/version.h}}
{{FILE:core/license.gen.h}}

{{FILE:core/authors.gen.h}}
{{FILE:core/donors.gen.h}}
{{FILE:core/version_generated.gen.h}}

{{FILE:core/global_constants.h}}
{{FILE:core/error/error_macros.h}}
{{FILE:core/error/error_list.h}}


{{FILE:core/math/math_defs.h}}
{{FILE:core/math/math_funcs.h}}

{{FILE:core/math/vector2.h}}
{{FILE:core/math/vector2i.h}}
{{FILE:core/math/vector3.h}}
{{FILE:core/math/projection.h}}
{{FILE:core/math/aabb.h}}
{{FILE:core/math/vector3i.h}}
{{FILE:core/math/vector4i.h}}
{{FILE:core/math/vector4.h}}
{{FILE:core/math/rect2.h}}
{{FILE:core/math/rect2i.h}}
{{FILE:core/math/face3.h}}
{{FILE:core/math/transform_2d.h}}
{{FILE:core/math/quaternion.h}}
{{FILE:core/math/color.h}}
{{FILE:core/math/plane.h}}
{{FILE:core/math/basis.h}}

{{FILE:core/math/transform.h}}

{{FILE:core/math/random_pcg.h}}
{{FILE:core/math/random_number_generator.h}}
{{FILE:core/math/bsp_tree.h}}
{{FILE:core/math/bvh.h}}
{{FILE:core/math/expression.h}}
{{FILE:core/math/quick_hull.h}}
{{FILE:core/math/convex_hull.h}}
{{FILE:core/math/triangle_mesh.h}}
{{FILE:core/math/geometry.h}}
{{FILE:core/math/delaunay.h}}
{{FILE:core/math/a_star.h}}
{{FILE:core/math/disjoint_set.h}}
{{FILE:core/math/audio_frame.h}}
{{FILE:core/math/bvh_tree.h}}
{{FILE:core/math/octree.h}}
{{FILE:core/math/math_fieldwise.h}}
{{FILE:core/math/triangulate.h}}
{{FILE:core/math/transform_interpolator.h}}
{{FILE:core/math/bvh_abb.h}}

{{FILE:core/log/logger_backend.h}}
{{FILE:core/log/logger.h}}

{{FILE:core/containers/sort_array.h}}
{{FILE:core/containers/lru.h}}
{{FILE:core/containers/paged_array.h}}
{{FILE:core/containers/safe_list.h}}
{{FILE:core/containers/vmap.h}}
{{FILE:core/containers/rb_map.h}}
{{FILE:core/containers/pooled_list.h}}
{{FILE:core/containers/bin_sorted_array.h}}
{{FILE:core/containers/ring_buffer.h}}
{{FILE:core/containers/vector.h}}
{{FILE:core/containers/hash_map.h}}
{{FILE:core/containers/search_array.h}}
{{FILE:core/containers/command_queue_mt.h}}
{{FILE:core/containers/pool_vector.h}}
{{FILE:core/containers/fixed_array.h}}
{{FILE:core/containers/paged_allocator.h}}
{{FILE:core/containers/list.h}}
{{FILE:core/containers/cowdata.h}}
{{FILE:core/containers/self_list.h}}
{{FILE:core/containers/hashfuncs.h}}
{{FILE:core/containers/rid_handle.h}}
{{FILE:core/containers/og_hash_map.h}}
{{FILE:core/containers/vset.h}}
{{FILE:core/containers/pair.h}}
{{FILE:core/containers/threaded_callable_queue.h}}
{{FILE:core/containers/tight_local_vector.h}}
{{FILE:core/containers/hash_set.h}}
{{FILE:core/containers/bitfield_dynamic.h}}
{{FILE:core/containers/oa_hash_map.h}}
{{FILE:core/containers/rb_set.h}}
{{FILE:core/containers/rid.h}}
{{FILE:core/containers/local_vector.h}}
{{FILE:core/containers/ordered_hash_map.h}}
{{FILE:core/containers/simple_type.h}}
{{FILE:core/containers/packed_data_container.h}}

{{FILE:core/string/node_path.h}}
{{FILE:core/string/ucaps.h}}
{{FILE:core/string/translation.h}}
{{FILE:core/string/string_buffer.h}}
{{FILE:core/string/print_string.h}}
{{FILE:core/string/string_name.h}}
{{FILE:core/string/char_utils.h}}
{{FILE:core/string/ustring.h}}
{{FILE:core/string/string_builder.h}}

{{FILE:core/core_string_names.h}}

{{FILE:core/object/object.h}}
{{FILE:core/object/class_db.h}}
{{FILE:core/object/script_language.h}}
{{FILE:core/object/ref_ptr.h}}
{{FILE:core/object/method_bind.h}}
{{FILE:core/object/message_queue.h}}
{{FILE:core/object/script_debugger_local.h}}
{{FILE:core/object/func_ref.h}}
{{FILE:core/object/resource.h}}
{{FILE:core/object/object_id.h}}
{{FILE:core/object/object_rc.h}}
{{FILE:core/object/undo_redo.h}}
{{FILE:core/object/reference.h}}

{{FILE:core/config/engine.h}}
{{FILE:core/config/project_settings.h}}

{{FILE:core/variant/variant.h}}
{{FILE:core/variant/dictionary.h}}
{{FILE:core/variant/method_ptrcall.h}}
{{FILE:core/variant/variant_parser.h}}
{{FILE:core/variant/type_info.h}}
{{FILE:core/variant/array.h}}

{{FILE:core/input/shortcut.h}}
{{FILE:core/input/input_event.h}}
{{FILE:core/input/input.h}}
{{FILE:core/input/default_controller_mappings.h}}
{{FILE:core/input/input_map.h}}

{{FILE:core/os/os.h}}
{{FILE:core/os/memory.h}}
{{FILE:core/os/thread_pool.h}}
{{FILE:core/os/safe_refcount.h}}
{{FILE:core/os/thread_work_pool.h}}
{{FILE:core/os/main_loop.h}}
{{FILE:core/os/thread_safe.h}}
{{FILE:core/os/dir_access.h}}
{{FILE:core/os/rw_lock.h}}
{{FILE:core/os/mutex.h}}
{{FILE:core/os/pool_allocator.h}}
{{FILE:core/os/time.h}}
{{FILE:core/os/thread_pool_execute_job.h}}
{{FILE:core/os/threaded_array_processor.h}}
{{FILE:core/os/thread_pool_job.h}}
{{FILE:core/os/spin_lock.h}}
{{FILE:core/os/thread.h}}
{{FILE:core/os/keyboard.h}}
{{FILE:core/os/file_access.h}}
{{FILE:core/os/sub_process.h}}
{{FILE:core/os/semaphore.h}}
{{FILE:core/os/midi_driver.h}}

{{FILE:core/thirdparty/misc/triangulator.h}}
{{FILE:core/thirdparty/misc/pcg.h}}
{{FILE:core/thirdparty/misc/hq2x.h}}
{{FILE:core/thirdparty/misc/fastlz.h}}
{{FILE:core/thirdparty/zlib/zlib.h}}
{{FILE:core/thirdparty/zlib/crc32.h}}
{{FILE:core/thirdparty/zlib/inftrees.h}}
{{FILE:core/thirdparty/zlib/deflate.h}}
{{FILE:core/thirdparty/zlib/inffast.h}}
{{FILE:core/thirdparty/zlib/trees.h}}
{{FILE:core/thirdparty/zlib/inflate.h}}
{{FILE:core/thirdparty/zlib/zconf.h}}
{{FILE:core/thirdparty/zlib/zutil.h}}
{{FILE:core/thirdparty/zlib/inffixed.h}}
{{FILE:core/thirdparty/zlib/gzguts.h}}
{{FILE:core/thirdparty/stb_rect_pack/stb_rect_pack.h}}
{{FILE:core/thirdparty/misc/clipper.hpp}}

{{FILE:core/io/multiplayer_api.h}}
{{FILE:core/io/ip_address.h}}
{{FILE:core/io/net_socket.h}}
{{FILE:core/io/image.h}}
{{FILE:core/io/networked_multiplayer_peer.h}}
{{FILE:core/io/networked_multiplayer_custom.h}}
{{FILE:core/io/xml_parser.h}}
{{FILE:core/io/tcp_server.h}}
{{FILE:core/io/dtls_server.h}}
{{FILE:core/io/udp_server.h}}
{{FILE:core/io/packet_peer_udp.h}}
{{FILE:core/io/packet_peer_dtls.h}}
{{FILE:core/io/translation_loader_po.h}}
{{FILE:core/io/stream_peer_ssl.h}}
{{FILE:core/io/packet_peer.h}}
{{FILE:core/io/json.h}}
{{FILE:core/io/resource_saver.h}}
{{FILE:core/io/file_access_encrypted.h}}
{{FILE:core/io/resource_loader.h}}
{{FILE:core/io/file_access_network.h}}
{{FILE:core/io/resource_format_binary.h}}
{{FILE:core/io/stream_peer_tcp.h}}
{{FILE:core/io/marshalls.h}}
{{FILE:core/io/logger.h}}
{{FILE:core/io/http_client.h}}
{{FILE:core/io/ip.h}}
{{FILE:core/io/config_file.h}}
{{FILE:core/io/stream_peer.h}}
{{FILE:core/io/file_access_memory.h}}
{{FILE:core/io/image_loader.h}}
{{FILE:core/io/resource_importer.h}}

{{FILE:core/crypto/mbedtls/include/pandemonium_core_mbedtls_config.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ecdsa.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/pkcs12.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/poly1305.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ssl_ticket.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/xtea.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/chacha20.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/error.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ssl_cache.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/hkdf.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/md4.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ecdh.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/padlock.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/entropy.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/asn1write.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/x509_csr.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ecp.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/md5.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/entropy_poll.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ssl_cookie.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/x509.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/gcm.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/rsa_internal.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/net_sockets.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/des.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/check_config.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ssl_internal.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/pkcs11.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/aria.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/pk_internal.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/cipher.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/nist_kw.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/timing.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ripemd160.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/sha512.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ctr_drbg.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ccm.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/dhm.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/md_internal.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/sha256.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/hmac_drbg.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ecp_internal.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/pem.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/pkcs5.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/platform_util.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/bn_mul.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/rsa.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/md.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/threading.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/pk.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/platform.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/blowfish.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/havege.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/aesni.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/memory_buffer_alloc.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ecjpake.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/aes.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/base64.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/debug.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/platform_time.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/net.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/x509_crt.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/compat-1.3.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/asn1.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/sha1.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/camellia.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/version.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/oid.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/arc4.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/chachapoly.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ssl.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/ssl_ciphersuites.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/x509_crl.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/md2.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/constant_time.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/cipher_internal.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/certs.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/cmac.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/config.h}}
{{FILE:core/crypto/mbedtls/include/mbedtls/bignum.h}}
{{FILE:core/crypto/mbedtls/library/mps_trace.h}}
{{FILE:core/crypto/mbedtls/library/mps_error.h}}
{{FILE:core/crypto/mbedtls/library/constant_time_invasive.h}}
{{FILE:core/crypto/mbedtls/library/common.h}}
{{FILE:core/crypto/mbedtls/library/ssl_tls13_keys.h}}
{{FILE:core/crypto/mbedtls/library/mps_reader.h}}
{{FILE:core/crypto/mbedtls/library/check_crypto_config.h}}
{{FILE:core/crypto/mbedtls/library/mps_common.h}}
{{FILE:core/crypto/mbedtls/library/constant_time_internal.h}}
{{FILE:core/crypto/mbedtls/library/ecp_invasive.h}}

{{FILE:core/crypto/hashing_context.h}}
{{FILE:core/crypto/crypto.h}}
{{FILE:core/crypto/crypto_core.h}}
{{FILE:core/crypto/aes_context.h}}

{{FILE:core/bind/core_bind.h}}
{{FILE:core/bind/logger_bind.h}}
{{FILE:core/register_core_types.h}}

#endif
