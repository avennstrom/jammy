#if defined(JM_LINUX)
#include "audio.h"

int jm_audio_init(
	void* hwnd)
{
}

void jm_audio_update()
{
}

void jm_audio_play(
	jm_sound_handle soundHandle)
{
}

jm_sound_handle jm_load_sound(
	const char* filename)
{
    return (jm_sound_handle)0;
}

#endif