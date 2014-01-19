--[[
To customize the following settings table, create a file named "premake5-custom.lua" in this directory.

Example:

ioq3.createMissionPackProjects = false
ioq3.outputPath = "C:\\Games\\Quake III Arena"
ioq3.outputBase = "mymod"

Set outputPath if you want to be able to debug from MSVC. If you do this, ioquake3.x86.exe, renderer_opengl1_x86.dll etc. in the output path will be overwritten when you build the solution, so it's probably a good idea to have a separate Q3A copy for development.
--]]
ioq3 =
{
	-- Create a project for building the dedicated server
	createDedicatedServerProject = true,
	
	-- Create the three projects for building the mission pack game dlls
	createMissionPackProjects = true,
	
	-- The engine executables, renderer dlls and SDL.dll will be output here
	outputPath = nil,
	
	-- If outputPath is set, the game dlls will be output to this subdirectory of outputPath
	outputBase = "baseq3",
	
	-- If createMissionPackProjects is true, and outputPath is set, the mission pack game dlls will be output to this subdirectory of outputPath
	outputMissionPack = "missionpack"
}

-- Include the custom configuration file if it exists
local customFilename = "premake5-custom.lua"

if os.isfile(customFilename) then
	dofile(customFilename)
end

-- SDL needs the DirectX SDK
local dxSdkDir = os.getenv("DXSDK_DIR")
	
if dxSdkDir == nil then
	print("WARNING: DXSDK_DIR environment variable not set. Install the DirectX SDK.")
end

-----------------------------------------------------------------------------

solution "ioquake3"
	language "C"
	location "msvc-premake"
	startproject "quake3"
	
	defines
	{
		"_CRT_SECURE_NO_DEPRECATE"
	}

	platforms { "native", "x32", "x64" }
	configurations { "Debug", "Release" }
	
	configuration "x64"
		defines { "_WIN64", "__WIN64__" }
			
	configuration "Debug"
		optimize "Debug"
		defines "_DEBUG"
		flags "Symbols"
				
	configuration "Release"
		optimize "Full"
		defines "NDEBUG"
	
-----------------------------------------------------------------------------
	
project "ioquake3"
	kind "WindowedApp"
	
	configuration "x64"
		targetname "ioquake3.x86_64"
	configuration "not x64"
		targetname "ioquake3.x86"
	configuration {}
	
	defines
	{
		"_WIN32",
		"WIN32",
		"BOTLIB",
		"USE_CURL",
		"USE_CURL_DLOPEN",
		"USE_OPENAL",
		"USE_OPENAL_DLOPEN",
		"USE_VOIP",
		"USE_RENDERER_DLOPEN"
	}

	files
	{
		"../code/asm/ftola.asm",
		"../code/asm/snapvector.asm",
		"../code/botlib/*.c",
		"../code/botlib/*.h",
		"../code/client/*.c",
		"../code/client/*.h",
		"../code/qcommon/*.c",
		"../code/qcommon/*.h",
		"../code/sdl/sdl_input.c",
		"../code/sdl/sdl_snd.c",
		"../code/SDL12/src/main/win32/SDL_win32_main.c",
		"../code/server/*.c",
		"../code/server/*.h",
		"../code/sys/con_log.c",
		"../code/sys/con_passive.c",
		"../code/sys/sys_main.c",
		"../code/sys/sys_win32.c",
		"../code/sys/*.h",
		"../code/sys/*.rc"
	}
	
	configuration "x64"
		files "../code/asm/vm_x86_64.asm"
	configuration {}
	
	excludes
	{
		"../code/client/libmumblelink.*",
		"../code/qcommon/vm_none.c",
		"../code/qcommon/vm_powerpc*.*",
		"../code/qcommon/vm_sparc.*",
		"../code/server/sv_rankings.c"
	}
	
	includedirs
	{
		"../code/SDL12/include",
		"../code/libcurl",
		"../code/AL",
		"../code/libspeex/include",
		"../code/zlib",
		"../code/jpeg-8c"
	}
	
	links
	{
		"user32",
		"advapi32",
		"winmm",
		"wsock32",
		"ws2_32",
		"OpenGL32",
		"psapi",
		"gdi32",
		
		-- Other projects
		"libsdl",
		"libspeex",
		"zlib"
	}
	
	linkoptions
	{
		"/SAFESEH:NO" -- for MSVC2012
	}
	
	configuration { "not x64", "**.asm" }
		buildmessage "Assembling..."
		buildcommands('ml /c /Zi /Fo"%{cfg.objdir}/%{file.basename}.asm.obj" "%{file.relpath}"')
		buildoutputs '%{cfg.objdir}/%{file.basename}.asm.obj'
		
	configuration { "x64", "**.asm" }
		buildmessage "Assembling..."
		buildcommands('ml64 /c /Zi /Fo"%{cfg.objdir}/%{file.basename}.asm.obj" "%{file.relpath}"')
		buildoutputs '%{cfg.objdir}/%{file.basename}.asm.obj'
	
	configuration "Debug"
		if ioq3.outputPath == nil then
			targetdir "../build/ioquake3_debug"
		else
			targetdir(ioq3.outputPath)
		end

		objdir "../build/ioquake3_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil then
			targetdir "../build/ioquake3_release"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/ioquake3_release"

