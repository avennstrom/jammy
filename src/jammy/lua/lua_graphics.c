#include <jammy/command_buffer.h>
#include <jammy/texture.h>
#include <jammy/font.h>
#include <jammy/effect.h>
#include <jammy/math.h>
#include <jammy/remotery/Remotery.h>
#include <jammy/color.h>

#include <lua.h>
#include <lauxlib.h>

#include <string.h>

typedef struct jm_lua_texture
{
	jm_texture_handle handle;
} jm_lua_texture;

static jm_lua_texture* lua_checkTexture(lua_State* L, int index)
{
	luaL_checktype(L, index, LUA_TUSERDATA);
	jm_lua_texture* texture = (jm_lua_texture*)luaL_checkudata(L, index, "Texture");
	if (texture == NULL) luaL_typerror(L, index, "Texture");
	return texture;
}

static jm_lua_texture* lua_pushTexture(lua_State* L)
{
	jm_lua_texture* texture = (jm_lua_texture*)lua_newuserdata(L, sizeof(jm_lua_texture));
	luaL_getmetatable(L, "Texture");
	lua_setmetatable(L, -2);
	return texture;
}

static jm_lua_texture* lua_toTexture(lua_State* L, int index)
{
	jm_lua_texture* texture = (jm_lua_texture*)lua_touserdata(L, index);
	if (texture == NULL) luaL_typerror(L, index, "Texture");
	return texture;
}

static bool lua_isTexture(lua_State* L, int index)
{
	jm_lua_texture* texture = (jm_lua_texture*)luaL_checkudata(L, index, "Texture");
	return (texture != NULL);
}

static int lua_Texture___gc(lua_State* L)
{
	jm_lua_texture* texture = lua_checkTexture(L, 1);
	jm_destroy_texture(texture->handle);
	return 0;
}

static float g_cameraTransform[16];

static int __drawText(lua_State* L)
{
	if (!lua_istable(L, 1))
	{
		luaL_argerror(L, 1, "");
	}

	jm_render_command_draw_text* cmd = JM_COMMAND_BUFFER_PUSH(g_currentCommandBuffer, jm_render_command_draw_text);
	jm_render_command_draw_text_init(cmd);

	// get font
	lua_pushliteral(L, "font");
	lua_gettable(L, 1);
	if (!lua_isnumber(L, -1))
	{
		luaL_argerror(L, 1, "the 'font' parameter must be an integer");
	}
	cmd->fontHandle = (jm_font_handle)lua_tointeger(L, -1);
	lua_pop(L, 1);

	// get text
	lua_pushliteral(L, "text");
	lua_gettable(L, 1);
	if (!lua_isstring(L, -1))
	{
		luaL_argerror(L, 1, "the 'text' parameter must be a string");
	}
	const char* text = lua_tostring(L, -1);
	cmd->text = jm_command_buffer_alloc(g_currentCommandBuffer, strlen(text) + 1);
	strcpy(cmd->text, text);
	lua_pop(L, 1);

	// override color
	lua_pushliteral(L, "color");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_istable(L, -1))
		{
			luaL_argerror(L, 1, "the 'color' parameter must be an array containing the red, green, blue, and alpha value of the desired color");
		}

		lua_rawgeti(L, -1, 1);
		const float r = jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 2);
		const float g = jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 3);
		const float b = jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 4);
		const float a = lua_isnil(L, -1) ? 1.0f : jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);

		cmd->color = jm_pack_color32_rgba_f32(r, g, b, a);
	}
	lua_pop(L, 1);

	// override x
	lua_pushliteral(L, "x");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'x' parameter must be a number");
		}
		cmd->x = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	// override y
	lua_pushliteral(L, "y");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'y' parameter must be a number");
		}
		cmd->y = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	// override width
	lua_pushliteral(L, "width");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'width' parameter must be a number");
		}
		cmd->width = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	// override scale
	lua_pushliteral(L, "scale");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'scale' parameter must be a number");
		}
		cmd->scale = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	// override line spacing factor
	lua_pushliteral(L, "lineSpacingFactor");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'lineSpacingFactor' parameter must be a number");
		}
		cmd->lineSpacingMultiplier = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	// override range
	lua_pushliteral(L, "range");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'rangeEnd' parameter must be a number");
		}
		cmd->rangeEnd = (uint32_t)lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	return 0;
}

