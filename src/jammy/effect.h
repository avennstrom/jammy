#pragma once

#include <inttypes.h>

#define JM_EFFECT_INVALID ((jm_effect)-1)
#define JM_EFFECT_INSTANCE_INVALID ((jm_effect_instance)-1)

typedef uint32_t jm_effect;
typedef uint32_t jm_effect_instance;

typedef struct jm_effect_instance_data
{
	size_t count;
	float* x;
	float* y;
	float* vx;
	float* vy;
} jm_effect_instance_data;

int jm_effects_init();

void jm_effects_destroy();

jm_effect jm_load_effect(
	const char* path);

jm_effect_instance jm_instantiate_effect(
	const char* path);

void jm_effects_update();
