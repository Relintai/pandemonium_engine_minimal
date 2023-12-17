
/*  audio_driver_dummy.cpp                                               */


#include "audio_driver_dummy.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"

Error AudioDriverDummy::init() {
	active.clear();
	exit_thread.clear();
	samples_in = nullptr;

	mix_rate = GLOBAL_GET("audio/mix_rate");
	speaker_mode = SPEAKER_MODE_STEREO;
	channels = 2;

	int latency = GLOBAL_GET("audio/output_latency");
	buffer_frames = closest_power_of_2(latency * mix_rate / 1000);

	samples_in = memnew_arr(int32_t, buffer_frames * channels);

	thread.start(AudioDriverDummy::thread_func, this);

	return OK;
};

void AudioDriverDummy::thread_func(void *p_udata) {
	AudioDriverDummy *ad = (AudioDriverDummy *)p_udata;

	uint64_t usdelay = (ad->buffer_frames / float(ad->mix_rate)) * 1000000;

	while (!ad->exit_thread.is_set()) {
		if (ad->active.is_set()) {
			ad->lock();
			ad->start_counting_ticks();

			ad->audio_server_process(ad->buffer_frames, ad->samples_in);

			ad->stop_counting_ticks();
			ad->unlock();
		};

		OS::get_singleton()->delay_usec(usdelay);
	};
};

void AudioDriverDummy::start() {
	active.set();
};

int AudioDriverDummy::get_mix_rate() const {
	return mix_rate;
};

AudioDriver::SpeakerMode AudioDriverDummy::get_speaker_mode() const {
	return speaker_mode;
};

void AudioDriverDummy::lock() {
	mutex.lock();
};

void AudioDriverDummy::unlock() {
	mutex.unlock();
};

void AudioDriverDummy::finish() {
	exit_thread.set();
	thread.wait_to_finish();

	if (samples_in) {
		memdelete_arr(samples_in);
		samples_in = nullptr;
	};
};

AudioDriverDummy::AudioDriverDummy(){

};

AudioDriverDummy::~AudioDriverDummy(){

};
