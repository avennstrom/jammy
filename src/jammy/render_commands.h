#pragma once

#include <jammy/texture.h>
#include <jammy/font.h>
#include <jammy/effect.h>
#include <jammy/renderer.h>

#include <d3d11.h>

#include <float.h>

#define JM_DECLARE_RENDER_COMMAND(CommandName) \
	void __##CommandName(void*, const void*); \
	typedef struct CommandName CommandName; \
	struct CommandName

typedef struct jm_draw_context
{
	ID3D11DeviceContext* d3dctx;

	uint32_t vertexBufferOffset;
	uint32_t indexBufferOffset;
} jm_draw_context;

typedef void(*jm_render_command_dispatcher)(jm_draw_context*, const void*);

void jm_draw_context_begin(
	jm_draw_context* ctx,
	ID3D11DeviceContext* d3dctx);

JM_DECLARE_RENDER_COMMAND(jm_render_command_draw_text)
{
	char* text;
	jm_font_handle fontHandle;
	uint32_t color;
	float x, y;
	float width;
	float scale;
	float lineSpacingMultiplier;
	uint32_t rangeStart;
	uint32_t rangeEnd;
};

inline void jm_render_command_draw_text_init(
	jm_render_command_draw_text* cmd)
{
	cmd->x = 0.0f;
	cmd->y = 0.0f;
	cmd->width = FLT_MAX;
	cmd->scale = 1.0f;
	cmd->color = 0xffffffff;
	cmd->fontHandle = JM_FONT_HANDLE_INVALID;
	cmd->text = NULL;
	cmd->lineSpacingMultiplier = 1.0f;
	cmd->rangeStart = 0;
	cmd->rangeEnd = UINT32_MAX;
}

typedef struct jm_vertex
{
	float x, y;
} jm_vertex;

typedef struct jm_texcoord
{
	float u, v;
} jm_texcoord;

JM_DECLARE_RENDER_COMMAND(jm_render_command_draw)
{
	jm_vertex* vertices;
	jm_texcoord* texcoords;
	uint32_t vertexCount;
	void* indices;
	uint32_t indexCount;
	jm_primitive_topology topology;
	jm_fill_mode fillMode;
	jm_texture_handle textureHandle;
	uint32_t color;
	jm_sampler_state samplerState;
};

inline void jm_render_command_draw_init(
	jm_render_command_draw* cmd)
{
	cmd->vertexCount = 0;
	cmd->vertices = NULL;
	cmd->texcoords = NULL;
	cmd->indexCount = 0;
	cmd->indices = NULL;
	cmd->topology = JM_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	cmd->fillMode = JM_FILL_MODE_SOLID;
	cmd->textureHandle = JM_TEXTURE_HANDLE_INVALID;
	cmd->color = 0xffffffff;
	cmd->samplerState = JM_SAMPLER_STATE_POINT;
}