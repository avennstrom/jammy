#pragma once

#include <inttypes.h>

#include <d3d11.h>

typedef enum jm_vertex_shader
{
	JM_VERTEX_SHADER_COLOR,
	JM_VERTEX_SHADER_TEXTURE,
	JM_VERTEX_SHADER_TEXT,
	JM_VERTEX_SHADER_COUNT,
} jm_vertex_shader;

typedef enum jm_pixel_shader
{
	JM_PIXEL_SHADER_COLOR,
	JM_PIXEL_SHADER_TEXTURE,
	JM_PIXEL_SHADER_TEXT,
	JM_PIXEL_SHADER_COUNT,
} jm_pixel_shader;

typedef enum jm_input_layout
{
	JM_INPUT_LAYOUT_POS,
	JM_INPUT_LAYOUT_POS_UV,
	JM_INPUT_LAYOUT_COUNT,
} jm_input_layout;

typedef enum jm_constant_buffer
{
	JM_CONSTANT_BUFFER_PER_VIEW_VS,
	JM_CONSTANT_BUFFER_PER_VIEW_PS,
	JM_CONSTANT_BUFFER_PER_INSTANCE_VS,
	JM_CONSTANT_BUFFER_PER_INSTANCE_PS,
	JM_CONSTANT_BUFFER_COUNT,
} jm_constant_buffer;

typedef enum jm_sampler_state
{
	JM_SAMPLER_STATE_POINT,
	JM_SAMPLER_STATE_LINEAR,
	JM_SAMPLER_STATE_COUNT,
} jm_sampler_state;

typedef enum jm_primitive_topology
{
	JM_PRIMITIVE_TOPOLOGY_LINELIST,
	JM_PRIMITIVE_TOPOLOGY_LINESTRIP,
	JM_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	JM_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
} jm_primitive_topology;

typedef enum jm_fill_mode
{
	JM_FILL_MODE_SOLID,
	JM_FILL_MODE_WIREFRAME,
} jm_fill_mode;

typedef enum jm_blend_state
{
	JM_BLEND_STATE_OPAQUE,
	JM_BLEND_STATE_TRANSPARENT, // forsenCD Clap
	JM_BLEND_STATE_ADDITIVE,
	JM_BLEND_STATE_COUNT,
} jm_blend_state;

int jm_renderer_init();

int jm_create_swapchain(
	HWND hwnd, 
	uint32_t width,
	uint32_t height,
	IDXGISwapChain** swapChain);

ID3D11Device* jm_renderer_get_device();

ID3D11DeviceContext* jm_renderer_get_context();

ID3D11InputLayout* jm_renderer_get_inputLayout(
	jm_input_layout inputLayout);

ID3D11VertexShader* jm_renderer_get_vs(
	jm_vertex_shader shader);

ID3D11PixelShader* jm_renderer_get_ps(
	jm_pixel_shader shader);

ID3D11Buffer* jm_renderer_get_constant_buffer(
	jm_constant_buffer cb);

ID3D11SamplerState* jm_renderer_get_sampler(
	jm_sampler_state samplerState);

ID3D11BlendState* jm_renderer_get_blend_state(
	jm_blend_state blendState);

ID3D11RasterizerState* jm_renderer_get_rasterizer_state();

ID3D11DepthStencilState* jm_renderer_get_depth_stencil_state();

ID3D11Buffer* jm_renderer_get_dynamic_vertex_buffer();

ID3D11Buffer* jm_renderer_get_dynamic_index_buffer();