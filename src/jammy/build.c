#if !defined(JM_STANDALONE) && defined(JM_WINDOWS)

#include "build.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

static void bin2hexstr(
	const void* bin,
	size_t len,
	char* hex)
{
	const char table[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
	for (size_t i = 0; i < len; ++i)
	{
		const uint8_t b = ((const uint8_t*)bin)[i];
		const uint8_t lo = b & 0xf;
		const uint8_t hi = b >> 4;
		hex[i * 2 + 0] = table[hi];
		hex[i * 2 + 1] = table[lo];
	}
	hex[len * 2] = '\0';
}

static bool getVisualStudioInstallDir(
	char* installDir,
	DWORD installDirLen)
{
	HKEY hkey;
	if (ERROR_SUCCESS != RegOpenKeyEx(
		HKEY_CURRENT_USER,
		"Software\\Microsoft\\VisualStudio",
		0,
		KEY_ENUMERATE_SUB_KEYS,
		&hkey))
	{
		fprintf(stderr, "RegOpenKeyEx failed\n");
		return false;
	}

	float highestVer = 0.0f;
	char highestVerString[32];
	highestVerString[0] = '\0';

	for (DWORD i = 0; true; ++i)
	{
		char subkey[256];
		DWORD bufsize = sizeof(subkey);
		LSTATUS status = RegEnumKeyEx(
			hkey,
			i,
			subkey,
			&bufsize,
			NULL,
			NULL,
			NULL,
			NULL);

		const char* config = strstr(subkey, "_Config");
		if (config)
		{
			float ver;
			sscanf(subkey, "%f_Config", &ver);
			if (ver > highestVer)
			{
				highestVer = ver;
				strcpy(highestVerString, subkey);
			}
		}

		if (ERROR_NO_MORE_ITEMS == status)
		{
			break;
		}
	}

	if (highestVer == 0.0f)
	{
		fprintf(stderr, "No installed versions of Visual Studio could be found\n");
		return false;
	}

	char configValueName[256];
	sprintf(configValueName, "Software\\Microsoft\\VisualStudio\\%s", highestVerString);

	if (ERROR_SUCCESS != RegGetValue(
		HKEY_CURRENT_USER,
		configValueName,
		"ShellFolder",
		RRF_RT_REG_SZ,
		NULL,
		installDir,
		&installDirLen))
	{
		fprintf(stderr, "RegGetValue failed\n");
		return false;
	}

	RegCloseKey(hkey);
	return true;
}

static bool getWindowsKitsRootDirAndVersion(
	char* rootDir,
	DWORD rootDirLen,
	char* version,
	DWORD versionLen)
{
	if (ERROR_SUCCESS != RegGetValue(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
		"KitsRoot10",
		RRF_RT_REG_SZ,
		NULL,
		rootDir,
		&rootDirLen))
	{
		return false;
	}

	HKEY hkey;
	if (ERROR_SUCCESS != RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
		0,
		KEY_ENUMERATE_SUB_KEYS,
		&hkey))
	{
		return false;
	}

	if (ERROR_SUCCESS != RegEnumKeyEx(
		hkey,
		0,
		version,
		&versionLen,
		NULL,
		NULL,
		NULL,
		NULL))
	{
		return false;
	}

	RegCloseKey(hkey);
	return true;
}

static bool getGameName(
	char* gameName)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	lua_newtable(L);
	lua_setglobal(L, "jam");

	if (luaL_dofile(L, "jammy.lua"))
	{
		const char* error = lua_tostring(L, -1);
		fprintf(stderr, "%s\n", error);
		return false;
	}

	lua_getglobal(L, "jam");
	lua_pushliteral(L, "name");
	lua_gettable(L, -2);

	if (lua_isnil(L, -1))
	{
		return false;
	}

	strcpy(gameName, lua_tostring(L, -1));
	lua_close(L);

	return true;
}

char* lua2c()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_dofile(L, ".jammy/lua2c/lua2c.lua"))
	{
		const char* error = lua_tostring(L, -1);
		fprintf(stderr, "%s\n", error);
		ExitProcess(1);
	}

	const char* _source = lua_tostring(L, -1);
	char* source = malloc(strlen(_source) + 1);
	strcpy(source, _source);
	lua_close(L);

	return source;
}

