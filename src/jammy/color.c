#include "color.h"

static jm_color32 jm_pack_color32_rgba_u8(
	uint8_t r,
	uint8_t g,
	uint8_t b,
	uint8_t a)
{
	const uint32_t rgba = r | (g << 8) | (b << 16) | (a << 24);
	return rgba;
}

jm_color32 jm_pack_color32_rgba_f32(
	float r,
	float g,
	float b,
	float a)
{
	jm_assert(r >= 0.0f && r <= 1.0f);
	jm_assert(r >= 0.0f && r <= 1.0f);
	jm_assert(r >= 0.0f && r <= 1.0f);
	jm_assert(r >= 0.0f && r <= 1.0f);

	const uint32_t r8 = (uint32_t)(r * 255.0f);
	const uint32_t g8 = (uint32_t)(g * 255.0f);
	const uint32_t b8 = (uint32_t)(b * 255.0f);
	const uint32_t a8 = (uint32_t)(a * 255.0f);
	const uint32_t rgba = jm_pack_color32_rgba_u8(r8 & 0xff, g8 & 0xff, b8 & 0xff, a8 & 0xff);
	return rgba;
}

static void jm_unpack_color32_rgba_u8(
	jm_color32 color,
	uint8_t* r,
	uint8_t* g,
	uint8_t* b,
	uint8_t* a)
{
	*r = (uint8_t)(color) & 0xff;
	*g = (uint8_t)(color >> 8) & 0xff;
	*b = (uint8_t)(color >> 16) & 0xff;
	*a = (uint8_t)(color >> 24) & 0xff;
}

void jm_unpack_color32_rgba_f32(
	jm_color32 color,
	float* r,
	float* g,
	float* b,
	float* a)
{
	jm_assert(r);
	jm_assert(g);
	jm_assert(b);
	jm_assert(a);

	uint8_t r8, g8, b8, a8;
	jm_unpack_color32_rgba_u8(color, &r8, &g8, &b8, &a8);

	*r = r8 / 255.0f;
	*g = g8 / 255.0f;
	*b = b8 / 255.0f;
	*a = a8 / 255.0f;
}