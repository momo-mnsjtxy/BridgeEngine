#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bapi_sound_internal {
	plat_audio_spec_t			spec;
	uint8_t					   *buffer;
	uint32_t					length;
	float						volume;
	int							loop;
	int							playing;
	plat_audio_stream_t			stream;
	int							queued_once;
	struct bapi_sound_internal *next_active;
	struct bapi_sound_internal *next_allocated;
};

static plat_audio_device_t audio_device	 = NULL;
static bapi_sound_t		   active_sounds = NULL;
static bapi_sound_t		   allocated_sounds = NULL;
static int				   audio_unsupported_warning_logged;

static int audio_is_supported(void)
{
	return plat_supports(PLAT_CAPABILITY_AUDIO);
}

static void warn_audio_unsupported_once(const plat_interface_t *plat)
{
	if (!audio_unsupported_warning_logged && plat != NULL && plat->core.log_warn != NULL) {
		plat->core.log_warn("Audio is not supported by this platform");
		audio_unsupported_warning_logged = 1;
	}
}

static void remove_active_sound(bapi_sound_t sound)
{
	bapi_sound_t *current = &active_sounds;
	while (*current != NULL) {
		if (*current == sound) {
			*current		   = sound->next_active;
			sound->next_active = NULL;
			return;
		}
		current = &(*current)->next_active;
	}
}

static void add_active_sound(bapi_sound_t sound)
{
	if (sound == NULL || sound->next_active != NULL || active_sounds == sound) {
		return;
	}

	sound->next_active = active_sounds;
	active_sounds	   = sound;
}

static void remove_allocated_sound(bapi_sound_t sound)
{
	bapi_sound_t *current = &allocated_sounds;
	while (*current != NULL) {
		if (*current == sound) {
			*current = sound->next_allocated;
			return;
		}
		current = &(*current)->next_allocated;
	}
}

static int ensure_sound_stream(bapi_sound_t sound)
{
	if (!audio_is_supported()) return 1;

	if (sound->stream != NULL) {
		return 0;
	}

	const plat_interface_t *plat = plat_get();

	sound->stream = plat->audio.create_audio_stream(&sound->spec);
	if (sound->stream == NULL) {
		printf("[AUDIO] Failed to create sound stream\n");
		return 1;
	}

	if (plat->audio.bind_audio_stream(audio_device, sound->stream) != 0) {
		printf("[AUDIO] Failed to bind sound stream\n");
		plat->audio.destroy_audio_stream(sound->stream);
		sound->stream = NULL;
		return 1;
	}

	if (plat->audio.set_audio_stream_gain(sound->stream, sound->volume) != 0) {
		printf("[AUDIO] Failed to set sound gain\n");
		plat->audio.destroy_audio_stream(sound->stream);
		sound->stream = NULL;
		return 1;
	}

	return 0;
}

static int queue_sound_buffer(bapi_sound_t sound)
{
	if (!audio_is_supported()) return 1;

	const plat_interface_t *plat = plat_get();

	if (plat->audio.put_audio_stream_data(sound->stream, sound->buffer, (int)sound->length) != 0) {
		printf("[AUDIO] Failed to queue audio data\n");
		return 1;
	}

	if (plat->audio.flush_audio_stream(sound->stream) != 0) {
		printf("[AUDIO] Failed to flush audio stream\n");
		return 1;
	}

	return 0;
}

int bapi_audio_init(void)
{
	const plat_interface_t *plat = plat_get();
	if (!audio_is_supported() || plat->audio.open_audio_device == NULL) {
		warn_audio_unsupported_once(plat);
		return 1;
	}

	audio_device = plat->audio.open_audio_device(PLAT_AUDIO_F32, 2, 44100);
	if (audio_device == NULL) {
		printf("[AUDIO] Failed to open audio device\n");
		return 1;
	}

	printf("[AUDIO] Audio device opened successfully\n");
	return 0;
}

void bapi_audio_cleanup(void)
{
	if (!audio_is_supported()) return;

	const plat_interface_t *plat = plat_get();

	while (allocated_sounds != NULL) {
		bapi_sound_free(allocated_sounds);
	}
	active_sounds = NULL;

	if (audio_device != NULL) {
		plat->audio.close_audio_device(audio_device);
		audio_device = NULL;
	}
}

