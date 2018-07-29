#pragma once

#include <jammy/render_types.h>

#include <inttypes.h>
#include <stdbool.h>

#define JM_TEXTURE_HANDLE_INVALID ((jm_texture_handle)-1)

typedef uint32_t jm_texture_handle;

typedef struct jm_texture_info
{
	uint32_t width;
	uint32_t height;
	bool isSemitransparent;
} jm_texture_info;

int jm_textures_init();

jm_texture_handle jm_load_texture(
	const char* path);

void jm_texture_reload(
	const char* path);

jm_texture_resource jm_texture_get_resource(
	jm_texture_handle textureHandle);

const jm_texture_info* jm_texture_get_info(
	jm_texture_handle textureHandle);

bool jm_texture_isSemitransparent(
	jm_texture_handle textureHandle);
