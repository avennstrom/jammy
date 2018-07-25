#include <jammy/lua/lua.h>

void jm_luaopen(struct lua_State* L)
{
	jm_luaopen_graphics(L);
	jm_luaopen_audio(L);
	jm_luaopen_physics(L);
}