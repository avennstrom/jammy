freetype_include = "C:\\Users\\Andreas\\Documents\\GitHub\\freetype2\\include"
freetype_libdir = "C:\\Users\\Andreas\\Documents\\cmake\\freetype2\\Release"
freetype_libdir_debug = freetype_libdir
freetype_lib = "freetype"
freetype_lib_debug = freetype_lib

solution "jammy"
	platforms { "Win64", "Linux64" }
	configurations { "Debug", "Development", "Standalone" }

project "bin2h"
	location "src"
	language "C"
	kind "ConsoleApp"
	targetname "bin2h"
	architecture "x86_64"
	targetdir "%{sln.location}/utils"
	objdir "%{sln.location}/intermediate/%{prj.name}/%{cfg.shortname}"

	files {
		"%{prj.location}/bin2h/bin2h.c",
	}

	flags {
		"FatalWarnings",
	}

	defines {
		"_CRT_SECURE_NO_WARNINGS",
	}

project "lua"
	location "src"
	language "C"
	kind "StaticLib"
	targetname "lua"
	architecture "x86_64"
	targetdir "%{sln.location}/lib/%{prj.name}/%{cfg.shortname}"
	objdir "%{sln.location}/intermediate/%{prj.name}/%{cfg.shortname}"

	files {
		"%{prj.location}/lua/*.h",
		"%{prj.location}/lua/*.c",
	}

	excludes {
		"%{prj.location}/lua/luac.c",
		"%{prj.location}/lua/lua.c",
	}

	defines {
		"LUA_FLOAT_TYPE=LUA_FLOAT_FLOAT",
		"WIN32_LEAN_AND_MEAN",
		"_CRT_SECURE_NO_WARNINGS",
	}

	filter { "platforms:Linux64" }
		defines { "LUA_USE_MKSTEMP" }

	filter { "configurations:Debug" }
		defines { "_DEBUG" }
		symbols "On"
		optimize "Off"

	filter { "configurations:Development or Standalone" }
		defines { "NDEBUG" }
		symbols "Off"
		optimize "Size"

	filter { "configurations:Standalone" }
		targetdir "%{sln.location}/bin/.jammy/lib"
		postbuildcommands {
			'xcopy "%{prj.location}lua\\*.h" "%{sln.location}bin\\.jammy\\include" /iy',
		}

project "chipmunk"
	location "src"
	language "C"
	kind "StaticLib"
	targetname "chipmunk"
	architecture "x86_64"
	targetdir "%{sln.location}/lib/%{prj.name}/%{cfg.shortname}"
	objdir "%{sln.location}/intermediate/%{prj.name}/%{cfg.shortname}"

	files {
		"%{prj.location}/chipmunk/*.h",
		"%{prj.location}/chipmunk/*.c",
	}

	includedirs {
		"%{prj.location}",
	}

	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"CP_USE_DOUBLES=0",
	}

	filter { "configurations:Debug" }
		defines { "_DEBUG" }
		symbols "On"
		optimize "Off"

	filter { "configurations:Development or Standalone" }
		defines { "NDEBUG" }
		symbols "Off"
		optimize "Size"

	filter { "configurations:Standalone" }
		targetdir "%{sln.location}/bin/.jammy/lib"

