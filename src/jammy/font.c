#include "font.h"

#include <jammy/hash.h>
#include <jammy/file.h>
#include <jammy/assert.h>
#include <jammy/renderer.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <d3d11.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define MAX_FONTS 512

typedef struct jm_fonts
{
	FT_Library library;

	size_t count;
	uint64_t* keys;
	FT_Face* faces;

	jm_font_info* fontInfo;
} jm_fonts;

jm_fonts g_fonts;

int jm_fonts_init()
{
	FT_Error error = FT_Init_FreeType(&g_fonts.library);
	if (error)
	{

	}

	g_fonts.count = 0;
	g_fonts.keys = calloc(MAX_FONTS, sizeof(uint64_t));
	g_fonts.faces = calloc(MAX_FONTS, sizeof(FT_Face*));
	g_fonts.fontInfo = calloc(MAX_FONTS, sizeof(jm_font_info));
	return 0;
}

static int key_search_compare(const uint64_t* a, const uint64_t* b)
{
	return (*a) == (*b);
}

jm_font_handle jm_load_font(
	const char* path,
	uint32_t size)
{
	const uint64_t size64 = size;
	const uint64_t key = jm_fnv(path) ^ (size64 << 32 | size64);
	const uint64_t* find = bsearch(&key, g_fonts.keys, g_fonts.count, sizeof(uint64_t), key_search_compare);
	if (find)
	{
		return (jm_font_handle)(find - g_fonts.keys);
	}

	if (!jm_file_exists(path))
	{
		printf("[ERROR] Can't find file '%s'", path);
		return JM_FONT_HANDLE_INVALID;
	}

	const jm_font_handle fontHandle = (jm_font_handle)g_fonts.count++;
	g_fonts.keys[fontHandle] = key;

	FT_Face face;
	FT_Error error = FT_New_Face(g_fonts.library, path, 0, &face);
	if (error)
	{
		fprintf(stderr, "[ERROR] %d", error);
		return JM_FONT_HANDLE_INVALID;
	}

	error = FT_Set_Char_Size(face, 0, size, 300, 300);
	if (error)
	{
		fprintf(stderr, "[ERROR] %d", error);
		return JM_FONT_HANDLE_INVALID;
	}
	
	uint32_t maxGlyphWidth = 0;
	uint32_t maxGlyphRows = 0;
	uint32_t glyphCount = 0;

	uint32_t maxCharcode = 0;

	FT_UInt gindex;
	FT_ULong charcode = FT_Get_First_Char(face, &gindex);
	while (gindex != 0)
	{
		FT_Error error = FT_Load_Glyph(face, gindex, FT_LOAD_BITMAP_METRICS_ONLY);
		jm_assert(error == 0);

		maxGlyphWidth = max(maxGlyphWidth, face->glyph->bitmap.width);
		maxGlyphRows = max(maxGlyphRows, face->glyph->bitmap.rows);
		maxCharcode = max(maxCharcode, charcode);
		++glyphCount;

		charcode = FT_Get_Next_Char(face, charcode, &gindex);
	}

	const uint32_t glyphGridWidth = (uint32_t)ceilf(sqrtf((float)glyphCount));
	const uint32_t glyphGridHeight = (uint32_t)ceilf((float)glyphCount / (float)glyphGridWidth);
	const uint32_t glyphBitmapRowPitch = maxGlyphWidth * glyphGridWidth;
	const uint32_t glyphGridPitchX = maxGlyphWidth;
	const uint32_t glyphGridPitchY = glyphBitmapRowPitch * maxGlyphRows;

	const size_t pixelBufferSize = glyphBitmapRowPitch * maxGlyphRows * glyphGridHeight;
	uint8_t* pixels = malloc(pixelBufferSize);
	memset(pixels, 0xffffffff, pixelBufferSize);

	jm_font_info fontInfo;
	fontInfo.height = (float)(face->height >> 6);
	fontInfo.glyphs = calloc(maxCharcode + 1, sizeof(jm_glyph_info));
	memset(fontInfo.glyphs, 0x00000000, (maxCharcode + 1) * sizeof(jm_glyph_info));

	charcode = FT_Get_First_Char(face, &gindex);
	uint32_t i = 0;
	while (gindex != 0)
	{
		FT_Error error = FT_Load_Glyph(face, gindex, FT_LOAD_RENDER);
		jm_assert(error == 0);

		const uint32_t x = i % glyphGridWidth;
		const uint32_t y = i / glyphGridWidth;

		jm_glyph_info* glyphInfo = &fontInfo.glyphs[charcode];

		glyphInfo->hasBitmap = (face->glyph->bitmap.buffer != NULL);
		glyphInfo->advance_x = (float)(face->glyph->advance.x >> 6);
		if (face->glyph->bitmap.buffer)
		{
			glyphInfo->u0 = (float)x / (float)glyphGridWidth;
			glyphInfo->v0 = (float)y / (float)glyphGridHeight;
			glyphInfo->u1 = glyphInfo->u0 + (float)face->glyph->bitmap.width / (float)glyphBitmapRowPitch;
			glyphInfo->v1 = glyphInfo->v0 + (float)face->glyph->bitmap.rows / (float)(maxGlyphRows * glyphGridHeight);
			glyphInfo->bitmap_left = (float)face->glyph->bitmap_left;
			glyphInfo->bitmap_top = (float)face->glyph->bitmap_top;
			glyphInfo->width = (float)face->glyph->bitmap.width;
			glyphInfo->height = (float)face->glyph->bitmap.rows;

			uint8_t* dstPixels = pixels + (x * glyphGridPitchX) + (y * glyphGridPitchY);

			const uint8_t* srcPixels = face->glyph->bitmap.buffer;
			const size_t srcRowPitch = face->glyph->bitmap.pitch;

			for (size_t row = 0; row < face->glyph->bitmap.rows; ++row)
			{
				memcpy(
					dstPixels + (row * glyphBitmapRowPitch),
					srcPixels + (row * srcRowPitch),
					srcRowPitch);
			}
		}

		charcode = FT_Get_Next_Char(face, charcode, &gindex);
		++i;
	}

	// todo - cross platform
	D3D11_TEXTURE2D_DESC desc;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.Width = maxGlyphWidth * glyphGridWidth;
	desc.Height = maxGlyphRows * glyphGridHeight;

	D3D11_SUBRESOURCE_DATA sd;
	sd.pSysMem = pixels;
	sd.SysMemPitch = glyphBitmapRowPitch;
	sd.SysMemSlicePitch = 0;

	ID3D11Device* d3ddev = jm_renderer_get_device();

	ID3D11Texture2D* texture;
	d3ddev->lpVtbl->CreateTexture2D(d3ddev, &desc, &sd, &texture);
	d3ddev->lpVtbl->CreateShaderResourceView(d3ddev, (ID3D11Resource*)texture, NULL, &fontInfo.srv);
	texture->lpVtbl->Release(texture);

	free(pixels);

	g_fonts.fontInfo[fontHandle] = fontInfo;
	g_fonts.faces[fontHandle] = face;

	return fontHandle;
}

void* jm_font_get_face(
	jm_font_handle fontHandle)
{
	jm_assert(fontHandle != JM_FONT_HANDLE_INVALID);
	return g_fonts.faces[fontHandle];
}

const jm_font_info* jm_font_get_info(
	jm_font_handle fontHandle)
{
	jm_assert(fontHandle != JM_FONT_HANDLE_INVALID);
	return &g_fonts.fontInfo[fontHandle];
}

float jm_font_measure_text(
	jm_font_handle fontHandle,
	const char* text)
{
	return jm_font_measure_text_range(fontHandle, text, text + strlen(text));
}

float jm_font_measure_text_range(
	jm_font_handle fontHandle,
	const char* begin,
	const char* end)
{
	const jm_font_info* fontInfo = jm_font_get_info(fontHandle);
	const jm_glyph_info* glyphInfo = fontInfo->glyphs;

	float textWidth = 0.0f;
	const char* text = begin;
	while (text != end)
	{
		const char charcode = *text;
		textWidth += glyphInfo[charcode].advance_x;
		++text;
	}
	return textWidth;
}