-----------------------------------------------------------------------------

if ioq3.createDedicatedServerProject then
project "ioquake3_dedicated"
	kind "ConsoleApp"
	
	configuration "x64"
		targetname "ioq3ded.x86_64"
	configuration "not x64"
		targetname "ioq3ded.x86"
	configuration {}
	
	defines
	{
		"DEDICATED",
		"BOTLIB",
		"USE_VOIP"
	}

	files
	{
		"../code/asm/ftola.asm",
		"../code/asm/snapvector.asm",
		"../code/botlib/*.c",
		"../code/botlib/*.h",
		"../code/null/null_client.c",
		"../code/null/null_input.c",
		"../code/null/null_snddma.c",
		"../code/qcommon/*.c",
		"../code/qcommon/*.h",
		"../code/server/*.c",
		"../code/server/*.h",
		"../code/sys/con_log.c",
		"../code/sys/con_win32.c",
		"../code/sys/sys_main.c",
		"../code/sys/sys_win32.c",
		"../code/sys/*.h",
		"../code/sys/*.rc"
	}
	
	configuration "x64"
		files "../code/asm/vm_x86_64.asm"
	configuration {}
	
	excludes
	{
		"../code/qcommon/vm_none.c",
		"../code/qcommon/vm_powerpc*.*",
		"../code/qcommon/vm_sparc.*",
		"../code/server/sv_rankings.c"
	}
	
	includedirs
	{
		"../code/zlib"
	}
	
	links
	{
		"winmm",
		"wsock32",
		"ws2_32",
		"psapi",
		
		-- Other projects
		"zlib"
	}
	
	linkoptions
	{
		"/SAFESEH:NO" -- for MSVC2012
	}
	
	configuration { "not x64", "**.asm" }
		buildmessage "Assembling..."
		buildcommands('ml /c /Zi /Fo"%{cfg.objdir}/%{file.basename}.asm.obj" "%{file.relpath}"')
		buildoutputs '%{cfg.objdir}/%{file.basename}.asm.obj'
		
	configuration { "x64", "**.asm" }
		buildmessage "Assembling..."
		buildcommands('ml64 /c /Zi /Fo"%{cfg.objdir}/%{file.basename}.asm.obj" "%{file.relpath}"')
		buildoutputs '%{cfg.objdir}/%{file.basename}.asm.obj'
	
	configuration "Debug"
		if ioq3.outputPath == nil then
			targetdir "../build/ioquake3_dedicated_debug"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/ioquake3_dedicated_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil then
			targetdir "../build/ioquake3_dedicated_release"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/ioquake3_dedicated_release"
end

-----------------------------------------------------------------------------

project "renderer_opengl1"
	kind "SharedLib"

	configuration "x64"
		targetname "renderer_opengl1_x86_64"
	configuration "not x64"
		targetname "renderer_opengl1_x86"
	configuration {}
	
	defines
	{
		"_WIN32",
		"WIN32",
		"_WINDOWS",
		"USE_INTERNAL_JPEG",
		"USE_RENDERER_DLOPEN"
	}
	
	files
	{
		"../code/jpeg-8c/*.c",
		"../code/jpeg-8c/*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/qcommon/qcommon.h",
		"../code/qcommon/qfiles.h",
		"../code/qcommon/puff.c",
		"../code/qcommon/puff.h",
		"../code/qcommon/surfaceflags.h",
		"../code/renderergl1/*.c",
		"../code/renderergl1/*.h",
		"../code/renderercommon/*.c",
		"../code/renderercommon/*.h",
		"../code/sdl/sdl_gamma.c",
		"../code/sdl/sdl_glimp.c"
	}
	
	includedirs
	{
		"../code/SDL12/include",
		"../code/libcurl",
		"../code/AL",
		"../code/libspeex/include",
		"../code/zlib",
		"../code/jpeg-8c"
	}
	
	links
	{
		"user32",
		"advapi32",
		"winmm",
		"wsock32",
		"ws2_32",
		"OpenGL32",
		"psapi",
		
		-- Other projects
		"libsdl",
		"zlib"
	}
	
	configuration "Debug"
		if ioq3.outputPath == nil then
			targetdir "../build/renderer_opengl1_debug"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/renderer_opengl1_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil then
			targetdir "../build/renderer_opengl1_release"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/renderer_opengl1_release"
	
