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

    const uint32_t width = 192 * 2;
	const uint32_t height = 192 * 2;
	const uint32_t pixelScale = 1;

	lua_State* L = luaL_newstate();

	lua_newtable(L);
	lua_pushliteral(L, "width");
	lua_pushinteger(L, 1);
	lua_settable(L, -3);
	lua_pushliteral(L, "height");
	lua_pushinteger(L, 1);
	lua_settable(L, -3);
	lua_setglobal(L, "jam");

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
        width, 
        height, 
        0, 
        visual->depth, 
        InputOutput, 
        visual->visual, 
        CWBackPixel | CWColormap | CWBorderPixel | CWEventMask, 
        &windowAttribs);

    XStoreName(display, window, gameName);

    GLXContext context = glXCreateContext(display, visual, NULL, GL_TRUE);
    glXMakeCurrent(display, window, context);

    glewInit();

    printf("GL Vendor: %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("GL Version: %s\n", glGetString(GL_VERSION));
    printf("GL Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));glewInit();

    if (jm_renderer_init())
	{
		fprintf(stderr, "jm_renderer_init failed");
		exit(1);
	}

    const long eventMask = ExposureMask | KeyPressMask;
    XSelectInput(display, window, eventMask);
    XMapWindow(display, window);
 
    lua_getglobal(L, "start");
	jm_lua_call(L, 0, 0);

    uint32_t bufferIndex = 0;

    struct timespec clockResolution;
    clock_getres(CLOCK_REALTIME, &clockResolution);

    struct timespec startTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    while (1) 
    {
        XEvent event;
        while (XCheckWindowEvent(display, window, eventMask, &event))
        {
            if (event.type == KeyPress)
            {
                break;
            }
        }

        struct timespec currentTime;
        clock_gettime(CLOCK_REALTIME, &currentTime);

        const double s = (double)(currentTime.tv_sec - startTime.tv_sec);
        const double ns = (double)currentTime.tv_nsec / (double)clockResolution.tv_nsec;
        const double elapsedTime = s + (ns / 1000000000.0);

        lua_getglobal(L, "jam");
        lua_pushliteral(L, "elapsedTime");
        lua_pushnumber(L, (lua_Number)elapsedTime);
        lua_settable(L, -3);
        lua_pop(L, 1);

        // set the current command buffer
		jm_set_current_command_buffer(&commandBuffers[bufferIndex]);

        // begin render command recording
		jm_command_buffer_begin(&commandBuffers[bufferIndex]);

        // call game tick function
        lua_rawgeti(L, LUA_REGISTRYINDEX, fnTick);
        jm_lua_call(L, 0, 0);

        // call game draw function
        lua_rawgeti(L, LUA_REGISTRYINDEX, fnDraw);
        jm_lua_call(L, 0, 0);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // execute render commands
        jm_draw_context drawContext;
        jm_draw_context_begin(
            &drawContext,
            NULL);
        jm_command_buffer_execute(&commandBuffers[bufferIndex], &drawContext);

        glXSwapBuffers(display, window);
    }

    glXDestroyContext(display, context);
 
    XFree(visual);
	XFreeColormap(display, windowAttribs.colormap);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
#endif