#if defined(JM_LINUX)
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

#include <X11/Xlib.h>
#include <GL/glew.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
 
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
		fprintf(stderr, "%s\n", error);
		exit(1);
	}
}
#endif

int main() 
{
    char exePath[256];
    readlink("/proc/self/exe", exePath, sizeof(exePath));
    char* lastDash = strrchr(exePath, '/');
    *lastDash = '\0';
    printf("%s\n", exePath);
    chdir(exePath);

#if RMT_ENABLED
	Remotery* rmt;
#endif
	rmt_CreateGlobalInstance(&rmt);
	rmt_SetCurrentThreadName("Gameplay");

	lua_State* L = luaL_newstate();

	jm_command_buffer commandBuffers[2];
	jm_command_buffer_init(&commandBuffers[0], COMMAND_BUFFER_SIZE, MAX_RENDER_COMMANDS);
	jm_command_buffer_init(&commandBuffers[1], COMMAND_BUFFER_SIZE, MAX_RENDER_COMMANDS);

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
		exit(1);
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

    static const char defaultGameName[] = "jammy";
    char* gameName;
    uint32_t width;
    uint32_t height;
	uint32_t pixelScale = 1;
    int vsync = true;

	lua_getglobal(L, "jam");
    {
        // get game name
        lua_pushliteral(L, "name");
        lua_gettable(L, -2);
        if (lua_isstring(L, -1))
        {
            gameName = malloc(strlen(lua_tostring(L, -1)) + 1);
            strcpy(gameName, lua_tostring(L, -1));
        }
        else
        {
            gameName = malloc(strlen(defaultGameName) + 1);
            strcpy(gameName, defaultGameName);
        }
        lua_pop(L, 1);
        // get dimensions
        lua_pushliteral(L, "graphics");
        lua_gettable(L, -2);
        {
            lua_pushliteral(L, "width");
            lua_gettable(L, -2);
            if (!lua_isnumber(L, -1))
            {
                fprintf(stderr, "please specify the game's resolution by setting jam.graphics.width and jam.graphics.height\n");
                exit(1);
            }
            width = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_pushliteral(L, "height");
            lua_gettable(L, -2);
            if (!lua_isnumber(L, -1))
            {
                fprintf(stderr, "please specify the game's resolution by setting jam.graphics.width and jam.graphics.height\n");
                exit(1);
            }
            height = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_pushliteral(L, "pixelScale");
            lua_gettable(L, -2);
            if (lua_isnumber(L, -1))
            {
                pixelScale = lua_tointeger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushliteral(L, "vsync");
            lua_gettable(L, -2);
            if (lua_isboolean(L, -1))
            {
                vsync = lua_toboolean(L, -1);
            }
            lua_pop(L, 1);

            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    
    int screenId = DefaultScreen(display);

    GLint majorGLX, minorGLX;
    glXQueryVersion(display, &majorGLX, &minorGLX);
    printf("GLX version %d.%d\n", majorGLX, minorGLX);

    GLint glxAttribs[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,   0,
        GLX_RED_SIZE,       8,
        GLX_GREEN_SIZE,     8,
        GLX_BLUE_SIZE,      8,
        GLX_SAMPLE_BUFFERS, 0,
        GLX_SAMPLES,        0,
        None
    };
    XVisualInfo* visual = glXChooseVisual(display, screenId, glxAttribs);

    XSetWindowAttributes windowAttribs;
    windowAttribs.border_pixel = BlackPixel(display, screenId);
    windowAttribs.background_pixel = WhitePixel(display, screenId);
    windowAttribs.override_redirect = True;
    windowAttribs.colormap = XCreateColormap(display, RootWindow(display, screenId), visual->visual, AllocNone);
    windowAttribs.event_mask = ExposureMask;
    Window window = XCreateWindow(
        display, 
        RootWindow(display, screenId),
        0,
        0, 
        width * pixelScale, 
        height * pixelScale, 
        0, 
        visual->depth, 
        InputOutput, 
        visual->visual, 
        CWBackPixel | CWColormap | CWBorderPixel | CWEventMask, 
        &windowAttribs);

    // set window title
    XStoreName(display, window, gameName);

    free(gameName);

    // prevent window resizing
    XSizeHints* sizeHints = XAllocSizeHints();
    sizeHints->flags = PMinSize | PMaxSize;
    sizeHints->min_width = sizeHints->max_width = width * pixelScale;
    sizeHints->min_height = sizeHints->max_height = height * pixelScale;
    XSetWMNormalHints(display, window, sizeHints);
    XFree(sizeHints);

    // create OpenGL context
    GLXContext context = glXCreateContext(display, visual, NULL, GL_TRUE);
    glXMakeCurrent(display, window, context);

    PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = glXGetProcAddress("glXSwapIntervalEXT");
    PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA = glXGetProcAddress("glXSwapIntervalMESA");
    if (glXSwapIntervalEXT)
    {
        glXSwapIntervalEXT(display, window, vsync);
    }
    else if (glXSwapIntervalMESA)
    {
        glXSwapIntervalMESA(vsync);
    }

    glewInit();

    printf("GL Vendor: %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("GL Version: %s\n", glGetString(GL_VERSION));
    printf("GL Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    if (jm_renderer_init())
	{
		fprintf(stderr, "jm_renderer_init failed");
		exit(1);
	}

    const long windowEventMask = KeyPressMask | KeyReleaseMask;
    XSelectInput(display, window, windowEventMask);
 
    // intercept WM_DELETE_WINDOW, which is fired when a user wants to close the window
    const Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, (Atom*)&WM_DELETE_WINDOW, 1);

    XMapWindow(display, window);

    lua_getglobal(L, "start");
	jm_lua_call(L, 0, 0);

    uint32_t bufferIndex = 0;

    struct timespec clockResolution;
    clock_getres(CLOCK_REALTIME, &clockResolution);
    struct timespec startTime;
    clock_gettime(CLOCK_REALTIME, &startTime);
    struct timespec lastTime = startTime;

    double tickTimer = 0.0;

    while (true) 
    {
        rmt_BeginCPUSample(tick, 0);

        bool shouldExit = false;
        while (XPending(display))
        {
            XEvent event;
            XNextEvent(display, &event);
            if (!XFilterEvent(&event, window))
            {
                switch (event.type)
                {
                    case ClientMessage:
                    {
                        if ((Atom)event.xclient.data.l[0] == WM_DELETE_WINDOW) 
                        {
                            // user wants to close the window
                            shouldExit = true;
                        }
                        break;
                    }

                    case KeyPress:
                    case KeyRelease:
                    {
                        const int fn = (event.type == KeyPress) ? fnKeyDown : fnKeyUp;
                        if (fn == LUA_REFNIL)
                        {
                            break;
                        }

                        const KeySym keySym = XLookupKeysym(&event.xkey, 0);
                        lua_rawgeti(L, LUA_REGISTRYINDEX, fn);
                        lua_pushinteger(L, (lua_Integer)keySym);
                        jm_lua_call(L, 1, 0);
                        break;
                    }
                }
            }
        }

        if (shouldExit)
        {
            break;
        }

        struct timespec currentTime;
        clock_gettime(CLOCK_REALTIME, &currentTime);

        const double s = (double)(currentTime.tv_sec - startTime.tv_sec);
        const double ns = (double)currentTime.tv_nsec / (double)clockResolution.tv_nsec;
        const double elapsedTime = s + (ns / 1000000000.0);
        const double currT = (double)(currentTime.tv_sec - lastTime.tv_sec) + ((double)currentTime.tv_nsec / 1000000000.0);
        const double prevT = ((double)lastTime.tv_nsec / 1000000000.0);
        const double deltaTime = currT - prevT;
        lastTime = currentTime;

        lua_getglobal(L, "jam");
        lua_pushliteral(L, "elapsedTime");
        lua_pushnumber(L, (lua_Number)elapsedTime);
        lua_settable(L, -3);
        lua_pushliteral(L, "deltaTime");
        lua_pushnumber(L, (lua_Number)TICK_RATE);
        lua_settable(L, -3);
        lua_pop(L, 1);

        // set the current command buffer
		jm_set_current_command_buffer(&commandBuffers[bufferIndex]);

        // begin render command recording
		jm_command_buffer_begin(&commandBuffers[bufferIndex]);

        // tick
        tickTimer += deltaTime;
		while (tickTimer >= TICK_RATE)
		{
			tickTimer -= TICK_RATE;

            // tick physics
            jm_physics_tick(TICK_RATE);

            // tick game
            rmt_BeginCPUSample(lua_tick, 0);
            lua_rawgeti(L, LUA_REGISTRYINDEX, fnTick);
            jm_lua_call(L, 0, 0);
            rmt_EndCPUSample();
        }

        // draw game
        rmt_BeginCPUSample(lua_draw, 0);
        lua_rawgeti(L, LUA_REGISTRYINDEX, fnDraw);
        jm_lua_call(L, 0, 0);
        rmt_EndCPUSample();

        // swap buffers
        rmt_BeginCPUSample(glXSwapBuffers, 0);
        glXSwapBuffers(display, window);
        rmt_EndCPUSample();

        // clear the backbuffer
        rmt_BeginCPUSample(clear, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        rmt_EndCPUSample();

        glViewport(0, 0, width * pixelScale, height * pixelScale);

        // execute render commands
        jm_draw_context drawContext;
        jm_draw_context_begin(&drawContext, NULL);
        jm_command_buffer_sort(&commandBuffers[bufferIndex]);
        jm_command_buffer_execute(&commandBuffers[bufferIndex], &drawContext);

        rmt_EndCPUSample();
    }

    glXDestroyContext(display, context);
 
    XFree(visual);
	XFreeColormap(display, windowAttribs.colormap);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
#endif