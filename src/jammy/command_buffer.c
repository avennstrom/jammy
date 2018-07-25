#include "command_buffer.h"

#include <jammy/assert.h>
#include <jammy/remotery/Remotery.h>

#include <lua.h>

#include <malloc.h>
#include <stdlib.h>
#include <memory.h>

jm_command_buffer* g_currentCommandBuffer = NULL;

int jm_command_buffer_init(
	jm_command_buffer* cb,
	size_t size,
	size_t maxCommands)
{
	cb->maxCommands = maxCommands;
	cb->capacity = size;
	cb->buffer = malloc(size);
	cb->commands = malloc(sizeof(void*) * maxCommands);
	cb->keys = malloc(sizeof(int32_t) * maxCommands);
	cb->indices = malloc(sizeof(uint32_t) * maxCommands);
	return 0;
}

void jm_command_buffer_destroy(
	jm_command_buffer* cb)
{
	free(cb->buffer);
	free(cb->commands);
	free(cb->keys);
	free(cb->indices);
}

int jm_command_buffer_begin(
	jm_command_buffer* cb)
{
	cb->bufferIt = cb->buffer;
	cb->commandIt = 0;
	return 0;
}

void jm_command_buffer_sort(
	jm_command_buffer* cb)
{
	rmt_BeginCPUSample(jm_command_buffer_sort, 0);

	for (size_t i = 0; i < cb->commandIt; ++i)
	{
		cb->indices[i] = i;
	}

	rmt_EndCPUSample();
}

jm_render_command_dispatcher get_command_dispatcher(const void* baseAddr)
{
	return *(jm_render_command_dispatcher*)baseAddr;
}

void jm_command_buffer_execute(
	jm_command_buffer* cb,
	jm_draw_context* ctx)
{
	rmt_BeginCPUSample(jm_command_buffer_execute, 0);

	for (int i = 0; i < cb->commandIt; ++i)
	{
		const size_t idx = cb->indices[i];
		const void* baseAddr = cb->commands[idx];
		const void* cmdAddr = (char*)baseAddr + sizeof(jm_render_command_dispatcher);

		jm_render_command_dispatcher dispatcher = get_command_dispatcher(baseAddr);
		dispatcher(ctx, cmdAddr);
	}

	rmt_EndCPUSample();
}

void* jm_command_buffer_push(
	jm_command_buffer* cb,
	size_t commandSize,
	jm_render_command_dispatcher dispatcher)
{
	jm_assert(cb->commandIt < cb->maxCommands);

	const size_t totalSize = sizeof(jm_render_command_dispatcher) + commandSize;

	void* baseAddr = cb->bufferIt;
	void* cmdAddr = cb->bufferIt + sizeof(jm_render_command_dispatcher);
	const void* auxAddr = cb->bufferIt + sizeof(jm_render_command_dispatcher) + commandSize;

	jm_assert((char*)baseAddr + totalSize < cb->buffer + cb->capacity);

	*(jm_render_command_dispatcher*)baseAddr = dispatcher;

	cb->keys[cb->commandIt] = 0; // todo
	cb->commands[cb->commandIt] = baseAddr;

	cb->bufferIt += totalSize;
	++cb->commandIt;

	memset(cmdAddr, 0, totalSize);

	return cmdAddr;
}

void* jm_command_buffer_alloc(
	jm_command_buffer* cb,
	size_t size)
{
	void* mem = cb->bufferIt;
	cb->bufferIt += size;
	return mem;
}