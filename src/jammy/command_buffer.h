#pragma once

#include <inttypes.h>

#include <jammy/render_commands.h>
#include <jammy/texture.h>
#include <jammy/font.h>

#define JM_COMMAND_BUFFER_PUSH(CommandBuffer, CommandName) \
	jm_command_buffer_push(CommandBuffer, sizeof(CommandName), __##CommandName)

typedef struct jm_command_buffer
{
	char* buffer;
	char* bufferIt;
	size_t capacity;

	int32_t* keys;
	void** commands;
	size_t commandIt;
	size_t maxCommands;

	size_t* indices;
} jm_command_buffer;

extern jm_command_buffer* g_currentCommandBuffer;

int jm_command_buffer_init(
	jm_command_buffer* cb,
	size_t size,
	size_t maxCommands);

void jm_command_buffer_destroy(
	jm_command_buffer* cb);

int jm_command_buffer_begin(
	jm_command_buffer* cb);

void jm_command_buffer_sort(
	jm_command_buffer* cb);

void jm_command_buffer_execute(
	jm_command_buffer* cb,
	struct jm_draw_context* ctx);

void* jm_command_buffer_push(
	jm_command_buffer* cb,
	size_t commandSize,
	jm_render_command_dispatcher dispatcher);

void* jm_command_buffer_alloc(
	jm_command_buffer* cb,
	size_t size);

void jm_set_current_command_buffer(
	jm_command_buffer* cb);