int jm_build(int argc, char** argv)
{
	char overrideVisualStudioDir[MAX_PATH + 1];
	overrideVisualStudioDir[0] = '\0';

	// parse arguments
	for (int i = 2; i < argc; ++i)
	{
		if (strstr(argv[i], "--vsroot=") == argv[i])
		{
			strcpy(overrideVisualStudioDir, argv[i] + strlen("--vsroot="));
		}
	}

	// convert Lua to C
	char* source = lua2c();

	// remove the main function
	char* mainBegin = strstr(source, "int main");
	*mainBegin = '\0';

	// remove static from lcf_main
	char* lcfmainBegin = strstr(source, "static int lcf_main");
	memcpy(lcfmainBegin, "       ", 7);
	
	srand((uint32_t)time(NULL));
	const uint16_t randomness[2] = { 
		(uint16_t)(rand() & 0xffff), 
		(uint16_t)(rand() & 0xffff),
	};
	char randomHex[32];
	bin2hexstr(randomness, sizeof(randomness), randomHex);

	char tempDir[MAX_PATH + 1];
	GetTempPath(_countof(tempDir), tempDir);
	strcat(tempDir, "jammy");

	CreateDirectory(tempDir, NULL);

	char sourceDir[MAX_PATH + 1];
	sprintf(sourceDir, "%s\\%s.c", tempDir, randomHex);

	// write generated C code to disk
	{
		FILE* f = fopen(sourceDir, "w");
		fwrite(source, 1, strlen(source), f);
		fclose(f);
	}

	free(source);

	char visualStudioInstallDir[MAX_PATH + 1];
	if (strlen(overrideVisualStudioDir) > 0)
	{
		strcpy(visualStudioInstallDir, overrideVisualStudioDir);
	}
	else
	{
		if (!getVisualStudioInstallDir(
			visualStudioInstallDir,
			sizeof(visualStudioInstallDir)))
		{
			fprintf(stderr, "Could not locate Visual Studio installation directory.\n");
			return 1;
		}
	}

	printf("Visual Studio root: %s\n", visualStudioInstallDir);

	char windowsKitsRoot[MAX_PATH + 1];
	char windowsKitsVersion[64];
	if (!getWindowsKitsRootDirAndVersion(
		windowsKitsRoot,
		sizeof(windowsKitsRoot),
		windowsKitsVersion,
		sizeof(windowsKitsVersion)))
	{
		fprintf(stderr, "Could not locate Windows Kits installation directory.\n");
		return 1;
	}

	printf("Windows Kits root: %s\n", windowsKitsRoot);
	printf("Windows Kits version: %s\n", windowsKitsVersion);

	char compilerPath[MAX_PATH + 1];
	sprintf(compilerPath, "%s\\VC\\bin\\amd64\\cl.exe", visualStudioInstallDir);

	// resolve Visual Studio paths
	char vcIncludeDir[MAX_PATH + 1];
	char vcLibDir[MAX_PATH + 1];
	sprintf(vcIncludeDir, "%s\\VC\\include", visualStudioInstallDir);
	sprintf(vcLibDir, "%s\\VC\\lib\\amd64", visualStudioInstallDir);

	// resolve Windows Kits paths
	char ucrtIncludeDir[MAX_PATH + 1];
	char ucrtLibDir[MAX_PATH + 1];
	char umLibDir[MAX_PATH + 1];
	sprintf(ucrtIncludeDir, "%s\\Include\\%s\\ucrt", windowsKitsRoot, windowsKitsVersion);
	sprintf(ucrtLibDir, "%s\\Lib\\%s\\ucrt\\x64", windowsKitsRoot, windowsKitsVersion);
	sprintf(umLibDir, "%s\\Lib\\%s\\um\\x64", windowsKitsRoot, windowsKitsVersion);
	
	char currentDir[MAX_PATH + 1];
	GetCurrentDirectory(sizeof(currentDir), currentDir);

	// resolve Engine paths
	char engineIncludeDir[MAX_PATH + 1];
	char engineLibDir[MAX_PATH + 1];
	sprintf(engineIncludeDir, "%s\\.jammy\\include", currentDir);
	sprintf(engineLibDir, "%s\\.jammy\\lib", currentDir);

	// build output dir
	char buildDir[MAX_PATH + 1];
	sprintf(buildDir, "%s\\build", currentDir);

	CreateDirectory(buildDir, NULL);

	// retrieve game name
	char gameName[128];
	if (!getGameName(gameName))
	{
		strcpy(gameName, "untitled");
	}

	char buildExePath[MAX_PATH + 1];
	sprintf(buildExePath, "%s\\%s.exe", buildDir, gameName);

	char compilerArgs[2048];
	compilerArgs[0] = '\0';
	sprintf(compilerArgs, "%s /I\"%s\"", compilerArgs, vcIncludeDir);
	sprintf(compilerArgs, "%s /I\"%s\"", compilerArgs, ucrtIncludeDir);
	sprintf(compilerArgs, "%s /I\"%s\"", compilerArgs, engineIncludeDir);
	sprintf(compilerArgs, "%s %s.c", compilerArgs, randomHex); // files
	sprintf(compilerArgs, "%s /Fe\"%s\"", compilerArgs, buildExePath);
	strcat(compilerArgs, " /MD");
	strcat(compilerArgs, " /O2 /Ot");
	// linker arguments
	strcat(compilerArgs, " /link");
	sprintf(compilerArgs, "%s /LIBPATH:\"%s\"", compilerArgs, vcLibDir);
	sprintf(compilerArgs, "%s /LIBPATH:\"%s\"", compilerArgs, ucrtLibDir);
	sprintf(compilerArgs, "%s /LIBPATH:\"%s\"", compilerArgs, umLibDir);
	sprintf(compilerArgs, "%s /LIBPATH:\"%s\"", compilerArgs, engineLibDir);
	strcat(compilerArgs, " \"lua.lib\""); // link lua
	strcat(compilerArgs, " \"jammy.lib\""); // link engine
	strcat(compilerArgs, " \"user32.lib\"");
	strcat(compilerArgs, " \"d3d11.lib\"");
	strcat(compilerArgs, " \"dxgi.lib\"");
	strcat(compilerArgs, " \"dxguid.lib\"");
	strcat(compilerArgs, " \"dsound.lib\"");
	strcat(compilerArgs, " /MACHINE:X64");
	strcat(compilerArgs, " /SUBSYSTEM:WINDOWS");
	strcat(compilerArgs, " /DEFAULTLIB:jammy.lib");

	// pipe compiler stdout
	HANDLE childStdOut_Rd = NULL;
	HANDLE childStdOut_Wr = NULL;

	SECURITY_ATTRIBUTES pipeSecurityAttr;
	pipeSecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	pipeSecurityAttr.bInheritHandle = TRUE;
	pipeSecurityAttr.lpSecurityDescriptor = NULL;
	CreatePipe(&childStdOut_Rd, &childStdOut_Wr, &pipeSecurityAttr, 0);
	SetHandleInformation(childStdOut_Rd, HANDLE_FLAG_INHERIT, 0);

	// run compiler
	STARTUPINFOA startupInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.hStdOutput = childStdOut_Wr;
	startupInfo.hStdError = childStdOut_Wr;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	PROCESS_INFORMATION processInfo;
	if (FALSE == CreateProcess(
		compilerPath,
		compilerArgs,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		tempDir,
		&startupInfo,
		&processInfo))
	{
		fprintf(stderr, "CreateProcess failed with error code: %d\n", GetLastError());
		return 1;
	}

	CloseHandle(childStdOut_Wr);
	CloseHandle(processInfo.hThread);

	// read compiler stdout
	while (true)
	{
		char buf[1024];
		BOOL success = FALSE;

		DWORD numBytesRead;
		success = ReadFile(childStdOut_Rd, buf, sizeof(buf), &numBytesRead, NULL);
		if (!success || numBytesRead == 0) break;

		fwrite(buf, sizeof(char), numBytesRead, stdout);
		if (!success) break;
	}

	//WaitForSingleObject(processInfo.hProcess, INFINITE);
	DWORD exitCode;
	GetExitCodeProcess(processInfo.hProcess, &exitCode);
	CloseHandle(processInfo.hProcess);

	DeleteFile(sourceDir);

	char objFilePath[MAX_PATH + 1];
	sprintf(objFilePath, "%s\\%s.obj", tempDir, randomHex);
	DeleteFile(objFilePath);

	char expFilePath[MAX_PATH + 1];
	sprintf(expFilePath, "%s\\%s.exp", buildDir, gameName);
	DeleteFile(expFilePath);

	char libFilePath[MAX_PATH + 1];
	sprintf(libFilePath, "%s\\%s.lib", buildDir, gameName);
	DeleteFile(libFilePath);

	if (exitCode != 0)
	{
		fprintf(stderr, "cl.exe failed with error code: %d\n", exitCode);
		return 1;
	}

	printf("Compilation successful\n");
	return 0;
}

#endif