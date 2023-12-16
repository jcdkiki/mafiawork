#ifndef RENG_AUDIO_H
#define RENG_AUDIO_H

#include "def.h"
#include "sys.h"

#define AUDIO_SAMPLE_RATE 96000

typedef struct wav_header {
	uint16_t type;
	uint16_t n_channels;
	uint32_t sample_rate;
	uint32_t bytes_per_second; // (sample_rate * bits_per_sample * n_channels) / 8.
	uint16_t bytes_per_sample; // (bite_per_sample * channels) / 8
	uint16_t bits_per_sample;
} wav_header_t;

typedef enum {
	AUDIO_PLAY_ONCE,
	AUDIO_PLAY_REPEAT,
	AUDIO_PLAY_BOUNCE
} AUDIO_PLAY_TYPE;

typedef struct audio_sample {
	uint32_t len;
	char* data;
} audio_sample_t;

typedef struct audio_instance {
	audio_sample_t *sample;
	
	AUDIO_PLAY_TYPE play_type;
	float volume;
	float speed;
	
	uint32_t start;
	uint32_t cur;
	uint32_t end;
	int8_t direction;
	
	int8_t is_interpolated;
	float interpolate_value;
	float interpolate_speed;
	uint32_t interpolate_point;
	float interpolate_volume_start;
	float interpolate_volume_end;
} audio_instance_t;

void				audio_sample_create_from_wavfile(audio_sample_t* sample, const char *filename);
void				audio_sample_create_from_wavfile_handle(audio_sample_t* sample, file_handle_t file);
void				audio_sample_destroy(audio_sample_t* sample);
void				audio_init();
audio_instance_t*	audio_play_sample(audio_sample_t* sample, uint32_t start, uint32_t end, float volume, float speed, AUDIO_PLAY_TYPE play_type);
void				audio_stop_instance(audio_instance_t* sample);
void				audio_instance_interpolate(audio_instance_t* inst, uint32_t interpolate_point, float interpolate_speed);
void				audio_deinit();

#endif