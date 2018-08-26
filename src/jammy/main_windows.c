#if defined(JM_WINDOWS)
#include <jammy/command_buffer.h>
#include <jammy/file.h>
#include <jammy/renderer.h>
#include <jammy/audio.h>
#include <jammy/physics.h>
#include <jammy/build.h>
#include <jammy/assert.h>
#include <jammy/player_controller.h>
#include <jammy/remotery/Remotery.h>
#include <jammy/lua/lua.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <Windows.h>
#include <shellapi.h>
#include <d3d11.h>

#include <stdlib.h>

#define COMMAND_BUFFER_SIZE (2 * 1024 * 1024)
#define MAX_RENDER_COMMANDS 4096

#define TICK_RATE (1.0 / 60.0)

#if defined(JM_STANDALONE)
#define jm_lua_call lua_call
#else
static void jm_lua_call(
	lua_State* L,
	int nargs,
	int nresults)
{
	if (lua_pcall(L, nargs, nresults, 0))
	{
		const char* error = lua_tostring(L, -1);
		fprintf(stderr, "%s", error);
		MessageBox(NULL, error, "Lua error", MB_ICONERROR);
		ExitProcess(1);
	}
}
#endif

// lua2c entry point
#if defined(JM_STANDALONE)
extern int lcf_main(lua_State* L);
#endif

typedef struct jm_hwnd_userdata
{
	lua_State* L;
	int fnKeyDown;
	int fnKeyUp;
} jm_hwnd_userdata;

LRESULT CALLBACK jm_window_proc(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam);

typedef struct jm_render_thread_param
{
	uint32_t width;
	uint32_t height;
	uint32_t pixelScale;

	jm_command_buffer* commandBuffer;

	IDXGISwapChain* swapChain;
	ID3D11Device* d3ddevice;
	ID3D11DeviceContext* d3dctx;

	HANDLE commandBufferSubmitted;
	HANDLE commandBufferFilled;

	bool shouldContinue;
} jm_render_thread_param;

DWORD jm_render_thread_proc(
	const jm_render_thread_param* param);

#if defined(JM_STANDALONE)
INT CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
#if !defined(JM_STANDALONE)
	if (argc > 1 && strcmp(argv[1], "--build") == 0)
	{
		return jm_build(argc, argv);
	}
#endif

#if RMT_ENABLED
	Remotery* rmt;
#endif
	rmt_CreateGlobalInstance(&rmt);
	rmt_SetCurrentThreadName("Gameplay");

	const uint32_t width = 192 * 4;
	const uint32_t height = 192 * 4;
	const uint32_t pixelScale = 1;

	lua_State* L = luaL_newstate();

	lua_newtable(L);
	lua_pushliteral(L, "width");
	lua_pushinteger(L, width);
	lua_settable(L, -3);
	lua_pushliteral(L, "height");
	lua_pushinteger(L, height);
	lua_settable(L, -3);
	lua_setglobal(L, "jam");

	jm_command_buffer commandBuffers[2];
	jm_command_buffer_init(&commandBuffers[0], COMMAND_BUFFER_SIZE, MAX_RENDER_COMMANDS);
	jm_command_buffer_init(&commandBuffers[1], COMMAND_BUFFER_SIZE, MAX_RENDER_COMMANDS);

	size_t bufferIndex = 0;

	if (jm_textures_init())
	{
		fprintf(stderr, "jm_textures_init failed");
		return 1;
	}

	if (jm_fonts_init())
	{
		fprintf(stderr, "jm_fonts_init failed");
		return 1;
	}

	if (jm_effects_init())
	{
		fprintf(stderr, "jm_effects_init failed");
		return 1;
	}

	if (jm_physics_init())
	{
		fprintf(stderr, "jm_physics_init failed");
		return 1;
	}

	// setup lua state
	luaL_openlibs(L);
	jm_luaopen(L);

#if defined(JM_STANDALONE)
	lua_pushcfunction(L, lcf_main);
	jm_lua_call(L, 0, 0);
