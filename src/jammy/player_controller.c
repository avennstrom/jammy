#include "player_controller.h"

#include <jammy/physics.h>
#include <jammy/assert.h>

#include <chipmunk/chipmunk.h>
#include <chipmunk/chipmunk_private.h>

#include <lua/lua.h>
#include <lua/lauxlib.h>

#include <stdbool.h>

#define UDATA_NAME "PlayerController"

#define PLAYER_VELOCITY 100.0f

#define PLAYER_AIR_ACCEL_TIME 0.00000001f
#define PLAYER_AIR_ACCEL (PLAYER_VELOCITY/PLAYER_AIR_ACCEL_TIME)

#define JUMP_HEIGHT 50.0f
#define FALL_VELOCITY 900.0f

typedef struct jm_player_controller
{
	cpBody* body;
	cpShape* shape;
	bool moveLeft;
	bool moveRight;
	bool isGrounded;
} jm_player_controller;

static void select_player_ground_normal(
	cpBody* body, 
	cpArbiter* arb, 
	cpVect* groundNormal)
{
	cpVect n = cpvneg(cpArbiterGetNormal(arb));

	if (n.y < groundNormal->y)
	{
		(*groundNormal) = n;
	}
}

static void player_update_velocity(
	cpBody* body, 
	cpVect gravity, 
	cpFloat damping, 
	cpFloat dt)
{
	jm_player_controller* ctrl = (jm_player_controller*)body->userData;
	jm_assert(ctrl->body == body);

	float walkDir = 0.0f;
	if (ctrl->moveLeft && !ctrl->moveRight)
	{
		walkDir = -1.0f;
	}
	else if (ctrl->moveRight && !ctrl->moveLeft)
	{
		walkDir = 1.0f;
	}

	int jumpState = 0;//(ChipmunkDemoKeyboard.y > 0.0f);

	// Grab the grounding normal from last frame
	cpVect groundNormal = cpvzero;
	cpBodyEachArbiter(ctrl->body, (cpBodyArbiterIteratorFunc)select_player_ground_normal, &groundNormal);

	float remainingBoost = 0.0f;

	ctrl->isGrounded = (groundNormal.y < 0.0f);
	if (groundNormal.y > 0.0f) remainingBoost = 0.0f;

	// apply gravity
	bool boost = false;//(jumpState && remainingBoost > 0.0f);
	cpVect g = (boost ? cpvzero : gravity);
	cpBodyUpdateVelocity(body, g, damping, dt);

	// Target horizontal speed for air/ground control
	const float target_vx = PLAYER_VELOCITY * walkDir;

	// Update the surface velocity and friction
	// Note that the "feet" move in the opposite direction of the player.
	ctrl->shape->surfaceV = cpv(-target_vx, 0.0f);
	ctrl->shape->u = 100000.0f;

	// Apply air control if not grounded
	if (!ctrl->isGrounded)
	{
		ctrl->body->v.x = target_vx;
	}

	body->v.y = cpfclamp(body->v.y, -INFINITY, FALL_VELOCITY);
}

static int __createPlayerController(lua_State* L)
{
	//const jm_body_handle bodyHandle = (jm_body_handle)luaL_checkinteger(L, 1);
	//jm_assert(bodyHandle != JM_BODY_INVALID);

	jm_player_controller* ctrl = lua_newuserdata(L, sizeof(jm_player_controller));
	luaL_getmetatable(L, UDATA_NAME);
	lua_setmetatable(L, -2);

	//ctrl->body = jm_physics_get_body(bodyHandle);
	//ctrl->shape = ctrl->body->shapeList;

	ctrl->body = cpSpaceAddBody(jm_physics_get_space(), cpBodyNew(1.0f, INFINITY));
	ctrl->body->p = cpv(100, 100);
	ctrl->body->velocity_func = player_update_velocity;
	ctrl->body->userData = ctrl;

	ctrl->shape = cpSpaceAddShape(jm_physics_get_space(), cpBoxShapeNew(ctrl->body, 16, 24, 0.0f));
	ctrl->shape->e = 0.0f; 
	ctrl->shape->u = 0.0f;
	ctrl->shape->type = 1;

	ctrl->moveLeft = false;
	ctrl->moveRight = false;
	ctrl->isGrounded = false;

	return 1;
}

static int __PlayerController_keyDown(lua_State* L)
{
	jm_player_controller* ctrl = luaL_checkudata(L, 1, UDATA_NAME);
	const lua_Integer key = luaL_checkinteger(L, 2);
	if (key == 0x25)
	{
		ctrl->moveLeft = true;
	}
	else if (key == 0x27)
	{
		ctrl->moveRight = true;
	}
	else if (key == 0x20 && ctrl->isGrounded)
	{
		const cpVect gravity = cpSpaceGetGravity(jm_physics_get_space());
		const float jump_v = -cpfsqrt(2.0f * JUMP_HEIGHT * gravity.y);
		ctrl->body->v = cpvadd(ctrl->body->v, cpv(0.0, jump_v));
		ctrl->isGrounded = false; // just in case
	}

	return 0;
}

static int __PlayerController_keyUp(lua_State* L)
{
	jm_player_controller* ctrl = luaL_checkudata(L, 1, UDATA_NAME);
	const lua_Integer key = luaL_checkinteger(L, 2);
	if (key == 0x25)
	{
		ctrl->moveLeft = false;
	}
	else if (key == 0x27)
	{
		ctrl->moveRight = false;
	}

	return 0;
}

static const luaL_reg __PlayerController_methods[] = {
	{ "keyDown", __PlayerController_keyDown },
	{ "keyUp", __PlayerController_keyUp },
	{ 0, 0 }
};

void jm_player_controller_luaopen(
	struct lua_State* L)
{
	lua_getglobal(L, "jam");

	lua_pushliteral(L, "createPlayerController");
	lua_pushcfunction(L, __createPlayerController);
	lua_settable(L, -3);

	lua_pop(L, 1);

	luaL_openlib(L, UDATA_NAME, __PlayerController_methods, 0);
	luaL_newmetatable(L, UDATA_NAME);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);

	lua_pop(L, 1);
	lua_pop(L, 1);
}