bapi_sound_t bapi_sound_load(const char *filepath)
{
	const plat_interface_t *plat = plat_get();
	if (!audio_is_supported() || plat->audio.load_wav == NULL) {
		warn_audio_unsupported_once(plat);
		return NULL;
	}

	bapi_sound_t sound = malloc(sizeof(struct bapi_sound_internal));
	if (sound == NULL) {
		return NULL;
	}

	memset(&sound->spec, 0, sizeof(plat_audio_spec_t));
	if (plat->audio.load_wav(filepath, &sound->spec, &sound->buffer, &sound->length) != 0) {
		printf("[AUDIO] Failed to load WAV %s\n", filepath);
		free(sound);
		return NULL;
	}

	printf("[AUDIO] Loaded WAV: channels=%d, freq=%d, length=%u\n", sound->spec.channels,
		   sound->spec.freq, sound->length);

	sound->volume	   = 1.0f;
	sound->loop		   = 0;
	sound->playing	   = 0;
	sound->stream	   = NULL;
	sound->queued_once = 0;
	sound->next_active = NULL;
	sound->next_allocated = allocated_sounds;
	allocated_sounds = sound;
	return sound;
}

int bapi_sound_play(bapi_sound_t sound)
{
	if (!audio_is_supported()) return 1;

	if (sound == NULL || audio_device == NULL) {
		printf("[AUDIO] Play failed: sound=%p, device=%p\n", (void *)sound, (void *)audio_device);
		return 1;
	}

	if (ensure_sound_stream(sound) != 0) {
		return 1;
	}

	const plat_interface_t *plat = plat_get();
	plat->audio.clear_audio_stream(sound->stream);

	sound->playing	   = 1;
	sound->queued_once = 0;
	add_active_sound(sound);

	printf("[AUDIO] Playing sound, length=%u, loop=%d\n", sound->length, sound->loop);
	if (queue_sound_buffer(sound) != 0) {
		sound->playing = 0;
		remove_active_sound(sound);
		return 1;
	}

	sound->queued_once = 1;
	return 0;
}

void bapi_sound_set_volume(bapi_sound_t sound, float volume)
{
	if (!audio_is_supported()) return;

	if (sound != NULL) {
		const plat_interface_t *plat = plat_get();

		if (volume < 0.0f) {
			sound->volume = 0.0f;
		} else if (volume > 1.0f) {
			sound->volume = 1.0f;
		} else {
			sound->volume = volume;
		}

		if (sound->stream != NULL) {
			plat->audio.set_audio_stream_gain(sound->stream, sound->volume);
		}
	}
}

void bapi_sound_set_loop(bapi_sound_t sound, int loop)
{
	if (!audio_is_supported()) return;

	if (sound != NULL) {
		sound->loop = loop;
	}
}

void bapi_sound_stop(bapi_sound_t sound)
{
	if (!audio_is_supported()) return;

	if (sound != NULL) {
		const plat_interface_t *plat = plat_get();

		sound->playing	   = 0;
		sound->queued_once = 0;
		if (sound->stream != NULL) {
			plat->audio.clear_audio_stream(sound->stream);
		}
		remove_active_sound(sound);
	}
}

void bapi_sound_update(void)
{
	if (!audio_is_supported()) return;

	const plat_interface_t *plat  = plat_get();
	bapi_sound_t			sound = active_sounds;

	while (sound != NULL) {
		bapi_sound_t next = sound->next_active;

		if (!sound->playing || sound->stream == NULL) {
			remove_active_sound(sound);
			sound = next;
			continue;
		}

		int queued = plat->audio.get_audio_stream_queued(sound->stream);
		if (queued < 0) {
			printf("[AUDIO] Failed to query queued audio data\n");
			sound->playing = 0;
			remove_active_sound(sound);
			sound = next;
			continue;
		}

		if (sound->loop) {
			if (queued <= (int)(sound->length / 2) && queue_sound_buffer(sound) != 0) {
				sound->playing = 0;
				remove_active_sound(sound);
			}
		} else if (sound->queued_once && queued == 0) {
			sound->playing	   = 0;
			sound->queued_once = 0;
			remove_active_sound(sound);
		}

		sound = next;
	}
}

void bapi_sound_free(bapi_sound_t sound)
{
	if (!audio_is_supported()) return;

	if (sound != NULL) {
		const plat_interface_t *plat = plat_get();

		bapi_sound_stop(sound);
		if (sound->buffer != NULL) {
			plat->audio.mem_free(sound->buffer);
		}
		if (sound->stream != NULL) {
			plat->audio.destroy_audio_stream(sound->stream);
		}
		remove_allocated_sound(sound);
		free(sound);
	}
}