#else
	if (luaL_dofile(L, "jammy.lua"))
	{
		const char* error = lua_tostring(L, -1);
		fprintf(stderr, "%s\n", error);
		MessageBox(NULL, error, "Lua error", MB_ICONERROR);
		ExitProcess(1);
	}
#endif
	
	lua_getglobal(L, "keyDown");
	const int fnKeyDown = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_getglobal(L, "keyUp");
	const int fnKeyUp = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_getglobal(L, "tick");
	const int fnTick = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_getglobal(L, "draw");
	const int fnDraw = luaL_ref(L, LUA_REGISTRYINDEX);

	// get game name
	char gameName[64];
	lua_getglobal(L, "jam");
	lua_pushliteral(L, "name");
	lua_gettable(L, -2);
	if (lua_isstring(L, -1))
	{
		strcpy(gameName, lua_tostring(L, -1));
	}
	else
	{
		strcpy(gameName, "jammy");
	}
	lua_pop(L, 2);

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = gameName;
	wc.lpfnWndProc = jm_window_proc;
	RegisterClass(&wc);

	RECT wr;
	wr.left = 0;
	wr.top = 0;
	wr.right = width * pixelScale;
	wr.bottom = height * pixelScale;
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	jm_hwnd_userdata hwndUserdata;
	hwndUserdata.L = L;
	hwndUserdata.fnKeyDown = fnKeyDown;
	hwndUserdata.fnKeyUp = fnKeyUp;

#if !defined(JM_STANDALONE)
	HINSTANCE hInstance = GetModuleHandle(NULL);
#endif

	DWORD windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hwnd = CreateWindow(
		wc.lpszClassName,
		gameName,
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL,
		NULL,
		hInstance,
		&hwndUserdata);

	if (jm_renderer_init())
	{
		fprintf(stderr, "jm_renderer_init failed");
		return 1;
	}

	ID3D11Device* d3ddevice = jm_renderer_get_device();
	ID3D11DeviceContext* d3dctx = jm_renderer_get_context();

	IDXGISwapChain* swapChain;
	jm_create_swapchain(hwnd, width, height, &swapChain);

	// gotta do this here because DirectSound needs HWND
	if (jm_audio_init(hwnd))
	{
		fprintf(stderr, "jm_audio_init failed");
		return 1;
	}
	jm_luaopen_audio(L);

	// start render thread
	jm_render_thread_param renderThreadParam;
	renderThreadParam.width = width;
	renderThreadParam.height = height;
	renderThreadParam.pixelScale = pixelScale;

	renderThreadParam.commandBuffer = NULL;

	renderThreadParam.d3ddevice = d3ddevice;
	renderThreadParam.d3dctx = d3dctx;
	renderThreadParam.swapChain = swapChain;

	renderThreadParam.commandBufferSubmitted = CreateSemaphore(NULL, 0, 1, NULL);
	renderThreadParam.commandBufferFilled = CreateEvent(NULL, FALSE, FALSE, NULL);

	renderThreadParam.shouldContinue = true;
	HANDLE renderThread = CreateThread(NULL, 0, jm_render_thread_proc, &renderThreadParam, 0, NULL);

	ShowWindow(hwnd, SW_SHOW);

	lua_getglobal(L, "start");
	jm_lua_call(L, 0, 0);

	// todo - cross-platform
	LARGE_INTEGER timerFrequency, startTime, lastTime;
	QueryPerformanceFrequency(&timerFrequency);
	QueryPerformanceCounter(&startTime);
	lastTime = startTime;

	double tickTimer = 0.0;

	while (IsWindowVisible(hwnd))
	{
		rmt_BeginCPUSample(frame, 0);

		// flip command buffers
		bufferIndex = 1 - bufferIndex;

		MSG msg;
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// update audio
		jm_audio_update();

		// todo - cross-platform
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		const double duration = (double)(currentTime.QuadPart - startTime.QuadPart) / (double)timerFrequency.QuadPart;
		const double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)timerFrequency.QuadPart;
		lastTime = currentTime;

		lua_getglobal(L, "jam");

		// set time stuff
		lua_pushliteral(L, "elapsedTime");
		lua_pushnumber(L, (float)duration);
		lua_settable(L, -3);
		lua_pushliteral(L, "deltaTime");
		lua_pushnumber(L, (float)TICK_RATE);
		lua_settable(L, -3);
		lua_pop(L, 1);

		// set the current command buffer
		jm_set_current_command_buffer(&commandBuffers[bufferIndex]);

		// begin render command recording
		jm_command_buffer_begin(&commandBuffers[bufferIndex]);

		tickTimer += deltaTime;
		while (tickTimer >= TICK_RATE)
		{
			tickTimer -= TICK_RATE;

			// tick physics
			jm_physics_tick((float)TICK_RATE);

			rmt_BeginCPUSample(tick, 0);
			{
				// call game tick function
				lua_rawgeti(L, LUA_REGISTRYINDEX, fnTick);
				jm_lua_call(L, 0, 0);

				// signal the rendering thread
				rmt_EndCPUSample();
			}
		}

		rmt_BeginCPUSample(draw, 0);
		{
			// call game draw function
			lua_rawgeti(L, LUA_REGISTRYINDEX, fnDraw);
			jm_lua_call(L, 0, 0);

			// physics debug drawing
			//jm_physics_draw();

			rmt_EndCPUSample();
		}

		rmt_BeginCPUSample(wait, 0);
		{
			// wait until the previous command buffer has been submitted
			WaitForSingleObject(renderThreadParam.commandBufferSubmitted, INFINITE);

			rmt_EndCPUSample();
		}

		// tell the rendering thread which command buffer to submit
		renderThreadParam.commandBuffer = &commandBuffers[bufferIndex];

		// signal the rendering thread
		SetEvent(renderThreadParam.commandBufferFilled);

		rmt_EndCPUSample();
	}

	lua_close(L);

	renderThreadParam.shouldContinue = false;
	SetEvent(renderThreadParam.commandBufferFilled);
	WaitForSingleObject(renderThread, INFINITE);

	UnregisterClass(wc.lpszClassName, hInstance);

	rmt_DestroyGlobalInstance(rmt);

	return 0;
}

