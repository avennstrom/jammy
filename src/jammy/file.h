#pragma once

#include <stdio.h>
#include <stdbool.h>

inline bool jm_file_exists(const char* path)
{
	FILE* file = fopen(path, "r");
	if (file)
	{
		fclose(file);
		return true;
	}
	return false;
}