-----------------------------------------------------------------------------

project "renderer_opengl2"
	kind "SharedLib"

	configuration "x64"
		targetname "renderer_opengl2_x86_64"
	configuration "not x64"
		targetname "renderer_opengl2_x86"
	configuration {}
	
	defines
	{
		"_WIN32",
		"WIN32",
		"USE_INTERNAL_JPEG",
		"USE_RENDERER_DLOPEN"
	}
	
	defines
	{
		"_WIN32"
	}

	files
	{
		-- Name the stringified GLSL files explicitly (without * wildcard) so they're added to the project even when they don't exist yet
		"../build/dynamic/renderergl2/bokeh_fp.c",
		"../build/dynamic/renderergl2/bokeh_vp.c",
		"../build/dynamic/renderergl2/calclevels4x_fp.c",
		"../build/dynamic/renderergl2/calclevels4x_vp.c",
		"../build/dynamic/renderergl2/depthblur_fp.c",
		"../build/dynamic/renderergl2/depthblur_vp.c",
		"../build/dynamic/renderergl2/dlight_fp.c",
		"../build/dynamic/renderergl2/dlight_vp.c",
		"../build/dynamic/renderergl2/down4x_fp.c",
		"../build/dynamic/renderergl2/down4x_vp.c",
		"../build/dynamic/renderergl2/fogpass_fp.c",
		"../build/dynamic/renderergl2/fogpass_vp.c",
		"../build/dynamic/renderergl2/generic_fp.c",
		"../build/dynamic/renderergl2/generic_vp.c",
		"../build/dynamic/renderergl2/lightall_fp.c",
		"../build/dynamic/renderergl2/lightall_vp.c",
		"../build/dynamic/renderergl2/pshadow_fp.c",
		"../build/dynamic/renderergl2/pshadow_vp.c",
		"../build/dynamic/renderergl2/shadowfill_fp.c",
		"../build/dynamic/renderergl2/shadowfill_vp.c",
		"../build/dynamic/renderergl2/shadowmask_fp.c",
		"../build/dynamic/renderergl2/shadowmask_vp.c",
		"../build/dynamic/renderergl2/ssao_fp.c",
		"../build/dynamic/renderergl2/ssao_vp.c",
		"../build/dynamic/renderergl2/texturecolor_fp.c",
		"../build/dynamic/renderergl2/texturecolor_vp.c",
		"../build/dynamic/renderergl2/tonemap_fp.c",
		"../build/dynamic/renderergl2/tonemap_vp.c",
		"../code/jpeg-8c/*.c",
		"../code/jpeg-8c/*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/qcommon/qcommon.h",
		"../code/qcommon/qfiles.h",
		"../code/qcommon/puff.c",
		"../code/qcommon/puff.h",
		"../code/qcommon/surfaceflags.h",
		"../code/renderergl2/*.c",
		"../code/renderergl2/*.h",
		"../code/renderergl2/glsl/*.glsl",
		"../code/renderercommon/*.c",
		"../code/renderercommon/*.h",
		"../code/sdl/sdl_gamma.c",
		"../code/sdl/sdl_glimp.c"
	}
	
	-- The stringified GLSL files cause virtual paths to be a little too deeply nested
	vpaths
	{
		["dynamic"] = "../build/dynamic/renderergl2/*.c",
		["*"] = "../code"
	}
	
	includedirs
	{
		"../code/SDL12/include",
		"../code/libcurl",
		"../code/AL",
		"../code/libspeex/include",
		"../code/zlib",
		"../code/jpeg-8c"
	}
	
	links
	{
		"user32",
		"advapi32",
		"winmm",
		"wsock32",
		"ws2_32",
		"OpenGL32",
		"psapi",
		
		-- Other projects
		"libsdl",
		"zlib"
	}
	
	-- Creates the output directory for stringified GLSL
	prebuildcommands
	{
		"if not exist ..\\..\\build\\dynamic mkdir ..\\..\\build\\dynamic",
		"if not exist ..\\..\\build\\dynamic\\renderergl2 mkdir ..\\..\\build\\dynamic\\renderergl2",
	}
	
	configuration "**.glsl"
		buildmessage "Stringifying %{file.basename}.glsl"
		buildcommands 'cscript.exe "..\\msvc\\glsl_stringify.vbs" //Nologo "%{file.relpath}" "..\\..\\build\\dynamic\\renderergl2\\%{file.basename}.c"'
		
		-- Should be relative to the project directory - '..\\..\\build\\dynamic\\renderergl2\\%{file.basename}.c', but premake appends a '..\\' for some reason...
		buildoutputs '..\\build\\dynamic\\renderergl2\\%{file.basename}.c'
	
	configuration "Debug"
		if ioq3.outputPath == nil then
			targetdir "../build/renderer_opengl2_debug"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/renderer_opengl2_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil then
			targetdir "../build/renderer_opengl2_release"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/renderer_opengl2_release"
		
