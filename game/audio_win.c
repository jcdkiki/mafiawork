#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

#include "audio.h"

#define N_CHANNELS 1
#define SAMPLE_SIZE_BYTES 4
#define CHUNK_SIZE (AUDIO_SAMPLE_RATE / TICKS_PER_SECOND)

typedef struct wavdata {
	audio_sample_t* sample;
	wav_header_t header;
} wavdata_t;

HWAVEOUT wave_out;
WAVEHDR header[2];
static float chunks[2][CHUNK_SIZE * N_CHANNELS];

uint8_t chunk_swap = 0;

list_t instances;
list_t delete_them;
static char errbuf[128];

/*
 * SUPER DIRTY HACK!
 * TODO: FIX IT
 */
int8_t is_memory_readable(void* ptr, size_t byteCount)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(ptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
		return 0;

	if (mbi.State != MEM_COMMIT)
		return 0;

	if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
		return 0;

	size_t blockOffset = (size_t)((char*)ptr - (char*)mbi.AllocationBase);
	size_t blockBytesPostPtr = mbi.RegionSize - blockOffset;

	if (blockBytesPostPtr < byteCount)
		return is_memory_readable((char*)ptr + blockBytesPostPtr,
			byteCount - blockBytesPostPtr);

	return 1;
}

void audio_sample_destroy(audio_sample_t* sample)
{
	sys_free(sample->data);
}

void audio_sample_read_fmt_chunk(wavdata_t* data, file_handle_t file, size_t chunk_size)
{
	sys_read_file(file, &data->header, sizeof(wav_header_t));
	sys_set_file_pos(file, chunk_size - sizeof(wav_header_t), FILEPOS_ADD);
}

void audio_sample_read_data_chunk(wavdata_t* data, file_handle_t file, size_t chunk_size)
{
	char* chunk_data = sys_malloc(chunk_size);
	size_t n_chunk_samples;
	float* out;
	char* in;
	uint32_t error = 0;

	size_t src_samples = chunk_size / data->header.bytes_per_sample;
	size_t dst_samples = (src_samples * AUDIO_SAMPLE_RATE) / data->header.sample_rate;

	data->sample->len = dst_samples;
	data->sample->data = sys_malloc(data->sample->len * N_CHANNELS * SAMPLE_SIZE_BYTES);

	out = data->sample->data;
	in = chunk_data;

	sys_read_file(file, chunk_data, chunk_size);

	for (uint32_t i = 0; i < dst_samples; i++) {
		out[i] = 0.f;
	}

	switch (data->header.type) {
		case WAVE_FORMAT_IEEE_FLOAT:
		{
			float* in_f = (float*)in;

			if (data->header.sample_rate >= AUDIO_SAMPLE_RATE) {
				uint32_t cur_dst = 0;

				for (uint32_t i = 0; i < src_samples; i++) {
					out[cur_dst] += in_f[i];

					error += AUDIO_SAMPLE_RATE;
					if (error >= data->header.sample_rate) {
						error -= data->header.sample_rate;
						cur_dst++;
					}
				}
			}
			else {
				uint32_t cur_src = 0;

				for (uint32_t i = 0; i < dst_samples - 1; i++) {
					float interpolation = (float)error / (float)AUDIO_SAMPLE_RATE;
					out[i] = in_f[cur_src] * (1 - interpolation ) + in_f[cur_src + 1] * interpolation;

					error += data->header.sample_rate;
					if (error >= AUDIO_SAMPLE_RATE) {
						error -= AUDIO_SAMPLE_RATE;
						cur_src++;
					}
				}

				out[dst_samples - 1] += in_f[cur_src];
			}
			break;
		}
		case WAVE_FORMAT_PCM:
		{
			uint32_t cur_dst = 0;
			float pcm_divide = (1 << (data->header.bits_per_sample - 1)) - 1;
			uint32_t pcm_sum = 0;
			uint32_t bytes_per_sample = data->header.bytes_per_sample;

			for (uint32_t i = 0; i < src_samples; i++) {
				uint64_t sample = 0;
				memcpy(&sample, &in[i * bytes_per_sample], bytes_per_sample);
				pcm_sum += sample;

				error += AUDIO_SAMPLE_RATE;
				if (error >= data->header.sample_rate) {
					out[cur_dst] += (float)pcm_sum / pcm_divide;
					pcm_sum = 0;
					
					error -= data->header.sample_rate;
					cur_dst++;
				}
			}

			out[cur_dst] += (float)pcm_sum / pcm_divide;
			break;
		}
		default:
			break;
	}

	sys_free(chunk_data);
}

