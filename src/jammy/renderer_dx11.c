#if defined(JM_WINDOWS)
#include "renderer.h"

#include <jammy/log.h>
#include <jammy/assert.h>

#include <jammy/shaders/dx11/color.vs.h>
#include <jammy/shaders/dx11/color.ps.h>
#include <jammy/shaders/dx11/texture.vs.h>
#include <jammy/shaders/dx11/texture.ps.h>
#include <jammy/shaders/dx11/text.vs.h>
#include <jammy/shaders/dx11/text.ps.h>

typedef enum jm_input_layout
{
	JM_INPUT_LAYOUT_POS,
	JM_INPUT_LAYOUT_POS_UV,
	JM_INPUT_LAYOUT_COUNT,
} jm_input_layout;

typedef struct jm_dx11_shader_program
{
	ID3D11VertexShader* vs;
	ID3D11PixelShader* ps;
	jm_input_layout inputLayout;
} jm_dx11_shader_program;

typedef struct jm_renderer
{
	ID3D11Device* device;
	ID3D11DeviceContext* context;

	ID3D11Buffer* dynamicVertexBuffer;
	ID3D11Buffer* dynamicIndexBuffer;

	ID3D11Buffer* constantBuffers[JM_CONSTANT_BUFFER_COUNT];

	ID3D11RasterizerState* rasterizerState;
	ID3D11DepthStencilState* depthStencilState;

	ID3D11InputLayout* inputLayouts[JM_INPUT_LAYOUT_COUNT];
	jm_dx11_shader_program shaderPrograms[JM_SHADER_PROGRAM_COUNT];

	ID3D11SamplerState* samplerStates[JM_SAMPLER_STATE_COUNT];

	ID3D11BlendState* blendStates[JM_BLEND_STATE_COUNT];
} jm_renderer;

static jm_renderer g_renderer;

static void jm_create_shader_program(
	jm_shader_program shaderProgram, 
	jm_input_layout inputLayout,
	const void* vsData, 
	size_t vsSize,
	const void* psData,
	size_t psSize)
{
	HRESULT hr;
	
	// create vertex shader
	hr = g_renderer.device->lpVtbl->CreateVertexShader(
		g_renderer.device, 
		vsData, 
		vsSize,
		NULL, 
		&g_renderer.shaderPrograms[shaderProgram].vs);
	jm_assert(SUCCEEDED(hr));

	// create pixel shader
	hr = g_renderer.device->lpVtbl->CreatePixelShader(
		g_renderer.device,
		psData,
		psSize,
		NULL,
		&g_renderer.shaderPrograms[shaderProgram].ps);
	jm_assert(SUCCEEDED(hr));

	g_renderer.shaderPrograms[shaderProgram].inputLayout = inputLayout;
}

int jm_renderer_init()
{
	const D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
	};

	UINT deviceFlags = 0;
