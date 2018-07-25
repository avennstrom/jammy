#include "texture.h"

#include <jammy/hash.h>
#include <jammy/file.h>
#include <jammy/assert.h>
#include <jammy/renderer.h>
#include <jammy/lodepng/lodepng.h>

#include <lua.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_TEXTURES 1024

typedef struct jm_textures
{
	size_t count;
	uint64_t* keys;
	ID3D11Texture2D** textures;
	ID3D11ShaderResourceView** textureSRVs;
	jm_texture_info* textureInfo;
} jm_textures;

jm_textures g_textures;

int jm_textures_init()
{
	g_textures.count = 0;
	g_textures.keys = calloc(MAX_TEXTURES, sizeof(uint64_t));
	g_textures.textures = calloc(MAX_TEXTURES, sizeof(void*));
	g_textures.textureSRVs = calloc(MAX_TEXTURES, sizeof(void*));
	g_textures.textureInfo = calloc(MAX_TEXTURES, sizeof(jm_texture_info));
	return 0;
}

static bool try_load_bmp(
	const char* path,
	void** outPixels,
	uint32_t* outWidth,
	uint32_t* outHeight,
	DXGI_FORMAT* outFormat)
{
	FILE* f = fopen(path, "r");

	BITMAPFILEHEADER bmpHeader;
	fread(&bmpHeader, sizeof(bmpHeader), 1, f);

	if (bmpHeader.bfType != 0x4D42)
	{
		// not a bmp file
		return false;
	}

	BITMAPINFOHEADER bmpInfoHeader;
	fread(&bmpInfoHeader, sizeof(bmpInfoHeader), 1, f);

	uint8_t* srcPixels = malloc(bmpInfoHeader.biSizeImage);
	fread(srcPixels, 1, bmpInfoHeader.biSizeImage, f);
	fclose(f);

	uint8_t* pixels = malloc(bmpInfoHeader.biSizeImage);

	// flip image upside down
	const size_t bytesPerRow = bmpInfoHeader.biWidth * 4;
	for (size_t dstRow = 0; dstRow < bmpInfoHeader.biHeight; ++dstRow)
	{
		const size_t srcRow = bmpInfoHeader.biHeight - dstRow - 1;
		memcpy(pixels + dstRow * bytesPerRow, srcPixels + srcRow * bytesPerRow, bytesPerRow);
	}

	// swizzle bgr to rgb
	for (size_t i = 0; i < (bmpInfoHeader.biSizeImage / 4 * 4); i += 4)
	{
		const uint8_t r = pixels[i];
		const uint8_t b = pixels[i + 2];
		pixels[i] = b;
		pixels[i + 2] = r;
	}

	free(srcPixels);

	*outPixels = pixels;
	*outWidth = bmpInfoHeader.biWidth;
	*outHeight = bmpInfoHeader.biHeight;
	*outFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	return true;
}

static bool try_load_png(
	const char* path,
	void** outPixels,
	uint32_t* outWidth,
	uint32_t* outHeight,
	DXGI_FORMAT* outFormat)
{
	unsigned error = lodepng_decode32_file((uint8_t**)outPixels, outWidth, outHeight, path);

	if (error)
	{
		printf("decoder error %u: %s\n", error, lodepng_error_text(error));
		return false;
	}

	*outFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	return true;
}

static bool jm_load_image_pixels(
	const char* path,
	void** outPixels,
	uint32_t* outWidth, 
	uint32_t* outHeight,
	DXGI_FORMAT* outFormat)
{
	if (try_load_bmp(path, outPixels, outWidth, outHeight, outFormat))
	{
		return true;
	}
	if (try_load_png(path, outPixels, outWidth, outHeight, outFormat))
	{
		return true;
	}

	printf("[ERROR] Unrecognized image format");
	return false;
}

static int key_search_compare(const uint64_t* a, const uint64_t* b)
{
	if (*a <  *b) return -1;
	if (*a == *b) return 0;
	if (*a >  *b) return 1;
	__assume(0);
}

jm_texture_handle jm_load_texture(
	const char* path)
{
	const uint64_t key = jm_fnv(path);
	const uint64_t* find = bsearch(&key, g_textures.keys, g_textures.count, sizeof(uint64_t), key_search_compare);
	if (find)
	{
		return (jm_texture_handle)(find - g_textures.keys);
	}

	if (!jm_file_exists(path))
	{
		printf("[ERROR] Can't find file '%s'", path);
		return JM_TEXTURE_HANDLE_INVALID;
	}

	const jm_texture_handle textureHandle = (jm_texture_handle)g_textures.count++;
	g_textures.keys[textureHandle] = key;
	
	void* pixels;
	uint32_t width, height;
	DXGI_FORMAT format;
	if (!jm_load_image_pixels(path, &pixels, &width, &height, &format))
	{
		return JM_TEXTURE_HANDLE_INVALID;
	}

	bool isSemitransparent = false;
	for (size_t i = 0; i < (width * height); ++i)
	{
		const uint8_t alpha = ((uint8_t*)pixels)[i * 4 + 3];
		if (alpha > 0x00 && alpha < 0xff)
		{
			isSemitransparent = true;
			break;
		}
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = format;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.Width = width;
	textureDesc.Height = height;

	D3D11_SUBRESOURCE_DATA textureData;
	textureData.pSysMem = pixels;
	textureData.SysMemPitch = textureDesc.Width * 4;
	textureData.SysMemSlicePitch = 0;

	ID3D11Device* d3ddevice = jm_renderer_get_device();

	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
	d3ddevice->lpVtbl->CreateTexture2D(d3ddevice, &textureDesc, &textureData, &texture);
	d3ddevice->lpVtbl->CreateShaderResourceView(d3ddevice, (ID3D11Resource*)texture, NULL, &srv);

	free(pixels);

#if defined(JM_DEBUG)
	texture->lpVtbl->SetPrivateData(texture, &WKPDID_D3DDebugObjectName, (UINT)strlen(path), path);
#endif

	jm_texture_info textureInfo;
	textureInfo.width = width;
	textureInfo.height = height;
	textureInfo.isSemitransparent = isSemitransparent;

	g_textures.textureInfo[textureHandle] = textureInfo;
	g_textures.textures[textureHandle] = texture;
	g_textures.textureSRVs[textureHandle] = srv;
	
	return textureHandle;
}

void jm_texture_reload(
	const char* path)
{
	const uint64_t key = jm_fnv(path);
	const jm_texture_handle textureHandle = jm_load_texture(path);

}

ID3D11ShaderResourceView* jm_texture_get_srv(
	jm_texture_handle textureHandle)
{
	jm_assert(textureHandle != JM_TEXTURE_HANDLE_INVALID);
	return g_textures.textureSRVs[textureHandle];
}

const jm_texture_info* jm_texture_get_info(
	jm_texture_handle textureHandle)
{
	jm_assert(textureHandle != JM_TEXTURE_HANDLE_INVALID);
	return &g_textures.textureInfo[textureHandle];
}

bool jm_texture_isSemitransparent(
	jm_texture_handle textureHandle)
{
	return jm_texture_get_info(textureHandle)->isSemitransparent;
}