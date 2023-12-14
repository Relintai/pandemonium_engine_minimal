#ifndef PANDEMONIUM_AUDIO_H
#define PANDEMONIUM_AUDIO_H
/*************************************************************************/
/*  pandemonium_audio.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           PANDEMONIUM ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"

extern int pandemonium_audio_is_available();
extern int pandemonium_audio_has_worklet();
extern int pandemonium_audio_has_script_processor();
extern int pandemonium_audio_init(int *p_mix_rate, int p_latency, void (*_state_cb)(int), void (*_latency_cb)(float));
extern void pandemonium_audio_resume();

extern int pandemonium_audio_capture_start();
extern void pandemonium_audio_capture_stop();

// Worklet
typedef int32_t PandemoniumAudioState[4];
extern int pandemonium_audio_worklet_create(int p_channels);
extern void pandemonium_audio_worklet_start(float *p_in_buf, int p_in_size, float *p_out_buf, int p_out_size, PandemoniumAudioState p_state);
extern void pandemonium_audio_worklet_start_no_threads(float *p_out_buf, int p_out_size, void (*p_out_cb)(int p_pos, int p_frames), float *p_in_buf, int p_in_size, void (*p_in_cb)(int p_pos, int p_frames));
extern int pandemonium_audio_worklet_state_add(PandemoniumAudioState p_state, int p_idx, int p_value);
extern int pandemonium_audio_worklet_state_get(PandemoniumAudioState p_state, int p_idx);
extern int pandemonium_audio_worklet_state_wait(int32_t *p_state, int p_idx, int32_t p_expected, int p_timeout);

// Script
extern int pandemonium_audio_script_create(int *p_buffer_size, int p_channels);
extern void pandemonium_audio_script_start(float *p_in_buf, int p_in_size, float *p_out_buf, int p_out_size, void (*p_cb)());

#ifdef __cplusplus
}
#endif

#endif /* PANDEMONIUM_AUDIO_H */