static LRESULT CALLBACK jm_window_proc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	jm_hwnd_userdata* userdata = (jm_hwnd_userdata*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* createstruct = (CREATESTRUCT*)lParam;
			userdata = createstruct->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)userdata);
			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			break;
		}

		case WM_KEYDOWN:
		{
			// ignore repeats
			//if ((lParam >> 24) > 0)
			//{
			//	return 0;
			//}

			if (userdata->fnKeyDown == LUA_REFNIL)
			{
				return 0;
			}

			lua_rawgeti(userdata->L, LUA_REGISTRYINDEX, userdata->fnKeyDown);
			lua_pushinteger(userdata->L, (lua_Integer)wParam);
			jm_lua_call(userdata->L, 1, 0);
			return 0;
		}

		case WM_KEYUP:
		{
			if (userdata->fnKeyUp == LUA_REFNIL)
			{
				return 0;
			}

			lua_rawgeti(userdata->L, LUA_REGISTRYINDEX, userdata->fnKeyUp);
			lua_pushinteger(userdata->L, (lua_Integer)wParam);
			jm_lua_call(userdata->L, 1, 0);
			return 0;
		}

		case WM_SYSKEYDOWN:
		{
			return 0;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static DWORD jm_render_thread_proc(
	const jm_render_thread_param* param)
{
	rmt_SetCurrentThreadName("Rendering");

	ID3D11Texture2D* backbuffer;
	if (FAILED(param->swapChain->lpVtbl->GetBuffer(param->swapChain, 0, &IID_ID3D11Texture2D, &backbuffer)))
	{
		fprintf(stderr, "IDXGISwapChain::GetBuffer failed");
		ExitProcess(1);
	}

	ID3D11RenderTargetView* backbufferRTV;
	if (FAILED(param->d3ddevice->lpVtbl->CreateRenderTargetView(param->d3ddevice, (ID3D11Resource*)backbuffer, NULL, &backbufferRTV)))
	{
		fprintf(stderr, "ID3D11Device::CreateRenderTargetView failed");
		ExitProcess(1);
	}

	D3D11_TEXTURE2D_DESC backbufferDesc;
	backbuffer->lpVtbl->GetDesc(backbuffer, &backbufferDesc);
	backbuffer->lpVtbl->Release(backbuffer);

	D3D11_TEXTURE2D_DESC depthBufferDesc;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.MiscFlags = 0;
	depthBufferDesc.SampleDesc = backbufferDesc.SampleDesc;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.Width = backbufferDesc.Width;
	depthBufferDesc.Height = backbufferDesc.Height;

	ID3D11Texture2D* depthBuffer;
	ID3D11DepthStencilView* depthBufferDSV;
	param->d3ddevice->lpVtbl->CreateTexture2D(param->d3ddevice, &depthBufferDesc, NULL, &depthBuffer);
	param->d3ddevice->lpVtbl->CreateDepthStencilView(param->d3ddevice, (ID3D11Resource*)depthBuffer, NULL, &depthBufferDSV);
	depthBuffer->lpVtbl->Release(depthBuffer);

	while (true)
	{
		rmt_BeginCPUSample(frame, 0);

		rmt_BeginCPUSample(wait, 0);
		{
			// tell the gameplay thread that the command buffer has been 
			// submitted and wait for the next command buffer to be filled
			SignalObjectAndWait(param->commandBufferSubmitted, param->commandBufferFilled, INFINITE, TRUE);

			rmt_EndCPUSample();
		}

		if (!param->shouldContinue)
		{
			break;
		}

		rmt_BeginCPUSample(present, 0);
		{
			param->swapChain->lpVtbl->Present(param->swapChain, 1, 0);
			rmt_EndCPUSample();
		}

		rmt_BeginCPUSample(command_submission, 0);
		{
			// sort render commands
			jm_command_buffer_sort(param->commandBuffer);

			const FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			param->d3dctx->lpVtbl->ClearRenderTargetView(param->d3dctx, backbufferRTV, clearColor);
			param->d3dctx->lpVtbl->ClearDepthStencilView(param->d3dctx, depthBufferDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

			param->d3dctx->lpVtbl->OMSetRenderTargets(param->d3dctx, 1, &backbufferRTV, depthBufferDSV);

			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = (float)param->width;
			viewport.Height = (float)param->height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			param->d3dctx->lpVtbl->RSSetViewports(param->d3dctx, 1, &viewport);

			param->d3dctx->lpVtbl->RSSetState(param->d3dctx, jm_renderer_get_rasterizer_state());
			param->d3dctx->lpVtbl->OMSetDepthStencilState(param->d3dctx, jm_renderer_get_depth_stencil_state(), 0);

			// set view constants
			typedef struct vs_constants
			{
				float viewProjectionMatrix[16];
			} vs_constants;

			ID3D11Buffer* vscb = jm_renderer_get_constant_buffer(JM_CONSTANT_BUFFER_PER_VIEW_VS);
			D3D11_MAPPED_SUBRESOURCE ms;
			if (SUCCEEDED(param->d3dctx->lpVtbl->Map(param->d3dctx, (ID3D11Resource*)vscb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
			{
				vs_constants* constants = ms.pData;
				memset(constants, 0, sizeof(vs_constants));

				float halfWidth = (float)param->width / 2.0f;
				float halfHeight = (float)param->height / 2.0f;

				constants->viewProjectionMatrix[0] = 1.0f / halfWidth;
				constants->viewProjectionMatrix[5] = -1.0f / halfHeight;
				constants->viewProjectionMatrix[10] = 1.0f;
				constants->viewProjectionMatrix[12] = -1.0f;
				constants->viewProjectionMatrix[13] = 1.0f;
				constants->viewProjectionMatrix[15] = 1.0f;

				param->d3dctx->lpVtbl->Unmap(param->d3dctx, (ID3D11Resource*)vscb, 0);
			}

			// execute render commands
			jm_draw_context drawContext;
			jm_draw_context_begin(
				&drawContext,
				param->d3dctx);
			jm_command_buffer_execute(param->commandBuffer, &drawContext);

			rmt_EndCPUSample();
		}

		rmt_EndCPUSample();
	}

	return 0;
}
#endif