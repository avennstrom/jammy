#include <jammy/physics.h>
#include <jammy/lua/lua_util.h>

#include <lua.h>
#include <lauxlib.h>

static int __createRigidBody(lua_State* L)
{
	lua_pushliteral(L, "shape");
	lua_gettable(L, 1);
	const jm_shape_handle shapeHandle = (jm_shape_handle)lua_tointeger(L, -1);
	lua_pop(L, 1);

	jm_physics_material material;
	material.density = 1.0f;
	material.friction = 1.0f;
	material.restitution = 0.0f;

	jm_body_type bodyType = JM_BODY_TYPE_DYNAMIC;

	// read material
	lua_pushliteral(L, "material");
	lua_gettable(L, 1);
	if (lua_istable(L, -1))
	{
		lua_pushliteral(L, "friction");
		lua_gettable(L, -2);
		if (lua_isnumber(L, -1))
		{
			material.friction = lua_tonumber(L, -1);
		}
		lua_pop(L, 1);

		lua_pushliteral(L, "restitution");
		lua_gettable(L, -2);
		if (lua_isnumber(L, -1))
		{
			material.restitution = lua_tonumber(L, -1);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// read body type
	lua_pushliteral(L, "type");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_tointeger(L, -1))
		{
			luaL_argerror(L, 1, "parameter 'type' must be an integer");
		}
		bodyType = (jm_body_type)lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	// read position
	float x = 0.0f;
	float y = 0.0f;

	lua_pushliteral(L, "x");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "parameter 'type' must be a number");
		}
		x = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);
	lua_pushliteral(L, "y");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "parameter 'type' must be a number");
		}
		y = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	const jm_body_handle bodyHandle = jm_create_body(shapeHandle, &material, bodyType, x, y);
	lua_pushinteger(L, (lua_Integer)bodyHandle);
	return 1;
}

static int __createBoxShape(lua_State* L)
{
	const float width = luaL_checknumber(L, 1);
	const float height = luaL_checknumber(L, 2);
	jm_shape_handle shape = jm_create_box_shape(width, height);
	lua_pushinteger(L, shape);
	return 1;
}

static int __createCircleShape(lua_State* L)
{
	const float radius = luaL_checknumber(L, 1);
	jm_shape_handle shape = 0;//jm_create_circle_shape(g_physics, radius);
	lua_pushinteger(L, shape);
	return 1;
}

static int __getVelocity(lua_State* L)
{
	const jm_body_handle bodyHandle = (jm_body_handle)luaL_checkinteger(L, 1);
	float x, y;
	jm_get_velocity(bodyHandle, &x, &y);

	lua_newtable(L);
	jm_lua_table_setnumber(L, -1, "x", x);
	jm_lua_table_setnumber(L, -1, "y", y);
	return 1;
}

static int __setVelocity(lua_State* L)
{
	const jm_body_handle bodyHandle = (jm_body_handle)luaL_checkinteger(L, 1);
	const float x = luaL_checknumber(L, 2);
	const float y = luaL_checknumber(L, 3);
	jm_set_body_velocity(bodyHandle, x, y);
	return 0;
}

static int __getBodyPos(lua_State* L)
{
	const jm_body_handle bodyHandle = (jm_body_handle)luaL_checkinteger(L, 1);
	float x, y;
	jm_get_body_pos(bodyHandle, &x, &y);

	lua_newtable(L);
	jm_lua_table_setnumber(L, -1, "x", x);
	jm_lua_table_setnumber(L, -1, "y", y);
	return 1;
}

static int __setBodyPos(lua_State* L)
{
	const jm_body_handle bodyHandle = (jm_body_handle)luaL_checkinteger(L, 1);
	const float x = luaL_checknumber(L, 2);
	const float y = luaL_checknumber(L, 3);
	jm_set_body_pos(bodyHandle, x, y);
	return 0;
}

void jm_luaopen_physics(
	struct lua_State* L)
{
	lua_getglobal(L, "jam");

#define REG_FUNC(Name) \
	lua_pushliteral(L, #Name); \
	lua_pushcfunction(L, __##Name); \
	lua_settable(L, -3);

	REG_FUNC(createRigidBody);
	REG_FUNC(createBoxShape);
	REG_FUNC(getVelocity);
	REG_FUNC(setVelocity);
	REG_FUNC(getBodyPos);
	REG_FUNC(setBodyPos);
#undef REG_FUNC

	lua_pushliteral(L, "bodyType");
	lua_newtable(L);

	lua_pushliteral(L, "Dynamic");
	lua_pushinteger(L, JM_BODY_TYPE_DYNAMIC);
	lua_settable(L, -3);
	lua_pushliteral(L, "Kinematic");
	lua_pushinteger(L, JM_BODY_TYPE_KINEMATIC);
	lua_settable(L, -3);
	lua_pushliteral(L, "Static");
	lua_pushinteger(L, JM_BODY_TYPE_STATIC);
	lua_settable(L, -3);

	lua_settable(L, -3);

	lua_pop(L, 1);
}