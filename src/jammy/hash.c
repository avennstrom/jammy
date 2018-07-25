#include "hash.h"

#include <jammy/assert.h>

uint64_t jm_fnv(
	const char* str)
{
	jm_assert(str);
	uint64_t hash = 14695981039346656037;
	while (*str != '\0')
	{
		hash *= 1099511628211;
		hash ^= *str++;
	}
	return hash;
}