#if defined(JM_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		deviceFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		&g_renderer.device,
		NULL,
		&g_renderer.context)))
	{
		return 1;
	}

	jm_create_shader_program(
		JM_SHADER_PROGRAM_COLOR,
		JM_INPUT_LAYOUT_POS,
		jm_embedded_vs_color,
		sizeof(jm_embedded_vs_color),
		jm_embedded_ps_color,
		sizeof(jm_embedded_ps_color));

	jm_create_shader_program(
		JM_SHADER_PROGRAM_TEXTURE,
		JM_INPUT_LAYOUT_POS_UV,
		jm_embedded_vs_texture,
		sizeof(jm_embedded_vs_texture),
		jm_embedded_ps_texture,
		sizeof(jm_embedded_ps_texture));

	jm_create_shader_program(
		JM_SHADER_PROGRAM_TEXT,
		JM_INPUT_LAYOUT_POS_UV,
		jm_embedded_vs_text,
		sizeof(jm_embedded_vs_text),
		jm_embedded_ps_text,
		sizeof(jm_embedded_ps_text));

	{
		const D3D11_INPUT_ELEMENT_DESC elements[] = {
			{ "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		g_renderer.device->lpVtbl->CreateInputLayout(
			g_renderer.device, 
			elements, 
			_countof(elements), 
			jm_embedded_vs_color, 
			sizeof(jm_embedded_vs_color), 
			&g_renderer.inputLayouts[JM_INPUT_LAYOUT_POS]);
	}
	{
		const D3D11_INPUT_ELEMENT_DESC elements[] = {
			{ "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		g_renderer.device->lpVtbl->CreateInputLayout(
			g_renderer.device, 
			elements, 
			_countof(elements), 
			jm_embedded_vs_texture, 
			sizeof(jm_embedded_vs_texture),
			&g_renderer.inputLayouts[JM_INPUT_LAYOUT_POS_UV]);
	}

	{
		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = 32 * 1024 * 1024;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;

		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		g_renderer.device->lpVtbl->CreateBuffer(g_renderer.device, &bd, NULL, &g_renderer.dynamicVertexBuffer);

		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		g_renderer.device->lpVtbl->CreateBuffer(g_renderer.device, &bd, NULL, &g_renderer.dynamicIndexBuffer);
	}

	// create constant buffers
	{
		D3D11_BUFFER_DESC bd;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = 64;

		g_renderer.device->lpVtbl->CreateBuffer(g_renderer.device, &bd, NULL, &g_renderer.constantBuffers[JM_CONSTANT_BUFFER_PER_VIEW_VS]);
		g_renderer.device->lpVtbl->CreateBuffer(g_renderer.device, &bd, NULL, &g_renderer.constantBuffers[JM_CONSTANT_BUFFER_PER_VIEW_PS]);
		g_renderer.device->lpVtbl->CreateBuffer(g_renderer.device, &bd, NULL, &g_renderer.constantBuffers[JM_CONSTANT_BUFFER_PER_INSTANCE_VS]);
		g_renderer.device->lpVtbl->CreateBuffer(g_renderer.device, &bd, NULL, &g_renderer.constantBuffers[JM_CONSTANT_BUFFER_PER_INSTANCE_PS]);
	}

	{
		D3D11_RASTERIZER_DESC rd;
		ZeroMemory(&rd, sizeof(rd));
		rd.AntialiasedLineEnable = TRUE;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = FALSE;
		rd.FillMode = D3D11_FILL_SOLID;
		rd.MultisampleEnable = TRUE;
		rd.ScissorEnable = FALSE;
		g_renderer.device->lpVtbl->CreateRasterizerState(g_renderer.device, &rd, &g_renderer.rasterizerState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC dsd;
		ZeroMemory(&dsd, sizeof(dsd));
		dsd.DepthEnable = TRUE;
		dsd.DepthFunc = D3D11_COMPARISON_LESS;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsd.StencilEnable = FALSE;
		g_renderer.device->lpVtbl->CreateDepthStencilState(g_renderer.device, &dsd, &g_renderer.depthStencilState);
	}

	// create samplers
	{
		D3D11_SAMPLER_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		g_renderer.device->lpVtbl->CreateSamplerState(g_renderer.device, &sd, &g_renderer.samplerStates[JM_SAMPLER_STATE_POINT]);
	}
	{
		D3D11_SAMPLER_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		g_renderer.device->lpVtbl->CreateSamplerState(g_renderer.device, &sd, &g_renderer.samplerStates[JM_SAMPLER_STATE_LINEAR]);
	}

	// create blend states
	{
		D3D11_BLEND_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.RenderTarget[0].BlendEnable = TRUE;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].RenderTargetWriteMask = 0xf;
		g_renderer.device->lpVtbl->CreateBlendState(g_renderer.device, &bd, &g_renderer.blendStates[JM_BLEND_STATE_OPAQUE]);
	}
	{
		D3D11_BLEND_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.RenderTarget[0].BlendEnable = TRUE;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].RenderTargetWriteMask = 0xf;
		g_renderer.device->lpVtbl->CreateBlendState(g_renderer.device, &bd, &g_renderer.blendStates[JM_BLEND_STATE_TRANSPARENT]);
	}

	return 0;
}