void audio_sample_create_from_wavfile_handle(audio_sample_t* sample, file_handle_t file)
{
	typedef struct str2func {
		const char* str;
		void (*func)(wavdata_t *data, file_handle_t file, size_t chunk_size);
	} str2func_t;
	
	char strbuf[4];
	uint32_t chunk_size;
	str2func_t chunkmap[2] = {
		{ .str = "fmt ", .func = audio_sample_read_fmt_chunk },
		{ .str = "data", .func = audio_sample_read_data_chunk }
	};

	wavdata_t data = {
		.sample = sample
	};

	sys_read_file(file, strbuf, 4);			// should be RIFF but idc for now
	sys_read_file(file, strbuf, 4);			// filesize but idc
	sys_read_file(file, strbuf, 4);			// should be WAVE but idc for now

	while (sys_read_file(file, strbuf, 4)) {
		sys_read_file(file, &chunk_size, 4);

		for (uint32_t i = 0; i < 2; i++) {
			if (strncmp(chunkmap[i].str, strbuf, 4) == 0) {
				chunkmap[i].func(&data, file, chunk_size);
				goto chunk_identified;
			}
		}

		sys_set_file_pos(file, chunk_size, FILEPOS_ADD);
	chunk_identified:;
	}
}

void audio_sample_create_from_wavfile(audio_sample_t* sample, const char* filename)
{
	file_handle_t file = sys_open_file(filename, "rb");
	audio_sample_create_from_wavfile_handle(sample, file);
	sys_close_file(file);
}

