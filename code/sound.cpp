typedef u32 Sound_Handle;

struct Sound {
	u32 num_frames;
	//u16 bytes_per_frame;
	char *samples;
};

Array<Sound> sound_data = make_array<Sound>(256, 0); // @TEMP

void
sound_load(const char *path, Memory_Arena *ma)
{
	String wav_file_string = read_entire_file(path, ma);
	const char *wav_file = wav_file_string.data;

	wav_file += 4;

	u32 wav_file_length = READ_AND_ADVANCE_STREAM(u32, wav_file);

	wav_file += 8;

	u32 format_data_length = READ_AND_ADVANCE_STREAM(u32, wav_file);
	u16 format_type        = READ_AND_ADVANCE_STREAM(u16, wav_file);
	u16 num_channels       = READ_AND_ADVANCE_STREAM(u16, wav_file);
	u32 sample_rate        = READ_AND_ADVANCE_STREAM(u32, wav_file);
	u32 bytes_per_second   = READ_AND_ADVANCE_STREAM(u32, wav_file);
	u16 bytes_per_frame    = READ_AND_ADVANCE_STREAM(u16, wav_file);
	u16 bits_per_sample    = READ_AND_ADVANCE_STREAM(u16, wav_file);

	char chunk_name[5] = {};
	strncpy(chunk_name, wav_file, 4);

	while (strcmp(chunk_name, "data") != 0) {
		wav_file += 4;
		u32 chunk_data_length = READ_AND_ADVANCE_STREAM(u32, wav_file);
		wav_file += chunk_data_length;

		strncpy(chunk_name, wav_file, 4);
	}

	wav_file += 4;

	u32 sample_data_length = READ_AND_ADVANCE_STREAM(u32, wav_file);

	printf("%u\n%u\n%u\n%u\n%u\n%u\n%u\n%u\n", format_data_length, format_type, num_channels, sample_rate, bytes_per_second, bytes_per_frame, bits_per_sample, sample_data_length); 

	assert((num_channels == 1 || num_channels == 2) && num_channels <= pcm_playback_info.num_channels);

	Sound s;
	s.num_frames = sample_data_length / bytes_per_frame;
	//s.bytes_per_frame = bytes_per_frame;

	if (num_channels == pcm_playback_info.num_channels) {
		s.samples = (char *)malloc(sample_data_length);
		memcpy(s.samples, wav_file, sample_data_length);
	} else if (num_channels == 1 && pcm_playback_info.num_channels == 2) {
		s.samples = (char *)malloc(sample_data_length * 2);

		s16 *stored_samples = (s16 *)s.samples;
		s16 *file_samples   = (s16 *)wav_file;

		for (u32 i = 0; i < s.num_frames; ++i) {
			stored_samples[i * 2]     = *file_samples;
			stored_samples[i * 2 + 1] = *file_samples;
			++file_samples;
		}
	} else {
		_abort("The wav file '%s' has %u channels, while the PCM has %u channels. This is unexpected and the sound code will probably not work.", path, num_channels, pcm_playback_info.num_channels);
	}

	sound_data.push(s);
}

void
sound_do()
{
	Sound fur_elise = sound_data[0];
	Sound speech = sound_data[1];

	static u32 frames_written = 0;
	static char *fe_samples = fur_elise.samples;
	//static s16 *sp_samples = (s16 *)speech.samples;
	static char *sp_samples = speech.samples;
	static u32 ii = 0;

	s16 period_buffer[pcm_playback_info.bytes_per_period];

	//if (frames_written < fur_elise.num_frames && platform_pcm_less_than_one_period_left_in_buffer()) {
	if (frames_written < fur_elise.num_frames && platform_pcm_less_than_one_period_left_in_buffer()) {
		u32 num_frames_to_write = u32_min(pcm_playback_info.frames_per_period, speech.num_frames - frames_written);
		u32 num_bytes_to_write = num_frames_to_write * pcm_playback_info.bytes_per_frame;
		printf("%u %u\n", num_bytes_to_write, pcm_playback_info.bytes_per_period);
		memcpy(period_buffer, fe_samples, pcm_playback_info.bytes_per_period);

		//if (frames_written > fur_elise.num_frames / 30) {
		s16 *this_sp = (s16 *)sp_samples;
		for (u32 i = 0; i < pcm_playback_info.frames_per_period * 2 && ii < speech.num_frames * 2; ++i) {
			period_buffer[i] += *this_sp++;
			++ii;
		}
		//}

		s32 frames_written_this_call = platform_pcm_write_period(period_buffer);
		if (frames_written_this_call > 0) {
			fe_samples += frames_written_this_call * pcm_playback_info.bytes_per_frame;
		//if (frames_written > fur_elise.num_frames / 30) {
			sp_samples += frames_written_this_call * pcm_playback_info.bytes_per_frame;
		//}
			frames_written += frames_written_this_call;
		}

		if (frames_written == fur_elise.num_frames) {
			platform_pcm_close_device();
		}
#if 0
		//s32 frames_written_this_call = snd_pcm_writei(linux_context.pcm_handle, samples, pcm_playback_info.frames_per_period);

		if (frames_written_this_call == -EPIPE) {
			log_print(MINOR_ERROR_LOG, "PCM underrun occurred.\n");
			snd_pcm_prepare(linux_context.pcm_handle);
			// @TODO: Exit early?
		} else if (frames_written_this_call < 0) {
			log_print(MINOR_ERROR_LOG, "Error from snd_pcm_writei: %s.\n", snd_strerror(frames_written_this_call));
			// @TODO: Exit early?
		} else {
			if (frames_written_this_call != (s32)pcm_playback_info.frames_per_period) {
				log_print(MINOR_ERROR_LOG, "PCM short write, wrote %d frames and expected %d.\n", frames_written_this_call, (s32)pcm_playback_info.frames_per_period);
			}
		}
#endif
	}
}

void
sound_init()
{
	platform_pcm_open_device();

	auto ma = mem_make_arena();
	sound_load("../data/sounds/fur_elise.wav", &ma);
	sound_load("../data/sounds/edited_random_sound.wav", &ma);
}

