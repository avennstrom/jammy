#if defined(JM_WINDOWS)
#include "audio.h"

#include <jammy/assert.h>
#include <jammy/file.h>
#include <jammy/remotery/Remotery.h>

#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include <stdio.h>

#define MAX_SOUNDS 1024

#if JM_ASSERT_ENABLED
#define JM_CHECK_DSOUND(func) jm_assert(SUCCEEDED(func))
#else
#define JM_CHECK_DSOUND(func) func
#endif

typedef struct jm_sound_data
{
	void* buffer;
	uint32_t bufferSize;
	struct
	{
		uint16_t wFormatTag;         /* format type */
		uint16_t nChannels;          /* number of channels (i.e. mono, stereo...) */
		uint32_t nSamplesPerSec;     /* sample rate */
		uint32_t nAvgBytesPerSec;    /* for buffer estimation */
		uint16_t nBlockAlign;        /* block size of data */
		uint16_t wBitsPerSample;     /* number of bits per sample of mono data */
		uint16_t cbSize;             /* the count in bytes of the size of */
	} waveFormat;
} jm_sound_data;

typedef struct jm_audio
{
	struct IDirectSound* directSound;
	uint32_t maxConcurrentSoundBuffers;

	uint32_t activeSoundBufferCount;
	struct IDirectSoundBuffer** activeSoundBuffers;

	uint32_t soundCount;
	uint64_t* soundKeys;
	jm_sound_data* sounds;
} jm_audio;

static jm_audio g_audio;

int jm_audio_init(
	HWND hwnd)
{
	if (FAILED(DirectSoundCreate(
		&DSDEVID_DefaultPlayback, 
		&g_audio.directSound,
		NULL)))
	{
		return 1;
	}

	if (FAILED(g_audio.directSound->lpVtbl->SetCooperativeLevel(
		g_audio.directSound,
		hwnd,
		DSSCL_PRIORITY)))
	{
		return 1;
	}

	g_audio.maxConcurrentSoundBuffers = 128;
	g_audio.activeSoundBuffers = calloc(g_audio.maxConcurrentSoundBuffers, sizeof(void*));
	g_audio.activeSoundBufferCount = 0;

	g_audio.soundCount = 0;
	g_audio.soundKeys = calloc(MAX_SOUNDS, sizeof(uint64_t));
	g_audio.sounds = calloc(MAX_SOUNDS, sizeof(jm_sound_data));

	return 0;
}

void jm_audio_update(
	jm_audio* audio)
{
	rmt_BeginCPUSample(jm_audio_update, 0);

	for (size_t i = 0; i < g_audio.activeSoundBufferCount;)
	{
		IDirectSoundBuffer* const buffer = g_audio.activeSoundBuffers[i];
		DWORD status;
		buffer->lpVtbl->GetStatus(buffer, &status);
		if (!(status & DSBSTATUS_PLAYING))
		{
			g_audio.activeSoundBuffers[i] = g_audio.activeSoundBuffers[--g_audio.activeSoundBufferCount];
			buffer->lpVtbl->Release(buffer);
		}
		else
		{
			++i;
		}
	}

	rmt_EndCPUSample();
}

typedef struct riff_header
{
	uint32_t chunkId;
	uint32_t chunkSize;
	uint32_t format;
} riff_header;

typedef struct wave_fmt
{
	uint32_t chunkId;
	uint32_t chunkSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	uint16_t extraParamSize;
} wave_fmt;

typedef struct wave_data
{
	uint32_t chunkId;
	uint32_t chunkSize;
} wave_data;

void jm_audio_play(
	jm_sound_handle sound)
{
	jm_assert(g_audio.activeSoundBufferCount < g_audio.maxConcurrentSoundBuffers);

	jm_sound_data soundData = g_audio.sounds[sound];

	DSBUFFERDESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwBufferBytes = soundData.bufferSize;
	bufferDesc.lpwfxFormat = (LPWAVEFORMATEX)&soundData.waveFormat;
	bufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS;

	IDirectSoundBuffer* dsBuffer;
	JM_CHECK_DSOUND(g_audio.directSound->lpVtbl->CreateSoundBuffer(
		g_audio.directSound,
		&bufferDesc,
		&dsBuffer,
		NULL));

	dsBuffer->lpVtbl->Initialize(dsBuffer, g_audio.directSound, &bufferDesc);

	void* audioPtr1;
	DWORD audioBytes1;
	void* audioPtr2;
	DWORD audioBytes2;
	JM_CHECK_DSOUND(dsBuffer->lpVtbl->Lock(
		dsBuffer,
		0,
		bufferDesc.dwBufferBytes,
		&audioPtr1,
		&audioBytes1,
		&audioPtr2,
		&audioBytes2,
		0));

	memcpy(audioPtr1, soundData.buffer, soundData.bufferSize);

	JM_CHECK_DSOUND(dsBuffer->lpVtbl->Unlock(
		dsBuffer,
		audioPtr1,
		audioBytes1,
		audioPtr2,
		audioBytes2));

	JM_CHECK_DSOUND(dsBuffer->lpVtbl->Play(dsBuffer, 0, 0, 0));

	g_audio.activeSoundBuffers[g_audio.activeSoundBufferCount++] = dsBuffer;
}

jm_sound_handle jm_load_sound(
	const char* filename)
{
	if (!jm_file_exists(filename))
	{
		printf("[ERROR] Can't find file '%s'", filename);
		return JM_SOUND_INVALID;
	}

	FILE* f = fopen(filename, "rb");

	riff_header riffHeader;
	fread(&riffHeader, 1, sizeof(riffHeader), f);
	jm_assert(riffHeader.chunkId == MAKEFOURCC('R','I','F','F'));
	jm_assert(riffHeader.format == MAKEFOURCC('W','A','V','E'));

	wave_fmt waveFmt;
	fread(&waveFmt, 1, sizeof(waveFmt), f);
	jm_assert(waveFmt.chunkId == MAKEFOURCC('f','m','t',' '));
	jm_assert(waveFmt.audioFormat == 1); // only PCM is supported atm

	// PCM doesn't support extra params
	fseek(f, -4, SEEK_CUR);

	wave_data waveData;
	fread(&waveData, 1, sizeof(waveData), f);
	jm_assert(waveData.chunkId == MAKEFOURCC('d','a','t','a'));

	void* audioBuffer = malloc(waveData.chunkSize);
	fread(audioBuffer, 1, waveData.chunkSize, f);
	fclose(f);

	const jm_sound_handle soundHandle = (jm_sound_handle)g_audio.soundCount++;

	jm_sound_data* soundData = &g_audio.sounds[soundHandle];
	soundData->buffer = audioBuffer;
	soundData->bufferSize = waveData.chunkSize;
	soundData->waveFormat.cbSize = 0;
	soundData->waveFormat.nAvgBytesPerSec = waveFmt.byteRate;
	soundData->waveFormat.nBlockAlign = waveFmt.blockAlign;
	soundData->waveFormat.nChannels = waveFmt.numChannels;
	soundData->waveFormat.nSamplesPerSec = waveFmt.sampleRate;
	soundData->waveFormat.wBitsPerSample = waveFmt.bitsPerSample;
	soundData->waveFormat.wFormatTag = waveFmt.audioFormat;

	return soundHandle;
}
#endif