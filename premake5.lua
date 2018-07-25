solution "jammy"
	platforms { "Win64" }
	configurations { "Debug", "Development", "Standalone" }

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

	embedShaderPath = '%{prj.location}/jammy/embedded_shaders.h'

	files {
		"%{prj.location}/jammy/**.h",
		"%{prj.location}/jammy/**.c",
		"%{prj.location}/jammy/**.hlsl",
	}

	excludes {
		embedShaderPath,
	}

	includedirs {
		"%{prj.location}",
		"%{prj.location}/lua",
		"%{prj.location}/freetype",
	}

	libdirs {
		"%{sln.location}/lib/freetype/%{cfg.shortname}",
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

	fxc = '"C:/Program Files (x86)/Windows Kits/10/bin/x64/fxc.exe"'
	bin2h = '"%{sln.location}/utils/bin2h.exe"'

	prebuildcommands {
		'{DELETE} ' .. embedShaderPath,
	}

	filter { "files:**.hlsl" }
		local vsPath = '%{cfg.objdir}/shader_%{file.basename}_vs.bin'
		local psPath = '%{cfg.objdir}/shader_%{file.basename}_ps.bin'
		buildmessage 'Compiling %{file.relpath}'
		buildcommands { 
			fxc .. ' /nologo /O3 /E VertexMain /T vs_5_0 /Fo ' .. vsPath .. ' "%{file.path}"',
			fxc .. ' /nologo /O3 /E PixelMain  /T ps_5_0 /Fo ' .. psPath .. ' "%{file.path}"',
			bin2h .. ' jm_embedded_vs_%{file.basename} < ' .. vsPath .. ' >> ' .. embedShaderPath,
			bin2h .. ' jm_embedded_ps_%{file.basename} < ' .. psPath .. ' >> ' .. embedShaderPath,
		}
		buildoutputs { 
			vsPath,
			psPath,
			embedShaderPath,
		}

	filter { "platforms:Win64" }
		defines { "JM_WINDOWS" }

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
			"freetype",
		}
