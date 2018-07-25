#pragma once

#if !defined(JM_STANDALONE)
#define JM_ASSERT_ENABLED 1
#else
#define JM_ASSERT_ENABLED 0
#endif

#if JM_ASSERT_ENABLED

void jm_assert_impl(
	const char* expression,
	const char* file,
	unsigned line);

#define jm_assert(expression) do { if (!(expression)) { jm_assert_impl(#expression, __FILE__, __LINE__); } } while (0)
#else
#define jm_assert(expression)
#endif