#ifndef AUDIO_EFFECT_LIMITER_H
#define AUDIO_EFFECT_LIMITER_H

/*  audio_effect_limiter.h                                               */


#include "servers/audio/audio_effect.h"

class AudioEffectLimiter;

class AudioEffectLimiterInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectLimiterInstance, AudioEffectInstance);
	friend class AudioEffectLimiter;
	Ref<AudioEffectLimiter> base;

	float mix_volume_db;

public:
	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count);
};

class AudioEffectLimiter : public AudioEffect {
	GDCLASS(AudioEffectLimiter, AudioEffect);

	friend class AudioEffectLimiterInstance;
	float threshold;
	float ceiling;
	float soft_clip;
	float soft_clip_ratio;

protected:
	static void _bind_methods();

public:
	void set_threshold_db(float p_threshold);
	float get_threshold_db() const;

	void set_ceiling_db(float p_ceiling);
	float get_ceiling_db() const;

	void set_soft_clip_db(float p_soft_clip);
	float get_soft_clip_db() const;

	void set_soft_clip_ratio(float p_soft_clip);
	float get_soft_clip_ratio() const;

	Ref<AudioEffectInstance> instance();

	AudioEffectLimiter();
};

#endif // AUDIO_EFFECT_LIMITER_H
