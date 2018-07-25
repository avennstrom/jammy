#pragma once

#include <lua.h>

void jm_lua_table_setnumber(
	lua_State* L, 
	int idx, 
	const char* field, 
	lua_Number value);