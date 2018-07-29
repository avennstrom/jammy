#if defined(JM_WINDOWS)
#include "render_commands.h"

#include <jammy/renderer.h>
#include <jammy/assert.h>
#include <jammy/color.h>
#include <jammy/remotery/Remotery.h>

#include <stdbool.h>

static const D3D11_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology[] = 
{
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};

void __jm_render_command_draw_text(
	jm_draw_context* ctx,
	const jm_render_command_draw_text* cmd)
{
	rmt_BeginCPUSample(__jm_render_command_draw_text, 0);

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

	const D3D11_MAP vbMapType = (vertexBufferOffset == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	const D3D11_MAP ibMapType = (indexBufferOffset == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

	ID3D11Buffer* dynamicVertexBuffer = jm_renderer_get_dynamic_vertex_buffer();
	ID3D11Buffer* indexBuffer = jm_renderer_get_dynamic_index_buffer();

	D3D11_MAPPED_SUBRESOURCE vertexBufferData;
	D3D11_MAPPED_SUBRESOURCE indexBufferData;

	ctx->d3dctx->lpVtbl->Map(ctx->d3dctx, (ID3D11Resource*)dynamicVertexBuffer, 0, vbMapType, 0, &vertexBufferData);
	ctx->d3dctx->lpVtbl->Map(ctx->d3dctx, (ID3D11Resource*)indexBuffer, 0, ibMapType, 0, &indexBufferData);

	jm_vertex* dstPosition = (jm_vertex*)((uint8_t*)vertexBufferData.pData + vertexBufferOffset + positionOffset);
	jm_texcoord* dstTexcoord = (jm_texcoord*)((uint8_t*)vertexBufferData.pData + vertexBufferOffset + texcoordOffset);
	uint16_t* dstIndex = (uint16_t*)((uint8_t*)indexBufferData.pData + indexBufferOffset);

	float penX = cmd->x;
	float penY = cmd->y;
	penY += lineHeight;

	

	ctx->d3dctx->lpVtbl->Unmap(ctx->d3dctx, (ID3D11Resource*)dynamicVertexBuffer, 0);
	ctx->d3dctx->lpVtbl->Unmap(ctx->d3dctx, (ID3D11Resource*)indexBuffer, 0);

	// bind shaders
	jm_input_layout inputLayout = JM_INPUT_LAYOUT_POS_UV;
	jm_vertex_shader vertexShader = JM_VERTEX_SHADER_TEXT;
	jm_pixel_shader pixelShader = JM_PIXEL_SHADER_TEXT;

	ID3D11InputLayout* d3dInputLayout = jm_renderer_get_inputLayout(inputLayout);
	ID3D11VertexShader* vs = jm_renderer_get_vs(vertexShader);
	ID3D11PixelShader* ps = jm_renderer_get_ps(pixelShader);
	ctx->d3dctx->lpVtbl->IASetInputLayout(ctx->d3dctx, d3dInputLayout);
	ctx->d3dctx->lpVtbl->VSSetShader(ctx->d3dctx, vs, NULL, 0);
	ctx->d3dctx->lpVtbl->PSSetShader(ctx->d3dctx, ps, NULL, 0);

	// set blend state
	jm_blend_state blendState = JM_BLEND_STATE_TRANSPARENT;
	ID3D11BlendState* d3dBlendState = jm_renderer_get_blend_state(blendState);

	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ctx->d3dctx->lpVtbl->OMSetBlendState(ctx->d3dctx, d3dBlendState, blendFactor, 0xff);

	ID3D11Buffer* const vscb[] = {
		jm_renderer_get_constant_buffer(JM_CONSTANT_BUFFER_PER_VIEW_VS)
	};
	ID3D11Buffer* const pscb[] = {
		jm_renderer_get_constant_buffer(JM_CONSTANT_BUFFER_PER_INSTANCE_PS)
	};

	// update constants
	D3D11_MAPPED_SUBRESOURCE ms;
	if (SUCCEEDED(ctx->d3dctx->lpVtbl->Map(ctx->d3dctx, (ID3D11Resource*)pscb[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
	{
		typedef struct constants
		{
			float r, g, b, a;
		} constants;
		constants* cb = (constants*)ms.pData;
		jm_unpack_color32_rgba_f32(cmd->color, &cb->r, &cb->g, &cb->b, &cb->a);

		ctx->d3dctx->lpVtbl->Unmap(ctx->d3dctx, (ID3D11Resource*)pscb[0], 0);
	}

	// bind constant buffers
	ctx->d3dctx->lpVtbl->VSSetConstantBuffers(ctx->d3dctx, 0, _countof(vscb), vscb);
	ctx->d3dctx->lpVtbl->PSSetConstantBuffers(ctx->d3dctx, 0, _countof(pscb), pscb);

	// setup input assembler
	ID3D11Buffer* const vertexBuffers[] = {
		dynamicVertexBuffer,
		dynamicVertexBuffer,
	};
	const uint32_t strides[] = {
		sizeof(jm_vertex), // pos
		sizeof(jm_texcoord), // uv
	};
	const uint32_t offsets[] = {
		vertexBufferOffset + positionOffset, // pos
		vertexBufferOffset + texcoordOffset, // uv
	};

	ctx->d3dctx->lpVtbl->IASetVertexBuffers(ctx->d3dctx, 0, _countof(vertexBuffers), vertexBuffers, strides, offsets);
	ctx->d3dctx->lpVtbl->IASetPrimitiveTopology(ctx->d3dctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// bind texture
	ID3D11ShaderResourceView* srv[] = { fontInfo->srv };
	ctx->d3dctx->lpVtbl->PSSetShaderResources(ctx->d3dctx, 0, _countof(srv), srv);
	// bind sampler
	ID3D11SamplerState* samplers[] = { jm_renderer_get_sampler(JM_SAMPLER_STATE_POINT) };
	ctx->d3dctx->lpVtbl->PSSetSamplers(ctx->d3dctx, 0, _countof(samplers), samplers);

	const DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
	ctx->d3dctx->lpVtbl->IASetIndexBuffer(ctx->d3dctx, indexBuffer, indexFormat, indexBufferOffset);

	ctx->d3dctx->lpVtbl->DrawIndexed(ctx->d3dctx, indexCount, 0, 0);

	rmt_EndCPUSample();
}

void __jm_render_command_draw(
	jm_draw_context* ctx,
	const jm_render_command_draw* cmd)
{
	rmt_BeginCPUSample(__jm_render_command_draw, 0);

	const bool isIndexed = cmd->indices != NULL;
	const bool isTextured = cmd->textureHandle != JM_TEXTURE_HANDLE_INVALID;
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

	D3D11_MAPPED_SUBRESOURCE ms;

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

	ID3D11Buffer* dynamicVertexBuffer = jm_renderer_get_dynamic_vertex_buffer();
	ID3D11Buffer* indexBuffer = jm_renderer_get_dynamic_index_buffer();

	const D3D11_MAP mapType = (vertexBufferOffset == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	ID3D11Resource* vertexBuffer = (ID3D11Resource*)dynamicVertexBuffer;
	if (SUCCEEDED(ctx->d3dctx->lpVtbl->Map(ctx->d3dctx, vertexBuffer, 0, mapType, 0, &ms)))
	{
		uint8_t* dstPosition = (uint8_t*)ms.pData + vertexBufferOffset + positionOffset;
		uint8_t* dstTexcoord = (uint8_t*)ms.pData + vertexBufferOffset + texcoordOffset;
		uint8_t* dstVertexColor = (uint8_t*)ms.pData + vertexBufferOffset + vertexColorOffset;

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
		ctx->d3dctx->lpVtbl->Unmap(ctx->d3dctx, vertexBuffer, 0);
	}

	// bind shaders
	jm_input_layout inputLayout = JM_INPUT_LAYOUT_POS;
	jm_vertex_shader vertexShader = JM_VERTEX_SHADER_COLOR;
	jm_pixel_shader pixelShader = JM_PIXEL_SHADER_COLOR;
	if (isTextured)
	{
		inputLayout = JM_INPUT_LAYOUT_POS_UV;
		vertexShader = JM_VERTEX_SHADER_TEXTURE;
		pixelShader = JM_PIXEL_SHADER_TEXTURE;
	}

	ID3D11InputLayout* d3dInputLayout = jm_renderer_get_inputLayout(inputLayout);
	ID3D11VertexShader* vs = jm_renderer_get_vs(vertexShader);
	ID3D11PixelShader* ps = jm_renderer_get_ps(pixelShader);
	ctx->d3dctx->lpVtbl->IASetInputLayout(ctx->d3dctx, d3dInputLayout);
	ctx->d3dctx->lpVtbl->VSSetShader(ctx->d3dctx, vs, NULL, 0);
	ctx->d3dctx->lpVtbl->PSSetShader(ctx->d3dctx, ps, NULL, 0);

	// set blend state
	jm_blend_state blendState = JM_BLEND_STATE_OPAQUE;
	if (isSemitransparent)
	{
		blendState = JM_BLEND_STATE_TRANSPARENT;
	}
	ID3D11BlendState* d3dBlendState = jm_renderer_get_blend_state(blendState);

	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ctx->d3dctx->lpVtbl->OMSetBlendState(ctx->d3dctx, d3dBlendState, blendFactor, 0xff);

	ID3D11Buffer* const vscb[] = {
		jm_renderer_get_constant_buffer(JM_CONSTANT_BUFFER_PER_VIEW_VS)
	};
	ID3D11Buffer* const pscb[] = {
		jm_renderer_get_constant_buffer(JM_CONSTANT_BUFFER_PER_INSTANCE_PS)
	};

	// update constants
	if (SUCCEEDED(ctx->d3dctx->lpVtbl->Map(ctx->d3dctx, (ID3D11Resource*)pscb[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
	{
		typedef struct constants
		{
			float r, g, b, a;
		} constants;
		constants* cb = (constants*)ms.pData;
		jm_unpack_color32_rgba_f32(cmd->color, &cb->r, &cb->g, &cb->b, &cb->a);

		ctx->d3dctx->lpVtbl->Unmap(ctx->d3dctx, (ID3D11Resource*)pscb[0], 0);
	}

	// bind constant buffers
	ctx->d3dctx->lpVtbl->VSSetConstantBuffers(ctx->d3dctx, 0, _countof(vscb), vscb);
	ctx->d3dctx->lpVtbl->PSSetConstantBuffers(ctx->d3dctx, 0, _countof(pscb), pscb);

	// setup input assembler
	ID3D11Buffer* const vertexBuffers[] = { 
		dynamicVertexBuffer, 
		dynamicVertexBuffer,
		dynamicVertexBuffer,
	};
	const uint32_t strides[] = { 
		sizeof(jm_vertex), // pos
		sizeof(jm_texcoord), // uv
		sizeof(jm_color32), // color
	};
	const uint32_t offsets[] = { 
		vertexBufferOffset + positionOffset, // pos
		vertexBufferOffset + texcoordOffset, // uv
		vertexBufferOffset + vertexColorOffset, // color
	};

	ctx->d3dctx->lpVtbl->IASetVertexBuffers(ctx->d3dctx, 0, _countof(vertexBuffers), vertexBuffers, strides, offsets);
	ctx->d3dctx->lpVtbl->IASetPrimitiveTopology(ctx->d3dctx, d3dPrimitiveTopology[cmd->topology]);

	if (isTextured)
	{
		// bind texture
		ID3D11ShaderResourceView* srv[] = { jm_texture_get_srv(cmd->textureHandle) };
		ctx->d3dctx->lpVtbl->PSSetShaderResources(ctx->d3dctx, 0, _countof(srv), srv);
		// bind sampler
		ID3D11SamplerState* samplers[] = { jm_renderer_get_sampler(cmd->samplerState) };
		ctx->d3dctx->lpVtbl->PSSetSamplers(ctx->d3dctx, 0, _countof(samplers), samplers);
	}

	if (isIndexed)
	{
		// fill index buffer
		const uint32_t indexBufferSize = sizeof(uint16_t) * cmd->indexCount;
		const uint32_t indexBufferOffset = ctx->indexBufferOffset;
		ctx->indexBufferOffset += indexBufferSize;

		const D3D11_MAP mapType = (indexBufferOffset == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
		if (SUCCEEDED(ctx->d3dctx->lpVtbl->Map(ctx->d3dctx, (ID3D11Resource*)indexBuffer, 0, mapType, 0, &ms)))
		{
			memcpy((char*)ms.pData + indexBufferOffset, cmd->indices, indexBufferSize);
			ctx->d3dctx->lpVtbl->Unmap(ctx->d3dctx, (ID3D11Resource*)indexBuffer, 0);
		}

		const DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;

		ctx->d3dctx->lpVtbl->IASetIndexBuffer(ctx->d3dctx, indexBuffer, indexFormat, indexBufferOffset);
		ctx->d3dctx->lpVtbl->DrawIndexed(ctx->d3dctx, cmd->indexCount, 0, 0);
	}
	else
	{
		ctx->d3dctx->lpVtbl->Draw(ctx->d3dctx, cmd->vertexCount, 0);
	}

	rmt_EndCPUSample();
}

void jm_draw_context_begin(
	jm_draw_context* ctx, 
	ID3D11DeviceContext* d3dctx)
{
	ctx->d3dctx = d3dctx;
	ctx->vertexBufferOffset = 0;
	ctx->indexBufferOffset = 0;
}
#endif