-----------------------------------------------------------------------------

project "quake3_cgame"
	kind "SharedLib"
	
	configuration "x64"
		targetname "cgamex86_64"
	configuration "not x64"
		targetname "cgamex86"
	configuration {}

	files
	{
		"../code/cgame/*.c",
		"../code/cgame/*.h",
		"../code/game/bg_*.c",
		"../code/game/bg_*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/qcommon/surfaceflags.h"
	}
	
	excludes
	{
		"../code/cgame/cg_newdraw.c",
		"../code/game/bg_lib.*"
	}
	
	links "winmm"

	configuration "Debug"
		if ioq3.outputPath == nil and ioq3.outputBase == nil then
			targetdir "../build/quake3_cgame_debug"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputBase)
		end
		
		objdir "../build/quake3_cgame_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil and ioq3.outputBase == nil then
			targetdir "../build/quake3_cgame_release"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputBase)
		end
		
		objdir "../build/quake3_cgame_release"

-----------------------------------------------------------------------------

project "quake3_game"
	kind "SharedLib"

	configuration "x64"
		targetname "qagamex86_64"
	configuration "not x64"
		targetname "qagamex86"
	configuration {}
	
	files
	{
		"../code/game/*.c",
		"../code/game/*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/qcommon/surfaceflags.h"
	}
	
	excludes
	{
		"../code/game/bg_lib.*",
		"../code/game/g_rankings.c"
	}
	
	links "winmm"

	configuration "Debug"
		if ioq3.outputPath == nil and ioq3.outputBase == nil then
			targetdir "../build/quake3_game_debug"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputBase)
		end
		
		objdir "../build/quake3_game_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil and ioq3.outputBase == nil then
			targetdir "../build/quake3_game_release"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputBase)
		end
		
		objdir "../build/quake3_game_release"

-----------------------------------------------------------------------------

project "quake3_ui"
	kind "SharedLib"
	
	configuration "x64"
		targetname "uix86_64"
	configuration "not x64"
		targetname "uix86"
	configuration {}
	
	files
	{
		"../code/game/bg_misc.c",
		"../code/q3_ui/*.c",
		"../code/q3_ui/*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/ui/ui_syscalls.c"
	}
	
	excludes
	{
		"../code/q3_ui/ui_loadconfig.c",
		"../code/q3_ui/ui_login.c",
		"../code/q3_ui/ui_rankings.c",
		"../code/q3_ui/ui_rankstatus.c",
		"../code/q3_ui/ui_saveconfig.c",
		"../code/q3_ui/ui_signup.c",
		"../code/q3_ui/ui_specifyleague.c"
	}
	
	links "winmm"

	configuration "Debug"
		if ioq3.outputPath == nil and ioq3.outputBase == nil then
			targetdir "../build/quake3_ui_debug"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputBase)
		end
		
		objdir "../build/quake3_ui_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil and ioq3.outputBase == nil then
			targetdir "../build/quake3_ui_release"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputBase)
		end
		
		objdir "../build/quake3_ui_release"
	
-----------------------------------------------------------------------------

