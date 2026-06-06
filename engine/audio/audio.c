#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio/audio.h"

struct bapi_sound_internal {
    SDL_AudioSpec spec;
    Uint8* buffer;
    Uint32 length;
    float volume;
    int loop;
    int playing;
    SDL_AudioStream* stream;
    int queued_once;
    struct bapi_sound_internal* next_active;
};

static SDL_AudioDeviceID audio_device = 0;
static bapi_sound_t active_sounds = NULL;

static void remove_active_sound(bapi_sound_t sound) {
    bapi_sound_t* current = &active_sounds;
    while (*current != NULL) {
        if (*current == sound) {
            *current = sound->next_active;
            sound->next_active = NULL;
            return;
        }
        current = &(*current)->next_active;
    }
}

static void add_active_sound(bapi_sound_t sound) {
    if (sound == NULL || sound->next_active != NULL || active_sounds == sound) {
        return;
    }

    sound->next_active = active_sounds;
    active_sounds = sound;
}

static int ensure_sound_stream(bapi_sound_t sound) {
    if (sound->stream != NULL) {
        return 0;
    }

    sound->stream = SDL_CreateAudioStream(&sound->spec, NULL);
    if (sound->stream == NULL) {
        printf("[AUDIO] Failed to create sound stream: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_BindAudioStream(audio_device, sound->stream)) {
        printf("[AUDIO] Failed to bind sound stream: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(sound->stream);
        sound->stream = NULL;
        return 1;
    }

    if (!SDL_SetAudioStreamGain(sound->stream, sound->volume)) {
        printf("[AUDIO] Failed to set sound gain: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(sound->stream);
        sound->stream = NULL;
        return 1;
    }

    return 0;
}

static int queue_sound_buffer(bapi_sound_t sound) {
    if (!SDL_PutAudioStreamData(sound->stream, sound->buffer, (int)sound->length)) {
        printf("[AUDIO] Failed to queue audio data: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_FlushAudioStream(sound->stream)) {
        printf("[AUDIO] Failed to flush audio stream: %s\n", SDL_GetError());
        return 1;
    }

    return 0;
}

int bapi_audio_init(void) {
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 44100;

    audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (audio_device == 0) {
        printf("[AUDIO] Failed to open audio device: %s\n", SDL_GetError());
        return 1;
    }

    printf("[AUDIO] Audio device opened successfully\n");
    return 0;
}

void bapi_audio_cleanup(void) {
    bapi_sound_t sound = active_sounds;
    while (sound != NULL) {
        bapi_sound_t next = sound->next_active;
        sound->playing = 0;
        sound->queued_once = 0;
        if (sound->stream != NULL) {
            SDL_ClearAudioStream(sound->stream);
        }
        sound->next_active = NULL;
        sound = next;
    }

    active_sounds = NULL;

    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
}

bapi_sound_t bapi_sound_load(const char* filepath) {
    bapi_sound_t sound = malloc(sizeof(struct bapi_sound_internal));
    if (sound == NULL) {
        return NULL;
    }
    
    SDL_zero(sound->spec);
    if (!SDL_LoadWAV(filepath, &sound->spec, &sound->buffer, &sound->length)) {
        printf("[AUDIO] Failed to load WAV %s: %s\n", filepath, SDL_GetError());
        free(sound);
        return NULL;
    }
    
    printf("[AUDIO] Loaded WAV: format=%d, channels=%d, freq=%d, length=%u\n",
           sound->spec.format, sound->spec.channels, sound->spec.freq, sound->length);
    
    sound->volume = 1.0f;
    sound->loop = 0;
    sound->playing = 0;
    sound->stream = NULL;
    sound->queued_once = 0;
    sound->next_active = NULL;
    return sound;
}

int bapi_sound_play(bapi_sound_t sound) {
    if (sound == NULL || audio_device == 0) {
        printf("[AUDIO] Play failed: sound=%p, device=%u\n", (void*)sound, audio_device);
        return 1;
    }

    if (ensure_sound_stream(sound) != 0) {
        return 1;
    }

    SDL_ClearAudioStream(sound->stream);

    sound->playing = 1;
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

void bapi_sound_set_volume(bapi_sound_t sound, float volume) {
    if (sound != NULL) {
        if (volume < 0.0f) {
            sound->volume = 0.0f;
        } else if (volume > 1.0f) {
            sound->volume = 1.0f;
        } else {
            sound->volume = volume;
        }

        if (sound->stream != NULL) {
            SDL_SetAudioStreamGain(sound->stream, sound->volume);
        }
    }
}

void bapi_sound_set_loop(bapi_sound_t sound, int loop) {
    if (sound != NULL) {
        sound->loop = loop;
    }
}

void bapi_sound_stop(bapi_sound_t sound) {
    if (sound != NULL) {
        sound->playing = 0;
        sound->queued_once = 0;
        if (sound->stream != NULL) {
            SDL_ClearAudioStream(sound->stream);
        }
        remove_active_sound(sound);
    }
}

void bapi_sound_update(void) {
    bapi_sound_t sound = active_sounds;

    while (sound != NULL) {
        bapi_sound_t next = sound->next_active;

        if (!sound->playing || sound->stream == NULL) {
            remove_active_sound(sound);
            sound = next;
            continue;
        }

        int queued = SDL_GetAudioStreamQueued(sound->stream);
        if (queued < 0) {
            printf("[AUDIO] Failed to query queued audio data: %s\n", SDL_GetError());
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
            sound->playing = 0;
            sound->queued_once = 0;
            remove_active_sound(sound);
        }

        sound = next;
    }
}

void bapi_sound_free(bapi_sound_t sound) {
    if (sound != NULL) {
        bapi_sound_stop(sound);
        if (sound->buffer != NULL) {
            SDL_free(sound->buffer);
        }
        if (sound->stream != NULL) {
            SDL_DestroyAudioStream(sound->stream);
        }
        free(sound);
    }
}
