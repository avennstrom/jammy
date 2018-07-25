#pragma once

#include <inttypes.h>

#define JM_SOUND_INVALID ((jm_sound_handle)-1)

typedef uint32_t jm_sound_handle;

int jm_audio_init(
	void* hwnd);

void jm_audio_update();

void jm_audio_play(
	jm_sound_handle soundHandle);

jm_sound_handle jm_load_sound(
	const char* filename);
