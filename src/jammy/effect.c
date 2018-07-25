#include "effect.h"
#include <jammy/remotery/Remotery.h>

#include <intrin.h>

#define MAX_EFFECTS 512
#define MAX_EFFECT_INSTANCES 1024

typedef struct jm_effects
{
	struct
	{
		size_t count;
		uint64_t* keys;
	} resources;
	struct
	{
		size_t count;
		jm_effect_instance_data* data;
	} instances;

	float gravity;
} jm_effects;

jm_effects g_effects;

int jm_effects_init()
{
	g_effects.resources.count = 0;
	g_effects.resources.keys = malloc(sizeof(uint64_t) * MAX_EFFECTS);
	g_effects.instances.count = 0;
	g_effects.instances.data = malloc(sizeof(jm_effect_instance_data) * MAX_EFFECT_INSTANCES);
	return 0;
}

void jm_effects_destroy(
	jm_effects* effects)
{
	free(g_effects.resources.keys);
	free(g_effects.instances.data);
}

jm_effect jm_load_effect(
	const char* path)
{
	return JM_EFFECT_INVALID;
}

jm_effect_instance jm_instantiate_effect(
	const char* path)
{
	jm_effect_instance_data* data = g_effects.instances.data + g_effects.instances.count;
	const size_t capacity = 4;//todo
	const size_t elementSize = ((capacity + 15) / 16) * 16;
	const size_t totalSize = elementSize * 4;
	char* buffer = malloc(totalSize);
	data->x = (float*)(data + (elementSize * 0));
	data->y = (float*)(data + (elementSize * 1));
	data->vx = (float*)(data + (elementSize * 2));
	data->vy = (float*)(data + (elementSize * 3));

	data->count = 0;

	return (jm_effect_instance)g_effects.instances.count++;
}

void jm_effects_update()
{
	rmt_BeginCPUSample(jm_effects_update, 0);

	for (size_t effectIt = 0; effectIt < g_effects.instances.count; ++effectIt)
	{
		jm_effect_instance_data* data = g_effects.instances.data + effectIt;

		// apply gravity
		const __m128 gravity = _mm_set1_ps(g_effects.gravity);
		for (size_t i = 0; i < data->count; i += 4)
		{
			__m128 y = _mm_load_ps(data->y + i);
			y = _mm_add_ps(y, gravity);
			_mm_store_ps(data->y + i, y);
		}

		// move
		for (size_t i = 0; i < data->count; i += 4)
		{
			const __m128 vx = _mm_load_ps(data->vx + i);
			const __m128 vy = _mm_load_ps(data->vy + i);

			__m128 x = _mm_load_ps(data->x + i);
			__m128 y = _mm_load_ps(data->y + i);

			x = _mm_add_ps(x, vx);
			y = _mm_add_ps(y, vy);

			_mm_store_ps(data->x + i, x);
			_mm_store_ps(data->y + i, y);
		}
	}

	rmt_EndCPUSample();
}