static int __draw(lua_State* L)
{
	if (!lua_istable(L, 1))
	{
		luaL_argerror(L, 1, "");
	}

	// get vertices
	lua_pushliteral(L, "vertices");
	lua_gettable(L, 1);

	if (!lua_istable(L, -1))
	{
		luaL_argerror(L, 1, "the 'vertices' parameter must be an array");
	}

	if (lua_objlen(L, -1) == 0)
	{
		lua_pop(L, 2);
		return 0;
	}

	jm_render_command_draw* cmd = JM_COMMAND_BUFFER_PUSH(g_currentCommandBuffer, jm_render_command_draw);
	jm_render_command_draw_init(cmd);

	cmd->vertexCount = (uint32_t)lua_objlen(L, -1);
	cmd->vertices = jm_command_buffer_alloc(g_currentCommandBuffer, cmd->vertexCount * sizeof(jm_vertex));

	jm_vertex* vertices = (jm_vertex*)cmd->vertices;
	for (uint32_t i = 0; i < cmd->vertexCount; ++i)
	{
		jm_vertex* vtx = vertices + i;

		lua_rawgeti(L, -1, i + 1);
		if (!lua_istable(L, -1))
		{
			luaL_argerror(L, 1, "every element of the 'vertices' array must be a table");
		}
		
		lua_rawgeti(L, -1, 1);
		vtx->x = lua_tonumber(L, -1);
		lua_rawgeti(L, -2, 2);
		vtx->y = lua_tonumber(L, -1);
		lua_pop(L, 3);
	}
	lua_pop(L, 1);

	// get texcoords
	lua_pushliteral(L, "texcoords");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_istable(L, -1))
		{
			luaL_argerror(L, 1, "the 'texcoords' parameter must be an array");
		}

		jm_assert(lua_objlen(L, -1) == cmd->vertexCount);
		cmd->texcoords = jm_command_buffer_alloc(g_currentCommandBuffer, cmd->vertexCount * sizeof(jm_texcoord));

		jm_texcoord* texcoords = (jm_texcoord*)cmd->texcoords;
		for (uint32_t i = 0; i < cmd->vertexCount; ++i)
		{
			jm_texcoord* uv = texcoords + i;

			lua_rawgeti(L, -1, i + 1);

			lua_rawgeti(L, -1, 1);
			uv->u = lua_tonumber(L, -1);
			lua_rawgeti(L, -2, 2);
			uv->v = lua_tonumber(L, -1);
			lua_pop(L, 3);
		}
	}
	lua_pop(L, 1);

	// get indices
	lua_pushliteral(L, "indices");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_istable(L, -1))
		{
			luaL_argerror(L, 1, "the 'indices' parameter must be an array");
		}

		cmd->indexCount = (uint32_t)lua_objlen(L, -1);
		cmd->indices = jm_command_buffer_alloc(g_currentCommandBuffer, cmd->indexCount * sizeof(uint16_t));

		uint16_t* indices = (uint16_t*)cmd->indices;
		for (uint32_t i = 0; i < cmd->indexCount; ++i)
		{
			lua_rawgeti(L, -1, i + 1);
			indices[i] = (uint16_t)lua_tointeger(L, -1);
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	// override topology
	lua_pushliteral(L, "topology");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'topology' parameter must be an integer");
		}
		cmd->topology = (uint8_t)lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	// override color
	lua_pushliteral(L, "color");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_istable(L, -1))
		{
			luaL_argerror(L, 1, "the 'color' parameter must be an array containing the red, green, blue, and alpha value of the desired color");
		}

		lua_rawgeti(L, -1, 1);
		const float r = jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 2);
		const float g = jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 3);
		const float b = jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 4);
		const float a = lua_isnil(L, -1) ? 1.0f : jm_clamp(lua_tonumber(L, -1), 0, 1);
		lua_pop(L, 1);

		cmd->color = jm_pack_color32_rgba_f32(r, g, b, a);
	}
	lua_pop(L, 1);

	// override texture
	lua_pushliteral(L, "texture");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isTexture(L, -1))
		{
			luaL_argerror(L, 1, "the 'texture' parameter must be an integer");
		}

		jm_lua_texture* texture = (jm_lua_texture*)lua_checkTexture(L, -1);
		cmd->textureHandle = texture->handle;
	}
	lua_pop(L, 1);

	// override sampler state
	lua_pushliteral(L, "sampler");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		if (!lua_isnumber(L, -1))
		{
			luaL_argerror(L, 1, "the 'sampler' parameter must be an integer");
		}

		cmd->samplerState = (jm_sampler_state)lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	// set transform
	memcpy(cmd->transform, g_cameraTransform, sizeof(cmd->transform));

	return 0;
}