if ioq3.createMissionPackProjects then
project "missionpack_cgame"
	kind "SharedLib"

	configuration "x64"
		targetname "cgamex86_64"
	configuration "not x64"
		targetname "cgamex86"
	configuration {}
	
	defines
	{
		"MISSIONPACK"
	}

	files
	{
		"../code/cgame/*.c",
		"../code/cgame/*.h",
		"../code/game/bg_*.c",
		"../code/game/bg_*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/qcommon/surfaceflags.h",
		"../code/ui/ui_shared.*"
	}
	
	excludes
	{
		"../code/game/bg_lib.*"
	}
	
	links "winmm"

	configuration "Debug"
		if ioq3.outputPath == nil and ioq3.outputMissionPack == nil then
			targetdir "../build/missionpack_cgame_debug"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputMissionPack)
		end
		
		objdir "../build/missionpack_cgame_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil and ioq3.outputMissionPack == nil then
			targetdir "../build/missionpack_cgame_release"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputMissionPack)
		end
		
		objdir "../build/missionpack_cgame_release"
end

-----------------------------------------------------------------------------

if ioq3.createMissionPackProjects then
project "missionpack_game"
	kind "SharedLib"

	configuration "x64"
		targetname "qagamex86_64"
	configuration "not x64"
		targetname "qagamex86"
	configuration {}
	
	defines
	{
		"MISSIONPACK"
	}

	files
	{
		"../code/game/*.c",
		"../code/game/*.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/qcommon/surfaceflags.h"
	}
	
	excludes
	{
		"../code/game/bg_lib.*",
		"../code/game/g_rankings.c"
	}
	
	links "winmm"

	configuration "Debug"
		if ioq3.outputPath == nil and ioq3.outputMissionPack == nil then
			targetdir "../build/missionpack_game_debug"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputMissionPack)
		end
		
		objdir "../build/missionpack_game_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil and ioq3.outputMissionPack == nil then
			targetdir "../build/missionpack_game_release"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputMissionPack)
		end
		
		objdir "../build/missionpack_game_release"
end

-----------------------------------------------------------------------------

if ioq3.createMissionPackProjects then
project "missionpack_ui"
	kind "SharedLib"

	configuration "x64"
		targetname "uix86_64"
	configuration "not x64"
		targetname "uix86"
	configuration {}
	
	files
	{
		"../code/game/bg_misc.c",
		"../code/game/bg_public.h",
		"../code/qcommon/q_math.c",
		"../code/qcommon/q_shared.c",
		"../code/qcommon/q_shared.h",
		"../code/ui/*.c",
		"../code/ui/*.h"
	}
	
	links { "odbc32", "odbccp32" }

	configuration "Debug"
		if ioq3.outputPath == nil and ioq3.outputMissionPack == nil then
			targetdir "../build/missionpack_ui_debug"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputMissionPack)
		end
		
		objdir "../build/missionpack_ui_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil and ioq3.outputMissionPack == nil then
			targetdir "../build/missionpack_ui_release"
		else
			targetdir(ioq3.outputPath .. "\\" .. ioq3.outputMissionPack)
		end
		
		objdir "../build/missionpack_ui_release"
end

-----------------------------------------------------------------------------

