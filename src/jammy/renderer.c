#include "renderer.h"

#include <jammy/log.h>
#include <jammy/embedded_shaders.h>
#include <jammy/assert.h>

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
	ID3D11VertexShader* vertexShaders[JM_VERTEX_SHADER_COUNT];
	ID3D11PixelShader* pixelShaders[JM_PIXEL_SHADER_COUNT];

	ID3D11SamplerState* samplerStates[JM_SAMPLER_STATE_COUNT];

	ID3D11BlendState* blendStates[JM_BLEND_STATE_COUNT];
} jm_renderer;

static jm_renderer g_renderer;

static void _loadVertexShader(
	jm_vertex_shader shader, 
	const void* data, 
	size_t size)
{
	HRESULT hr = g_renderer.device->lpVtbl->CreateVertexShader(
		g_renderer.device, 
		data, 
		size,
		NULL, 
		&g_renderer.vertexShaders[shader]);
	jm_assert(SUCCEEDED(hr));
}

#define loadVertexShader(shader, data) \
	_loadVertexShader(shader, data, sizeof(data))

static void _loadPixelShader(
	jm_pixel_shader shader, 
	const void* data, 
	size_t size)
{
	HRESULT hr = g_renderer.device->lpVtbl->CreatePixelShader(
		g_renderer.device,
		data,
		size,
		NULL,
		&g_renderer.pixelShaders[shader]);
	jm_assert(SUCCEEDED(hr));
}

#define loadPixelShader(shader, data) \
	_loadPixelShader(shader, data, sizeof(data))

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

	loadVertexShader(JM_VERTEX_SHADER_COLOR, jm_embedded_vs_color);
	loadVertexShader(JM_VERTEX_SHADER_TEXTURE, jm_embedded_vs_texture);
	loadVertexShader(JM_VERTEX_SHADER_TEXT, jm_embedded_vs_text);

	loadPixelShader(JM_PIXEL_SHADER_COLOR, jm_embedded_ps_color);
	loadPixelShader(JM_PIXEL_SHADER_TEXTURE, jm_embedded_ps_texture);
	loadPixelShader(JM_PIXEL_SHADER_TEXT, jm_embedded_ps_text);

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
		dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
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

ID3D11InputLayout* jm_renderer_get_inputLayout(
	jm_input_layout inputLayout)
{
	return g_renderer.inputLayouts[inputLayout];
}

ID3D11VertexShader* jm_renderer_get_vs(
	jm_vertex_shader shader)
{
	return g_renderer.vertexShaders[shader];
}

ID3D11PixelShader* jm_renderer_get_ps(
	jm_pixel_shader shader)
{
	return g_renderer.pixelShaders[shader];
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