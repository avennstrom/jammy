#pragma once

#include <inttypes.h>

#if defined(JM_WINDOWS)
#include <d3d11.h>
typedef ID3D11ShaderResourceView* jm_texture_resource;
typedef ID3D11Buffer* jm_buffer_resource;
#else 
#include <GL/glew.h>
typedef GLuint jm_texture_resource;
typedef GLuint jm_buffer_resource;
#endif

typedef enum jm_texture_format
{
	JM_TEXTURE_FORMAT_R8,
	JM_TEXTURE_FORMAT_R8G8B8A8,
} jm_texture_format;

typedef enum jm_shader_program
{
	JM_SHADER_PROGRAM_COLOR,
	JM_SHADER_PROGRAM_TEXTURE,
	JM_SHADER_PROGRAM_TEXT,
	JM_SHADER_PROGRAM_COUNT,
} jm_shader_program;

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

typedef struct jm_vertex
{
	float x, y;
} jm_vertex;

typedef struct jm_texcoord
{
	float u, v;
} jm_texcoord;