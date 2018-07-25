#pragma once

#include <inttypes.h>

#define JM_SHAPE ((jm_shape_handle)-1)
#define JM_BODY_INVALID ((jm_body_handle)-1)

typedef uint32_t jm_shape_handle;
typedef uint32_t jm_body_handle;

typedef enum jm_body_type
{
	JM_BODY_TYPE_DYNAMIC,
	JM_BODY_TYPE_KINEMATIC,
	JM_BODY_TYPE_STATIC,
} jm_body_type;

typedef struct jm_physics_material
{
	float density;
	float friction;
	float restitution;
} jm_physics_material;

int jm_physics_init();

void jm_physics_tick(
	float dt);

void jm_physics_draw();

jm_body_handle jm_create_body(
	jm_shape_handle shapeHandle,
	const jm_physics_material* material,
	jm_body_type bodyType,
	float x,
	float y);

jm_shape_handle jm_create_box_shape(
	float width,
	float height);

void jm_get_velocity(
	jm_body_handle bodyHandle,
	float* x,
	float* y);

void jm_set_body_velocity(
	jm_body_handle bodyHandle,
	float x,
	float y);

void jm_get_body_pos(
	jm_body_handle bodyHandle,
	float* x,
	float* y);

void jm_set_body_pos(
	jm_body_handle bodyHandle,
	float x,
	float y);

struct cpBody* jm_physics_get_body(
	jm_body_handle bodyHandle);

struct cpShape* jm_physics_get_shape(
	jm_body_handle shapeHandle);

struct cpSpace* jm_physics_get_space();