int jm_create_swapchain(
	HWND hwnd, 
	uint32_t width,
	uint32_t height,
	IDXGISwapChain** swapChain)
{
	IDXGIFactory* dxgiFactory;
	if (FAILED(CreateDXGIFactory(&IID_IDXGIFactory, &dxgiFactory)))
	{
		jm_log_info("CreateDXGIFactory failed");
		return 1;
	}

	DXGI_SWAP_CHAIN_DESC desc;
	desc.BufferCount = 1;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Width = width;
	desc.BufferDesc.Height = height;
	desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags = 0;
	desc.OutputWindow = hwnd;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Windowed = TRUE;

	if (FAILED(dxgiFactory->lpVtbl->CreateSwapChain(dxgiFactory, (IUnknown*)g_renderer.device, &desc, swapChain)))
	{
		jm_log_info("CreateDXGIFactory failed");
		return 1;
	}

	dxgiFactory->lpVtbl->Release(dxgiFactory);

	return 0;
}

ID3D11Device* jm_renderer_get_device()
{
	return g_renderer.device;
}

ID3D11DeviceContext* jm_renderer_get_context()
{
	return g_renderer.context;
}

ID3D11Buffer* jm_renderer_get_constant_buffer(
	jm_constant_buffer cb)
{
	return g_renderer.constantBuffers[cb];
}

ID3D11SamplerState* jm_renderer_get_sampler(
	jm_sampler_state samplerState)
{
	return g_renderer.samplerStates[samplerState];
}

ID3D11BlendState* jm_renderer_get_blend_state(
	jm_blend_state blendState)
{
	return g_renderer.blendStates[blendState];
}

ID3D11RasterizerState* jm_renderer_get_rasterizer_state()
{
	return g_renderer.rasterizerState;
}

ID3D11DepthStencilState* jm_renderer_get_depth_stencil_state()
{
	return g_renderer.depthStencilState;
}

ID3D11Buffer* jm_renderer_get_dynamic_vertex_buffer()
{
	return g_renderer.dynamicVertexBuffer;
}

ID3D11Buffer* jm_renderer_get_dynamic_index_buffer()
{
	return g_renderer.dynamicIndexBuffer;
}

void jm_renderer_set_shader_program(
	jm_shader_program shaderProgram)
{
	const jm_dx11_shader_program* dx11Program = &g_renderer.shaderPrograms[shaderProgram];
	g_renderer.context->lpVtbl->IASetInputLayout(g_renderer.context, g_renderer.inputLayouts[dx11Program->inputLayout]);
	g_renderer.context->lpVtbl->VSSetShader(g_renderer.context, dx11Program->vs, NULL, 0);
	g_renderer.context->lpVtbl->PSSetShader(g_renderer.context, dx11Program->ps, NULL, 0);
}

static const DXGI_FORMAT g_formatDXGIFormat[] = {
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM,
};

static const uint32_t g_formatBPP[] = {
	1,
	4,
};

void jm_renderer_create_texture_resource(
	const jm_texture_resource_desc* desc,
	jm_texture_resource* resource)
{
	jm_assert(desc->name != NULL);
	const uint32_t dataPitch = desc->width * g_formatBPP[desc->format];

	D3D11_TEXTURE2D_DESC d3ddesc;
	d3ddesc.ArraySize = 1;
	d3ddesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	d3ddesc.CPUAccessFlags = 0;
	d3ddesc.Format = g_formatDXGIFormat[desc->format];
	d3ddesc.MipLevels = 1;
	d3ddesc.MiscFlags = 0;
	d3ddesc.SampleDesc.Count = 1;
	d3ddesc.SampleDesc.Quality = 0;
	d3ddesc.Usage = D3D11_USAGE_IMMUTABLE;
	d3ddesc.Width = desc->width;
	d3ddesc.Height = desc->height;

	D3D11_SUBRESOURCE_DATA sd;
	sd.pSysMem = desc->data;
	sd.SysMemPitch = dataPitch;
	sd.SysMemSlicePitch = 0;

	ID3D11Device* d3ddev = jm_renderer_get_device();

	ID3D11Texture2D* texture;
	d3ddev->lpVtbl->CreateTexture2D(d3ddev, &d3ddesc, &sd, &texture);
	d3ddev->lpVtbl->CreateShaderResourceView(d3ddev, (ID3D11Resource*)texture, NULL, resource);
	texture->lpVtbl->Release(texture);

#if defined(JM_DEBUG)
	texture->lpVtbl->SetPrivateData(texture, &WKPDID_D3DDebugObjectName, (UINT)strlen(desc->name), desc->name);
#endif
}

void jm_renderer_destroy_texture_resource(
	jm_texture_resource resource)
{
	resource->lpVtbl->Release(resource);
}
#endif