project "jammy"
	location "src"
	language "C"
	targetname "jammy"
	architecture "x86_64"
	targetdir "%{sln.location}/bin"
	debugdir "%{cfg.targetdir}"
	objdir "%{sln.location}/intermediate/%{prj.name}/%{cfg.shortname}"
	characterset "MBCS"

	files {
		"%{prj.location}/jammy/**.h",
		"%{prj.location}/jammy/**.c",
	}

	includedirs {
		"%{prj.location}",
		"%{prj.location}/lua",
		"/usr/local/include/freetype2",
		freetype_include,
	}

	defines {
		"LUA_FLOAT_TYPE=LUA_FLOAT_FLOAT",
		"_CRT_SECURE_NO_WARNINGS",
		"WIN32_LEAN_AND_MEAN",
		"RMT_ENABLED=0",
		"CP_USE_DOUBLES=0",
	}

	flags {
		"FatalWarnings",
	}

	filter { "configurations:Debug" }
		libdirs { freetype_libdir_debug }
		links { freetype_lib_debug }

	filter { "configurations:not Debug" }
		libdirs { freetype_libdir }
		links { freetype_lib }

	filter { "platforms:Linux64" }
		defines { "LUA_USE_MKSTEMP" }

	filter { "platforms:Win64" }
		files {
			"%{prj.location}/jammy/**.hlsl",
		}

	filter { "platforms:Win64", "configurations:Debug" }
		linkoptions "/NODEFAULTLIB:MSVCRT"

	filter { "platforms:Linux64" }
		files {
			"%{prj.location}/jammy/**.vs",
			"%{prj.location}/jammy/**.fs",
		}

	filter { "files:**.hlsl" }
		local fxc = 'fxc.exe'
		local bin2h = '"%{sln.location}/utils/bin2h.exe"'
		local vsPath = '%{cfg.objdir}/shader_%{file.basename}_vs.bin'
		local psPath = '%{cfg.objdir}/shader_%{file.basename}_ps.bin'
		local embedVsPath = '%{prj.location}/jammy/shaders/dx11/%{file.basename}.vs.h'
		local embedPsPath = '%{prj.location}/jammy/shaders/dx11/%{file.basename}.ps.h'
		buildmessage 'Compiling %{file.relpath}'
		buildcommands { 
			fxc .. ' /nologo /O3 /WX /E VertexMain /T vs_5_0 /Fo ' .. vsPath .. ' "%{file.path}"',
			fxc .. ' /nologo /O3 /WX /E PixelMain  /T ps_5_0 /Fo ' .. psPath .. ' "%{file.path}"',
			bin2h .. ' "' .. vsPath .. '" "' .. embedVsPath .. '" jm_embedded_vs_%{file.basename}',
			bin2h .. ' "' .. psPath .. '" "' .. embedPsPath .. '" jm_embedded_ps_%{file.basename}',
		}
		buildoutputs { 
			vsPath,
			psPath,
			embedVsPath,
			embedPsPath,
		}

	filter { "files:**.vs" }
		local bin2h = '%{sln.location}/utils/bin2h'
		local vsPath = '%{file.path}'
		local embedShaderPath = '%{prj.location}/jammy/shaders/opengl/%{file.basename}.vs.h'
		buildmessage 'Compiling %{file.relpath}'
		buildcommands { 
			bin2h .. ' ' .. vsPath .. ' ' .. embedShaderPath .. ' jm_embedded_vs_%{file.basename} --nullterminate',
		}
		buildoutputs { 
			embedShaderPath,
		}

	filter { "files:**.fs" }
		local bin2h = '%{sln.location}/utils/bin2h'
		local fsPath = '%{file.path}'
		local embedShaderPath = '%{prj.location}/jammy/shaders/opengl/%{file.basename}.fs.h'
		buildmessage 'Compiling %{file.relpath}'
		buildcommands { 
			bin2h .. ' ' .. fsPath .. ' ' .. embedShaderPath .. ' jm_embedded_fs_%{file.basename} --nullterminate',
		}
		buildoutputs { 
			embedShaderPath,
		}

	filter { "platforms:Win64" }
		defines { "JM_WINDOWS" }
	filter { "platforms:Linux64" }
		defines { "JM_LINUX" }

	filter { "configurations:Debug" }
		targetsuffix "_debug"
		defines { "_DEBUG", "JM_DEBUG" }
		symbols "On"
		optimize "Off"

	filter { "configurations:not Debug" }
		defines { "NDEBUG" }

	filter { "configurations:Development" }
		defines { "JM_DEVELOPMENT" }
		symbols "Off"
		optimize "Size"

	filter { "configurations:Standalone" }
		targetdir "%{sln.location}/bin/.jammy/lib"
		defines { "JM_STANDALONE" }
		symbols "Off"
		optimize "Size"
		kind "StaticLib"
		postbuildcommands {
			'xcopy "%{sln.location}/utils/lua2c" "%{sln.location}/bin/.jammy/lua2c" /eiy',
		}

	filter { "configurations:not Standalone" }
		kind "ConsoleApp"

	filter { "platforms:Win64" }
		links {
			"lua",
			"chipmunk",
			"winmm",
			"imm32",
			"version",
			"d3d11",
			"dxgi",
			"windowscodecs",
			"dxguid",
			"dsound",
		}

	filter { "platforms:Linux64" }
		buildoptions { 
			"-Wfatal-errors",
			"-Wno-incompatible-pointer-types",
		}
		links {
			"lua",
			"chipmunk",
			"freetype",
			"X11",
			"GL",
			"GLEW",
			"m",
			"pthread",
		}
