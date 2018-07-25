#include "lua_util.h"

void jm_lua_table_setnumber(
	lua_State* L,
	int idx,
	const char* field,
	lua_Number value)
{
	lua_pushstring(L, field);
	lua_pushnumber(L, value);
	if (idx < 0)
	{
		idx -= 2;
	}
	lua_rawset(L, idx);
}