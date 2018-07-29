#pragma once

#include <jammy/assert.h>

#include <inttypes.h>
#include <immintrin.h>

typedef uint32_t jm_color32;

jm_color32 jm_pack_color32_rgba_f32(
	float r,
	float g,
	float b,
	float a);

void jm_unpack_color32_rgba_f32(
	jm_color32 color,
	float* r,
	float* g,
	float* b,
	float* a);