void CALLBACK wave_out_proc(HWAVEOUT wave_out_handle, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
{
	MMRESULT res;

	switch (message) {
		case WOM_DONE:
		{
			float* chunk = chunks[chunk_swap];
			memset(chunk, 0, CHUNK_SIZE * N_CHANNELS * SAMPLE_SIZE_BYTES);

			for (listnode_t *node = delete_them.begin; node != NULL; node++) {
				list_destroy_node(&instances, LISTNODE_DATA(node, audio_instance_t*));
			}
			delete_them = (list_t) { 0 };

			for (listnode_t* node = instances.begin; node != NULL;) {
				audio_instance_t* inst = LISTNODE_DATAPTR(node, audio_instance_t);

				float* samples = inst->sample->data;
				if (!is_memory_readable(samples, 4)) /* SO FUCKING DIRTY AF */
					continue;

				listnode_t* next_node;
				uint32_t written = 0;
				int8_t need_to_delete = 0;

				uint32_t left = inst->start;
				uint32_t right = inst->end;
				uint32_t cur_src = inst->cur;
				float error = 0.f;
				float error_step = inst->speed;

				if (inst->speed >= 1.f) {
					for (uint32_t i = 0; i < CHUNK_SIZE; i++) {
						if ((inst->direction > 0 && cur_src >= right) || (inst->direction < 0 && cur_src < left)) {
							switch (inst->play_type) {
								case AUDIO_PLAY_ONCE:
									need_to_delete = 1;
									goto out_of_loop;
								case AUDIO_PLAY_REPEAT:
									cur_src = inst->start;
									break;
								case AUDIO_PLAY_BOUNCE:
									cur_src -= 2 * inst->direction;
									inst->direction = -inst->direction;
									break;
							}
						}

						chunk[i] += samples[cur_src] * inst->volume;

						error += error_step;
						while (error >= 1.f) {
							error -= 1.f;
							cur_src += inst->direction;
						}
					}
					inst->cur = cur_src;
				}
				else {
					for (uint32_t i = 0; i < CHUNK_SIZE; i++) {
						if ((inst->direction > 0 && cur_src >= right) || (inst->direction < 0 && cur_src < left)) {
							switch (inst->play_type) {
								case AUDIO_PLAY_ONCE:
									need_to_delete = 1;
									goto out_of_loop;
								case AUDIO_PLAY_REPEAT:
									cur_src = inst->start;
									break;
								case AUDIO_PLAY_BOUNCE:
									cur_src -= 2 * inst->direction;
									inst->direction = -inst->direction;
									break;
							}
						}

						chunk[i] += samples[cur_src] * inst->volume;
						written++;

						error += error_step;
						if (error >= 1.f) {
							error -= 1.f;
							cur_src += inst->direction;
						}
					}

					inst->cur = cur_src;
				}

			out_of_loop:
				next_node = node->next;
				if (need_to_delete) {
					list_destroy_node(&instances, node);
				}

				node = next_node;
			}

			
			if (waveOutWrite(wave_out, &header[chunk_swap], sizeof(header[chunk_swap])) != MMSYSERR_NOERROR) {
				sys_fatal_error("waveOutWrite failed");
			}
			
			chunk_swap = 1 - chunk_swap;
			break;
		}
		default:
			break;
	}
}

audio_instance_t* audio_play_sample(audio_sample_t* sample, uint32_t start, uint32_t end, float volume, float speed, AUDIO_PLAY_TYPE play_type)
{
	audio_instance_t* inst = list_emplace_back(&instances, audio_instance_t);
	
	inst->cur = start;
	inst->sample = sample;
	inst->start = start;
	inst->end = end;
	inst->play_type = play_type;
	inst->speed = speed;
	inst->direction = 1;
	inst->volume = volume;
	inst->is_interpolated = 0;
	
	return inst;
}

void audio_instance_interpolate(audio_instance_t* inst, uint32_t interpolate_point, float interpolate_speed)
{
	uint32_t interpolate_real_end = interpolate_point + (uint32_t)((1.f / interpolate_speed) * AUDIO_SAMPLE_RATE);

	inst->is_interpolated = 1;
	inst->interpolate_point = interpolate_point;
	inst->interpolate_value = 0.f;
	inst->interpolate_speed = interpolate_speed;
	inst->interpolate_volume_start = fabsf(inst->sample->data[interpolate_point] - inst->sample->data[interpolate_point - 1]);
	inst->interpolate_volume_end = fabsf(inst->sample->data[interpolate_real_end] - inst->sample->data[interpolate_real_end - 1]);
}

void audio_stop_instance(audio_instance_t* sample)
{
	*list_emplace_back(&delete_them, audio_instance_t*) = sample;
}

void audio_init()
{
	WAVEFORMATEX format = {
		.wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
		.nChannels = N_CHANNELS,
		.nSamplesPerSec = AUDIO_SAMPLE_RATE,
		.wBitsPerSample = SAMPLE_SIZE_BYTES * 8,
		.cbSize = 0,
	};

	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	if (waveOutOpen(&wave_out, WAVE_MAPPER, &format, (DWORD_PTR)wave_out_proc, (DWORD_PTR)NULL, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
		sys_fatal_error("waveOutOpen failed");
	}

	if (waveOutSetVolume(wave_out, 0xFFFFFFFF) != MMSYSERR_NOERROR) {
		sys_fatal_error("waveOutGetVolume failed");
	}

	for (int i = 0; i < 2; ++i) {
		memset(&chunks[i], 0, SAMPLE_SIZE_BYTES * N_CHANNELS);
		
		header[i].lpData = (CHAR*)chunks[i];
		header[i].dwBufferLength = CHUNK_SIZE * SAMPLE_SIZE_BYTES * N_CHANNELS;
		
		if (waveOutPrepareHeader(wave_out, &header[i], sizeof(header[i])) != MMSYSERR_NOERROR) {
			sys_fatal_error("waveOutPrepareHeader failed");
		}
		if (waveOutWrite(wave_out, &header[i], sizeof(header[i])) != MMSYSERR_NOERROR) {
			sys_fatal_error("waveOutWrite failed\n");
		}
	}
}

void audio_deinit()
{
	list_destroy(&instances);

	// TODO: proper way of closing dat shit
	/*
	res = waveOutClose(wave_out);
	if (res != MMSYSERR_NOERROR) {
		waveInGetErrorTextA(res, errbuf, 128);
		RENG_LOGF("waveOutClose: %s", errbuf);
		sys_fatal_error("waveOutClose failed");
	}
	*/
}