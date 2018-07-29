#pragma once

#include <jammy/render_types.h>

#include <inttypes.h>
#include <stdbool.h>

#define JM_FONT_HANDLE_INVALID ((jm_font_handle)-1)

typedef uint32_t jm_font_handle;

typedef struct jm_glyph_info
{
	float u0, v0;
	float u1, v1;
	float width;
	float height;
	float bitmap_left;
	float bitmap_top;
	float advance_x;
	bool hasBitmap;
} jm_glyph_info;

typedef struct jm_font_info
{
	void* srv;
	jm_glyph_info* glyphs;
	float height;
} jm_font_info;

int jm_fonts_init();

jm_font_handle jm_load_font(
	const char* path,
	uint32_t size);

float jm_font_measure_text(
	jm_font_handle fontHandle,
	const char* text);

float jm_font_measure_text_range(
	jm_font_handle fontHandle,
	const char* begin,
	const char* end);

const jm_font_info* jm_font_get_info(
	jm_font_handle fontHandle);

void jm_font_get_text_vertices(
	jm_font_handle fontHandle,
	const char* text,
	float topLeftX,
	float topLeftY,
	float width,
	uint32_t rangeStart,
	uint32_t rangeEnd,
	float textScale,
	jm_vertex* dstPosition,
	jm_texcoord* dstTexcoord,
	uint16_t* dstIndex,
	uint32_t* outIndexCount);