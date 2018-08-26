#if defined(JM_LINUX)
#include "assert.h"

#include <stdio.h>
#include <stdlib.h>

#if JM_ASSERT_ENABLED
void jm_assert_impl(
	const char* expression,
	const char* file,
	unsigned line)
{
	char message[1024];
	sprintf(message, "Expression: %s\n\nFile: %s\nLine: %u", expression, file, line);
	fprintf(stderr, 
		"------------------------\n"
		"!!! ASSERTION FAILED !!!\n"
		"------------------------\n"
		"%s\n"
		"------------------------\n", message);
    exit(1);
}
#endif

#endif