#if defined(JM_LINUX)
#include "render_commands.h"

#include <jammy/renderer.h>
#include <jammy/assert.h>
#include <jammy/color.h>

#include <GL/glew.h>

#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

static const GLenum glmode[] = 
{
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
};

void __jm_render_command_draw_text(
	jm_draw_context* ctx,
	const jm_render_command_draw_text* cmd)
{
    jm_assert(cmd->fontHandle != JM_FONT_HANDLE_INVALID);
	jm_assert(cmd->text);

	const size_t textLength = strlen(cmd->text);

	// fill buffers
	const uint32_t vertexCount = (uint32_t)textLength * 4;
	uint32_t vertexDataSize = 0;
	// pos
	uint32_t positionOffset = vertexDataSize;
	vertexDataSize += sizeof(jm_vertex) * vertexCount;
	// uv
	uint32_t texcoordOffset = vertexDataSize;
	vertexDataSize += sizeof(jm_texcoord) * vertexCount;

	const uint32_t vertexBufferOffset = ctx->vertexBufferOffset;
	ctx->vertexBufferOffset += vertexDataSize;

	const uint32_t optimisticIndexCount = (uint32_t)textLength * 5;
	const uint32_t indexDataSize = optimisticIndexCount * sizeof(uint16_t);
	const uint32_t indexBufferOffset = ctx->indexBufferOffset;
	ctx->indexBufferOffset += indexDataSize;

    const GLuint vertexBuffer = jm_renderer_get_dynamic_vertex_buffer();
    const GLuint indexBuffer = jm_renderer_get_dynamic_index_buffer();

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    void* dstVertices = glMapBufferRange(GL_ARRAY_BUFFER, vertexBufferOffset, vertexDataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
    void* dstIndices = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, indexBufferOffset, indexDataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

    jm_vertex* dstPosition = (jm_vertex*)((char*)dstVertices + positionOffset);
    jm_texcoord* dstTexcoord = (jm_texcoord*)((char*)dstVertices + positionOffset);

    uint32_t indexCount;
    jm_font_get_text_vertices(
        cmd->fontHandle,
        cmd->text,
        cmd->x,
        cmd->y,
        cmd->width,
        cmd->rangeStart,
        cmd->rangeEnd,
        cmd->scale,
        dstPosition,
        dstTexcoord,
        (uint16_t*)dstIndices,
        &indexCount);

    glUnmapBuffer(GL_ARRAY_BUFFER);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_SHORT, (const void*)(size_t)indexBufferOffset);
}

void __jm_render_command_draw(
	jm_draw_context* ctx,
	const jm_render_command_draw* cmd)
{
    const bool isIndexed = cmd->indices != NULL;
	const bool isTextured = cmd->texcoords != NULL && cmd->textureHandle != JM_TEXTURE_HANDLE_INVALID;
	const bool isVertexColor = false;//todo

	bool isSemitransparent = false;
	const uint8_t alpha = (cmd->color >> 24);
	if (alpha > 0x00 && alpha < 0xff)
	{
		// color is semitransparent
		isSemitransparent = true;
	}
	else if (isTextured && jm_texture_isSemitransparent(cmd->textureHandle))
	{
		// texture has semitransparent pixels
		isSemitransparent = true;
	}

	// fill vertex buffer
	uint32_t vertexDataSize = sizeof(jm_vertex) * cmd->vertexCount;
	uint32_t positionOffset = 0;
	uint32_t texcoordOffset = 0;
	uint32_t vertexColorOffset = 0;
	if (isTextured)
	{
		texcoordOffset = vertexDataSize;
		vertexDataSize += sizeof(jm_texcoord) * cmd->vertexCount;
	}
	if (isVertexColor)
	{
		vertexColorOffset = vertexDataSize;
		vertexDataSize += sizeof(jm_color32) * cmd->vertexCount;
	}

	const uint32_t vertexBufferOffset = ctx->vertexBufferOffset;
	ctx->vertexBufferOffset += vertexDataSize;

	const GLuint vertexBuffer = jm_renderer_get_dynamic_vertex_buffer();
    const GLuint indexBuffer = jm_renderer_get_dynamic_index_buffer();

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	{
		//printf("%u, %u\n", vertexBufferOffset, vertexDataSize);
        void* vertexBufferData = glMapBufferRange(GL_ARRAY_BUFFER, vertexBufferOffset, vertexDataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
		void* dstPosition = (uint8_t*)vertexBufferData + positionOffset;
		void* dstTexcoord = (uint8_t*)vertexBufferData + texcoordOffset;
		void* dstVertexColor = (uint8_t*)vertexBufferData + vertexColorOffset;

		// copy position
		memcpy(dstPosition, cmd->vertices, vertexDataSize);
		// copy texcoord
		if (isTextured)
		{
			memcpy(dstTexcoord, cmd->texcoords, vertexDataSize);
		}
		// copy color
		if (isVertexColor)
		{
			memcpy(dstVertexColor, NULL, vertexDataSize);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	// set shader
	jm_shader_program shaderProgram = JM_SHADER_PROGRAM_COLOR;
	if (isTextured)
	{
		shaderProgram = JM_SHADER_PROGRAM_TEXTURE;
	}
	jm_renderer_set_shader_program(shaderProgram);

	// set blend state
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
	if (isSemitransparent)
	{
        glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
        glDepthMask(GL_FALSE);
	}
    else
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

	// update uniforms
	const GLuint colorUniformLocation = jm_renderer_get_uniform_location(shaderProgram, "g_color");
	const GLuint matWorldViewProjLocation = jm_renderer_get_uniform_location(shaderProgram, "g_matWorldViewProj");

	float r, g, b, a;
	jm_unpack_color32_rgba_f32(cmd->color, &r, &g, &b, &a);
	glUniform4f(colorUniformLocation, r, g, b, a);
	glUniformMatrix4fv(matWorldViewProjLocation, 1, GL_FALSE, cmd->transform);

	// set positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)(size_t)(vertexBufferOffset + positionOffset));

	if (isTextured)
	{
		// bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, jm_texture_get_resource(cmd->textureHandle));

		// set texcoords
		glEnableVertexAttribArray(1);
    	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(size_t)(vertexBufferOffset + texcoordOffset));
	}
	else
	{
		glDisableVertexAttribArray(1);
	}

    const GLenum drawMode = glmode[cmd->topology];
	if (isIndexed)
	{
		// fill index buffer
		const uint32_t indexBufferSize = sizeof(uint16_t) * cmd->indexCount;
		const uint32_t indexBufferOffset = ctx->indexBufferOffset;
		ctx->indexBufferOffset += indexBufferSize;

		{
			//printf("%u, %u\n", indexBufferOffset, indexBufferSize);
            void* indexBufferData = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, indexBufferOffset, indexBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(indexBufferData, cmd->indices, indexBufferSize);
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}

		const GLenum indexFormat = GL_UNSIGNED_SHORT;

		//glEnable(GL_PRIMITIVE_RESTART);
		//glPrimitiveRestartIndex(0xffff);
        glDrawElements(drawMode, cmd->indexCount, indexFormat, (const void*)(size_t)indexBufferOffset);
	}
	else
	{
        glDrawArrays(drawMode, 0, cmd->vertexCount);
	}
}

void jm_draw_context_begin(
	jm_draw_context* ctx, 
	void* platformContext)
{
	ctx->vertexBufferOffset = 0;
	ctx->indexBufferOffset = 0;
}
#endif