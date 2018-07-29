#include "physics.h"

#include <jammy/assert.h>
#include <jammy/command_buffer.h>
#include <jammy/color.h>
#include <jammy/remotery/Remotery.h>

#include <chipmunk/chipmunk.h>

#include <string.h>

typedef struct jm_physics
{
	struct cpSpace* space;

	struct cpShape** shapes;
	size_t shapeCount;

	struct cpBody** bodies;
	size_t bodyCount;

	struct cpJoint** joints;
	size_t jointCount;
} jm_physics;

jm_physics g_physics;

int jm_physics_init()
{
	g_physics.space = cpSpaceNew();
	g_physics.bodies = calloc(1024, sizeof(void*));
	g_physics.shapes = calloc(1024, sizeof(void*));

	g_physics.bodyCount = 0;
	g_physics.shapeCount = 0;

	cpVect gravity;
	gravity.x = 0.0f;
	gravity.y = 600;
	cpSpaceSetGravity(g_physics.space, gravity);

	cpSpaceSetIterations(g_physics.space, 10);

	return 0;
}

void jm_physics_tick(
	float dt)
{
	cpSpaceStep(g_physics.space, dt);
}

static void debug_draw_polygon(
	int count,
	const cpVect* verts,
	cpFloat radius,
	cpSpaceDebugColor outlineColor,
	cpSpaceDebugColor fillColor,
	cpDataPointer data)
{
	// draw outline
	{
		jm_render_command_draw* cmd = JM_COMMAND_BUFFER_PUSH(g_currentCommandBuffer, jm_render_command_draw);
		jm_render_command_draw_init(cmd);
		cmd->vertexCount = count + 1;
		cmd->vertices = jm_command_buffer_alloc(g_currentCommandBuffer, sizeof(jm_vertex) * cmd->vertexCount);
		cmd->color = jm_pack_color32_rgba_f32(outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
		cmd->fillMode = JM_FILL_MODE_WIREFRAME;
		cmd->topology = JM_PRIMITIVE_TOPOLOGY_LINESTRIP;
		cmd->textureHandle = JM_TEXTURE_HANDLE_INVALID;
		memcpy(cmd->vertices, verts, sizeof(jm_vertex) * count);
		cmd->vertices[count] = cmd->vertices[0]; // wrap around
	}
}

static cpSpaceDebugColor debug_color_for_shape(
	cpShape* shape, 
	cpDataPointer data)
{
	cpSpaceDebugColor color;
	color.r = 1.0f;
	color.g = 0.0f;
	color.b = 0.0f;
	color.a = 1.0f;
	return color;
}

void jm_physics_draw()
{
	rmt_BeginCPUSample(jm_physics_draw, 0);

	cpSpaceDebugDrawOptions options;
	memset(&options, 0, sizeof(options));
	options.flags = CP_SPACE_DEBUG_DRAW_SHAPES;
	options.drawPolygon = debug_draw_polygon;
	options.colorForShape = debug_color_for_shape;
	options.shapeOutlineColor.r = 1.0f;
	options.shapeOutlineColor.g = 0.0f;
	options.shapeOutlineColor.b = 0.0f;
	options.shapeOutlineColor.a = 1.0f;

	cpSpaceDebugDraw(g_physics.space, &options);

	rmt_EndCPUSample();
}

jm_body_handle jm_create_body(
	jm_shape_handle shapeHandle,
	const jm_physics_material* material,
	jm_body_type bodyType,
	float x,
	float y)
{
	jm_body_handle bodyHandle = (jm_body_handle)g_physics.bodyCount++;

	cpBody* body = cpBodyNew(1.0f, INFINITY);
	cpShape* shape = g_physics.shapes[shapeHandle];

	cpShapeSetBody(shape, body);
	//cpShapeSetDensity(shape, material->density);
	cpShapeSetFriction(shape, material->friction);
	//cpShapeSetElasticity(shape, material->restitution);

	cpVect pos;
	pos.x = x;
	pos.y = y;
	cpBodySetPosition(body, pos);

	cpBodySetType(body, (cpBodyType)bodyType);

	g_physics.bodies[bodyHandle] = body;

	cpSpaceAddBody(g_physics.space, body);
	cpSpaceAddShape(g_physics.space, shape);

	return bodyHandle;
}

jm_shape_handle jm_create_box_shape(
	float width, 
	float height)
{
	jm_shape_handle shapeHandle = (jm_shape_handle)g_physics.shapeCount++;

	//const float radius = sqrtf(width*width + height*height) / 2.0f;
	//const float radius = min(width, height);
	const float radius = 0.0f;
	g_physics.shapes[shapeHandle] = cpBoxShapeNew(NULL, width, height, radius);

	return shapeHandle;
}

void jm_get_velocity(
	jm_body_handle bodyHandle,
	float* x,
	float* y)
{
	jm_assert(x);
	jm_assert(y);

	const cpBody* body = g_physics.bodies[bodyHandle];
	const cpVect velocity = cpBodyGetVelocity(body);
	*x = velocity.x;
	*y = velocity.y;
}

void jm_set_body_velocity(
	jm_body_handle bodyHandle,
	float x,
	float y)
{
	cpBody* body = g_physics.bodies[bodyHandle];
	cpVect velocity;
	velocity.x = x;
	velocity.y = y;
	cpBodySetVelocity(body, velocity);
}

void jm_get_body_pos(
	jm_body_handle bodyHandle,
	float* x,
	float* y)
{
	jm_assert(x);
	jm_assert(y);

	const cpBody* body = g_physics.bodies[bodyHandle];
	const cpVect pos = cpBodyGetPosition(body);
	*x = pos.x;
	*y = pos.y;
}

void jm_set_body_pos(
	jm_body_handle bodyHandle,
	float x,
	float y)
{
	cpBody* body = g_physics.bodies[bodyHandle];
	cpVect pos;
	pos.x = x;
	pos.y = y;
	cpBodySetPosition(body, pos);
}

cpBody* jm_physics_get_body(
	jm_body_handle bodyHandle)
{
	jm_assert(bodyHandle < g_physics.bodyCount);

	return g_physics.bodies[bodyHandle];
}

cpShape* jm_physics_get_shape(
	jm_body_handle shapeHandle)
{
	jm_assert(shapeHandle < g_physics.shapeCount);

	return g_physics.shapes[shapeHandle];
}

cpSpace* jm_physics_get_space()
{
	return g_physics.space;
}