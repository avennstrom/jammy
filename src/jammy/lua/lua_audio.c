#include <jammy/audio.h>

#include <lua.h>
#include <lauxlib.h>

static int __loadSound(lua_State* L)
{
	const char* filename = luaL_checkstring(L, 1);
	const jm_sound_handle sound = jm_load_sound(filename);
	lua_pushinteger(L, (lua_Integer)sound);
	return 1;
}

static int __playSound(lua_State* L)
{
	const jm_sound_handle sound = (jm_sound_handle)luaL_checkinteger(L, 1);
	jm_audio_play(sound);
	return 0;
}

void jm_luaopen_audio(
	lua_State* L)
{
	lua_getglobal(L, "jam");

	lua_newtable(L);

	lua_pushliteral(L, "loadSound");
	lua_pushcfunction(L, __loadSound);
	lua_settable(L, -3);

	lua_pushliteral(L, "playSound");
	lua_pushcfunction(L, __playSound);
	lua_settable(L, -3);

	lua_pushliteral(L, "audio");
	lua_pushvalue(L, -2);
	lua_settable(L, -4);

	lua_pop(L, 1);
}