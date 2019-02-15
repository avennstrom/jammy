#include "lua.h"

void jm_luaopen(lua_State* L)
{
	lua_newtable(L);
	lua_setglobal(L, "jam");
	
	jm_luaopen_graphics(L);
	jm_luaopen_audio(L);
	//jm_luaopen_physics(L);
	jm_luaopen_input(L);
}