project "libsdl"
	kind "SharedLib"
	
	configuration "x64"
		targetname "SDL64"
	configuration "not x64"
		targetname "SDL"
	configuration {}
	
	defines
	{
		"_WINDOWS"
	}

	files
	{
		"../code/SDL12/src/audio/disk/SDL_diskaudio.c",
		"../code/SDL12/src/audio/disk/SDL_diskaudio.h",
		"../code/SDL12/src/audio/dummy/SDL_dummyaudio.c",
		"../code/SDL12/src/audio/dummy/SDL_dummyaudio.h",
		"../code/SDL12/src/audio/*.c",
		"../code/SDL12/src/audio/*.h",
		"../code/SDL12/src/audio/windib/SDL_dibaudio.c",
		"../code/SDL12/src/audio/windib/SDL_dibaudio.h",
		"../code/SDL12/src/audio/windx5/SDL_dx5audio.c",
		"../code/SDL12/src/audio/windx5/SDL_dx5audio.h",
		"../code/SDL12/src/cdrom/SDL_cdrom.c",
		"../code/SDL12/src/cdrom/SDL_syscdrom.h",
		"../code/SDL12/src/cdrom/win32/SDL_syscdrom.c",
		"../code/SDL12/src/cpuinfo/SDL_cpuinfo.c",
		"../code/SDL12/src/events/*.c",
		"../code/SDL12/src/events/*.h",
		"../code/SDL12/src/file/SDL_rwops.c",
		"../code/SDL12/src/joystick/*.c",
		"../code/SDL12/src/joystick/*.h",
		"../code/SDL12/src/joystick/win32/SDL_mmjoystick.c",
		"../code/SDL12/src/loadso/win32/SDL_sysloadso.c",
		"../code/SDL12/src/*.c",
		"../code/SDL12/src/*.h",
		"../code/SDL12/src/stdlib/SDL_getenv.c",
		"../code/SDL12/src/stdlib/SDL_iconv.c",
		"../code/SDL12/src/stdlib/SDL_string.c",
		"../code/SDL12/src/thread/generic/SDL_syscond.c",
		"../code/SDL12/src/thread/*.c",
		"../code/SDL12/src/thread/*.h",
		"../code/SDL12/src/thread/win32/*.c",
		"../code/SDL12/src/thread/win32/*.h",
		"../code/SDL12/src/timer/*.c",
		"../code/SDL12/src/timer/*.h",
		"../code/SDL12/src/timer/win32/SDL_systimer.c",
		"../code/SDL12/src/video/dummy/*.c",
		"../code/SDL12/src/video/dummy/*.h",
		"../code/SDL12/src/video/*.c",
		"../code/SDL12/src/video/*.h",
		"../code/SDL12/src/video/wincommon/*.c",
		"../code/SDL12/src/video/wincommon/*.h",
		"../code/SDL12/src/video/windib/*.c",
		"../code/SDL12/src/video/windib/*.h",
		"../code/SDL12/src/video/Windx5/*.c",
		"../code/SDL12/src/video/Windx5/*.h"
	}
	
	excludes
	{
		"../code/SDL12/src/audio/SDL_audiodev*",
		"../code/SDL12/src/audio/SDL_mixer_MMX.*",
		"../code/SDL12/src/audio/SDL_mixer_m68k.*",
		"../code/SDL12/src/thread/win32/win_ce_semaphore.*",
		"../code/SDL12/src/video/SDL_yuv_mmx.c"
	}
	
	includedirs
	{
		"../code/SDL12/include",
	}
	
	links "winmm"
	
	if dxSdkDir ~= nil then
		includedirs(dxSdkDir .. "Include")
		
		configuration "x64"
			links(dxSdkDir .. "Lib/x64/dxguid")
		
		configuration "not x64"
			links(dxSdkDir .. "Lib/x86/dxguid")
	end
	
	configuration "Debug"
		if ioq3.outputPath == nil then
			targetdir "../build/libsdl_debug"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/libsdl_debug"
				
	configuration "Release"
		if ioq3.outputPath == nil then
			targetdir "../build/libsdl_release"
		else
			targetdir(ioq3.outputPath)
		end
		
		objdir "../build/libsdl_release"
		
-----------------------------------------------------------------------------

project "libspeex"
	kind "StaticLib"
	
	defines
	{
		"HAVE_CONFIG_H",
		
		-- alloca is undefined if this is omitted
		-- x64 needs it too
		"WIN32"
	}
	
	files
	{
		"../code/libspeex/*.c",
		"../code/libspeex/*.h",
		"../code/libspeex/include/speex/*.h"
	}
	
	excludes
	{
		"../code/libspeex/test*.c"
	}
	
	includedirs
	{
		"../code/libspeex/include"
	}
	
	buildoptions
	{
		-- Silence some warnings
		"/wd\"4018\"",
		"/wd\"4047\"",
		"/wd\"4244\"",
		"/wd\"4305\"",
	}
	
	configuration "Debug"
		targetdir "../build/libspeex_debug"
		objdir "../build/libspeex_debug"
				
	configuration "Release"
		targetdir "../build/libspeex_release"
		objdir "../build/libspeex_release"
		
-----------------------------------------------------------------------------

project "zlib"
	kind "StaticLib"

	files
	{
		"../code/zlib/*.c",
		"../code/zlib/*.h"
	}
	
	configuration "Debug"
		targetdir "../build/zlib_debug"
		objdir "../build/zlib_debug"
				
	configuration "Release"
		targetdir "../build/zlib_release"
		objdir "../build/zlib_release"
