#pragma once

#include <jammy/render_types.h>

int jm_renderer_init();

#if defined(JM_WINDOWS)
int jm_create_swapchain(
	HWND hwnd, 
	uint32_t width,
	uint32_t height,
	IDXGISwapChain** swapChain);
#endif

typedef struct jm_texture_resource_desc
{
	uint32_t width;
	uint32_t height;
	void* data;
} jm_texture_resource_desc;

void jm_renderer_create_texture_resource(
	const jm_texture_resource_desc* desc,
	jm_texture_resource* resource);

#if defined(JM_WINDOWS)
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
#endif

jm_buffer_resource jm_renderer_get_dynamic_vertex_buffer();
jm_buffer_resource jm_renderer_get_dynamic_index_buffer();

void jm_renderer_set_shader_program(
	jm_shader_program shaderProgram);

GLuint jm_renderer_get_shader_program(
	jm_shader_program shaderProgram);