static int __loadTexture(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const jm_texture_handle textureHandle = jm_load_texture(path);
	if (textureHandle == JM_TEXTURE_HANDLE_INVALID)
	{
		lua_pushnil(L);
	}
	else
	{
		jm_lua_texture* texture = lua_pushTexture(L);
		texture->handle = textureHandle;
	}

	return 1;
}

static int __loadFont(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const uint32_t size = (uint32_t)luaL_checkinteger(L, 2);
	const jm_font_handle font = jm_load_font(path, size);
	lua_pushinteger(L, font);
	return 1;
}

static int __loadEffect(lua_State* L)
{
	const char* path = lua_tostring(L, 1);
	const jm_effect effect = jm_load_effect(path);
	lua_pushinteger(L, effect);
	return 1;
}

static int __instantiateEffect(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	const jm_effect_instance instance = jm_instantiate_effect(path);
	lua_pushinteger(L, instance);
	return 1;
}

static int __setCamera(lua_State* L)
{
	const float width = (float)luaL_checkinteger(L, 1);
	const float height = (float)luaL_checkinteger(L, 2);

	const float halfWidth = width / 2.0f;
	const float halfHeight = height / 2.0f;

	g_cameraTransform[0] = 1.0f / halfWidth;
	g_cameraTransform[5] = -1.0f / halfHeight;
	g_cameraTransform[10] = 1.0f;
	g_cameraTransform[12] = -1.0f;
	g_cameraTransform[13] = 1.0f;
	g_cameraTransform[15] = 1.0f;
	return 1;
}

void jm_luaopen_graphics(
	lua_State* L)
{
	lua_getglobal(L, "jam");

	lua_newtable(L);

	lua_pushliteral(L, "loadTexture");
	lua_pushcfunction(L, __loadTexture);
	lua_settable(L, -3);

	lua_pushliteral(L, "loadFont");
	lua_pushcfunction(L, __loadFont);
	lua_settable(L, -3);

	lua_pushliteral(L, "loadEffect");
	lua_pushcfunction(L, __loadEffect);
	lua_settable(L, -3);

	lua_pushliteral(L, "instantiateEffect");
	lua_pushcfunction(L, __instantiateEffect);
	lua_settable(L, -3);

	lua_pushliteral(L, "drawText");
	lua_pushcfunction(L, __drawText);
	lua_settable(L, -3);

	lua_pushliteral(L, "draw");
	lua_pushcfunction(L, __draw);
	lua_settable(L, -3);

	lua_pushliteral(L, "setCamera");
	lua_pushcfunction(L, __setCamera);
	lua_settable(L, -3);

	lua_pushliteral(L, "topology");
	lua_newtable(L);

	lua_pushliteral(L, "TriangleList");
	lua_pushinteger(L, JM_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lua_settable(L, -3);
	lua_pushliteral(L, "TriangleStrip");
	lua_pushinteger(L, JM_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	lua_settable(L, -3);
	lua_pushliteral(L, "LineList");
	lua_pushinteger(L, JM_PRIMITIVE_TOPOLOGY_LINELIST);
	lua_settable(L, -3);
	lua_pushliteral(L, "LineStrip");
	lua_pushinteger(L, JM_PRIMITIVE_TOPOLOGY_LINESTRIP);
	lua_settable(L, -3);
	
	lua_settable(L, -3);

	lua_pushliteral(L, "sampler");
	lua_newtable(L);

	lua_pushliteral(L, "Point");
	lua_pushinteger(L, JM_SAMPLER_STATE_POINT);
	lua_settable(L, -3);
	lua_pushliteral(L, "Linear");
	lua_pushinteger(L, JM_SAMPLER_STATE_LINEAR);
	lua_settable(L, -3);

	lua_settable(L, -3);

	luaL_newmetatable(L, "Texture");          /* create metatable for Foo,
										and add it to the Lua registry */
	
	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, lua_Texture___gc);
	lua_rawset(L, -3);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);               /* dup methods table*/
	lua_rawset(L, -3);                  /* metatable.__index = methods */
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);               /* dup methods table*/
	lua_rawset(L, -3);                  /* hide metatable:
										metatable.__metatable = methods */
	lua_pop(L, 1);

	lua_pushliteral(L, "graphics");
	lua_pushvalue(L, -2);
	lua_settable(L, -4);

	lua_pop(L, 1);
}

void jm_set_current_command_buffer(
	jm_command_buffer* cb)
{
	g_currentCommandBuffer = cb;
}