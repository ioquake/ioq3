# The MIT License (MIT)
# 
# Copyright (c) 2022 github.com/Pan7
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
cmake_minimum_required(VERSION 2.8.12)

project("client")

set(CLIENT_TARGET "client")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")
include("${CMAKELIST_DIR}/include/git_rev.cmake")
include("${CMAKELIST_DIR}/include/strip.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")
include("${CMAKELIST_DIR}/include/vm_sources.cmake")

include(CheckCSourceCompiles)
include(CheckIncludeFile)


#cmake does append any extensions
set(CLIENT_BIN "${DEFAULT_CLIENT_BIN}" CACHE STRING "binary/executable name")

option(BUILD_STANDALONE "Standalone" ${DEFAULT_BUILD_STANDALONE})

#Note: options and build directories get cached
option(USE_INTERNAL_LIBS "Use internal libraries (Ogg,Vorbis,Opus,zlib,JPEG,...)" ${DEFAULT_USE_INTERNAL_LIBS})
option(USE_INTERNAL_CURL "Use internal cURL library" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_OGG "Use internal Ogg" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_OPENAL "Use internal OpenAL library" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_OPUS "Use internal Opus" ${USE_INTERNAL_LIBS})
#option(USE_INTERNAL_SPEEX "Use internal Speex library" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_SDL "Use internal SDL library" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_VORBIS "Use internal Vorbis" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_ZLIB "Use internal zlib" ${USE_INTERNAL_LIBS})
option(USE_LOCAL_HEADERS "Use local headers" ${USE_INTERNAL_LIBS})

option(USE_CURL "Use cURL" ${DEFAULT_USE_CURL})
option(USE_CURL_DLOPEN "Use cURL dynamic library opening" ${DEFAULT_USE_CURL_DLOPEN})
#on Apple/MacOS the icon of the AppBundle is used
if(NOT PLATFORM STREQUAL "darwin")
	option(USE_ICON "Use window icon" ${DEFAULT_USE_ICON})
endif()
option(USE_OPENAL "Use OpenAL" ${DEFAULT_USE_OPENAL})
option(USE_OPENAL_DLOPEN "Use OpenAL dynamic library opening" ${DEFAULT_USE_OPENAL_DLOPEN})
option(USE_RENDERER_DLOPEN "Use renderer dynamic library opening" ${DEFAULT_USE_RENDERER_DLOPEN})
option(USE_VOIP "Use VOIP" ${DEFAULT_USE_VOIP})
option(USE_MUMBLE "Use Mumble support" ${DEFAULT_USE_MUMBLE})
option(USE_CODEC_OPUS "Use Opus support" ${DEFAULT_USE_CODEC_OPUS})
option(USE_CODEC_VORBIS "Use Vorbis support" ${DEFAULT_USE_CODEC_VORBIS})
option(USE_FREETYPE "Use Freetype2 support" ${DEFAULT_USE_FREETYPE})
option(USE_AUTOUPDATER "Use Autoupdater" ${DEFAULT_USE_AUTOUPDATER})

option(USE_RENDERER_DLOPEN "Use renderer dynamic library opening" ${DEFAULT_USE_RENDERER_DLOPEN})
set(PRODUCT_VERSION "${DEFAULT_VERSION}" CACHE STRING "Product version")
option(USE_GIT_REV "Use Git revision for version" ${DEFAULT_USE_GIT_REV})

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
option(NO_STRIP "No strip (keep all symbol table and relocation information)" ${DEFAULT_NO_STRIP})
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(BIN_EXT "${DEFAULT_BIN_EXT}" CACHE STRING "Binary/executable extension")

set(REL_INSTALL_DEST "${DEFAULT_REL_INSTALL_DEST}" CACHE PATH "Relative installation destination")
set(INSTALL_DIR "${DEFAULT_INSTALL_DIR}" CACHE PATH "Installation directory")

set(BOTLIB_SOURCES
	"${BOTLIB_DIR}/be_aas_bspq3.c"
	"${BOTLIB_DIR}/be_aas_cluster.c"
	"${BOTLIB_DIR}/be_aas_debug.c"
	"${BOTLIB_DIR}/be_aas_entity.c"
	"${BOTLIB_DIR}/be_aas_file.c"
	"${BOTLIB_DIR}/be_aas_main.c"
	"${BOTLIB_DIR}/be_aas_move.c"
	"${BOTLIB_DIR}/be_aas_optimize.c"
	"${BOTLIB_DIR}/be_aas_reach.c"
	"${BOTLIB_DIR}/be_aas_route.c"
	"${BOTLIB_DIR}/be_aas_routealt.c"
	"${BOTLIB_DIR}/be_aas_sample.c"
	"${BOTLIB_DIR}/be_ai_char.c"
	"${BOTLIB_DIR}/be_ai_chat.c"
	"${BOTLIB_DIR}/be_ai_gen.c"
	"${BOTLIB_DIR}/be_ai_goal.c"
	"${BOTLIB_DIR}/be_ai_move.c"
	"${BOTLIB_DIR}/be_ai_weap.c"
	"${BOTLIB_DIR}/be_ai_weight.c"
	"${BOTLIB_DIR}/be_ea.c"
	"${BOTLIB_DIR}/be_interface.c"
	"${BOTLIB_DIR}/l_crc.c"
	"${BOTLIB_DIR}/l_libvar.c"
	"${BOTLIB_DIR}/l_log.c"
	"${BOTLIB_DIR}/l_memory.c"
	"${BOTLIB_DIR}/l_precomp.c"
	"${BOTLIB_DIR}/l_script.c"
	"${BOTLIB_DIR}/l_struct.c"
)
set(BOTLIB_HEADERS
	"${BOTLIB_DIR}/aasfile.h"
	"${BOTLIB_DIR}/be_aas.h"
	"${BOTLIB_DIR}/be_aas_bsp.h"
	"${BOTLIB_DIR}/be_aas_cluster.h"
	"${BOTLIB_DIR}/be_aas_debug.h"
	"${BOTLIB_DIR}/be_aas_def.h"
	"${BOTLIB_DIR}/be_aas_entity.h"
	"${BOTLIB_DIR}/be_aas_file.h"
	"${BOTLIB_DIR}/be_aas_funcs.h"
	"${BOTLIB_DIR}/be_aas_main.h"
	"${BOTLIB_DIR}/be_aas_move.h"
	"${BOTLIB_DIR}/be_aas_optimize.h"
	"${BOTLIB_DIR}/be_aas_reach.h"
	"${BOTLIB_DIR}/be_aas_route.h"
	"${BOTLIB_DIR}/be_aas_routealt.h"
	"${BOTLIB_DIR}/be_aas_sample.h"
	"${BOTLIB_DIR}/be_ai_char.h"
	"${BOTLIB_DIR}/be_ai_chat.h"
	"${BOTLIB_DIR}/be_ai_gen.h"
	"${BOTLIB_DIR}/be_ai_goal.h"
	"${BOTLIB_DIR}/be_ai_move.h"
	"${BOTLIB_DIR}/be_ai_weap.h"
	"${BOTLIB_DIR}/be_ai_weight.h"
	"${BOTLIB_DIR}/be_ea.h"
	"${BOTLIB_DIR}/be_interface.h"
	"${BOTLIB_DIR}/botlib.h"
	"${BOTLIB_DIR}/l_crc.h"
	"${BOTLIB_DIR}/l_libvar.h"
	"${BOTLIB_DIR}/l_log.h"
	"${BOTLIB_DIR}/l_memory.h"
	"${BOTLIB_DIR}/l_precomp.h"
	"${BOTLIB_DIR}/l_script.h"
	"${BOTLIB_DIR}/l_struct.h"
	"${BOTLIB_DIR}/l_utils.h"
)
set(CLIENT_SOURCES
	"${CLIENT_DIR}/cl_avi.c"
	"${CLIENT_DIR}/cl_cgame.c"
	"${CLIENT_DIR}/cl_cin.c"
	"${CLIENT_DIR}/cl_console.c"
	"${CLIENT_DIR}/cl_curl.c"
	"${CLIENT_DIR}/cl_input.c"
	"${CLIENT_DIR}/cl_keys.c"
	"${CLIENT_DIR}/cl_main.c"
	"${CLIENT_DIR}/cl_net_chan.c"
	"${CLIENT_DIR}/cl_parse.c"
	"${CLIENT_DIR}/cl_scrn.c"
	"${CLIENT_DIR}/cl_ui.c"
#	"${CLIENT_DIR}/libmumblelink.c"
	"${CLIENT_DIR}/qal.c"
	"${CLIENT_DIR}/snd_adpcm.c"
	"${CLIENT_DIR}/snd_codec.c"
	"${CLIENT_DIR}/snd_codec_ogg.c"
	"${CLIENT_DIR}/snd_codec_opus.c"
	"${CLIENT_DIR}/snd_codec_wav.c"
	"${CLIENT_DIR}/snd_dma.c"
	"${CLIENT_DIR}/snd_main.c"
	"${CLIENT_DIR}/snd_mem.c"
	"${CLIENT_DIR}/snd_mix.c"
	"${CLIENT_DIR}/snd_openal.c"
	"${CLIENT_DIR}/snd_wavelet.c"
)
set(CLIENT_HEADERS
	"${CLIENT_DIR}/cl_curl.h"
	"${CLIENT_DIR}/client.h"
	"${CLIENT_DIR}/keycodes.h"
	"${CLIENT_DIR}/keys.h"
	"${CLIENT_DIR}/libmumblelink.h"
	"${CLIENT_DIR}/qal.h"
	"${CLIENT_DIR}/snd_codec.h"
	"${CLIENT_DIR}/snd_local.h"
	"${CLIENT_DIR}/snd_public.h"
)
set(CLIENT_MUMBLE_SOURCES
	"${CLIENT_DIR}/libmumblelink.c"
)

include("${CMAKELIST_DIR}/include/libsdl_headers.cmake")

set(QCOMMON_SOURCES 
	"${QCOMMON_DIR}/cm_load.c"
	"${QCOMMON_DIR}/cm_patch.c"
	"${QCOMMON_DIR}/cm_polylib.c"
	"${QCOMMON_DIR}/cm_test.c"
	"${QCOMMON_DIR}/cm_trace.c"
	"${QCOMMON_DIR}/cmd.c"
	"${QCOMMON_DIR}/common.c"
	"${QCOMMON_DIR}/cvar.c"
	"${QCOMMON_DIR}/files.c"
	"${QCOMMON_DIR}/huffman.c"
	"${QCOMMON_DIR}/ioapi.c"
	"${QCOMMON_DIR}/md4.c"
	"${QCOMMON_DIR}/md5.c"
	"${QCOMMON_DIR}/msg.c"
	"${QCOMMON_DIR}/net_chan.c"
	"${QCOMMON_DIR}/net_ip.c"
	"${QCOMMON_DIR}/puff.c"
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
	"${QCOMMON_DIR}/unzip.c"
	"${QCOMMON_DIR}/vm.c"
	"${QCOMMON_DIR}/vm_interpreted.c"
#	"${QCOMMON_DIR}/vm_none.c"
#	"${QCOMMON_DIR}/vm_armv7l.c"
#	"${QCOMMON_DIR}/vm_powerpc.c"
#	"${QCOMMON_DIR}/vm_powerpc_asm.c"
#	"${QCOMMON_DIR}/vm_sparc.c"
#	"${QCOMMON_DIR}/vm_x86.c"
)
set(QCOMMON_HEADERS 
	"${QCOMMON_DIR}/cm_local.h"
	"${QCOMMON_DIR}/cm_patch.h"
	"${QCOMMON_DIR}/cm_polylib.h"
	"${QCOMMON_DIR}/cm_public.h"
	"${QCOMMON_DIR}/ioapi.h"
	"${QCOMMON_DIR}/puff.h"
	"${QCOMMON_DIR}/q_platform.h"
	"${QCOMMON_DIR}/q_shared.h"
	"${QCOMMON_DIR}/qcommon.h"
	"${QCOMMON_DIR}/qfiles.h"
	"${QCOMMON_DIR}/surfaceflags.h"
	"${QCOMMON_DIR}/unzip.h"
	"${QCOMMON_DIR}/vm_local.h"
#	"${QCOMMON_DIR}/vm_powerpc_asm.h"
#	"${QCOMMON_DIR}/vm_sparc.h"
)

set(SDL_SOURCES 
#	"${SDL_DIR}/sdl_gamma.c"
#	"${SDL_DIR}/sdl_glimp.c"
	"${SDL_DIR}/sdl_input.c"
	"${SDL_DIR}/sdl_snd.c"
)
set(SDL_HEADERS 
	"${SDL_DIR}/sdl_icon.h"
)


set(SERVER_SOURCES 
	"${SERVER_DIR}/sv_bot.c"
	"${SERVER_DIR}/sv_ccmds.c"
	"${SERVER_DIR}/sv_client.c"
	"${SERVER_DIR}/sv_game.c"
	"${SERVER_DIR}/sv_init.c"
	"${SERVER_DIR}/sv_main.c"
	"${SERVER_DIR}/sv_net_chan.c"
#	"${SERVER_DIR}/sv_rankings.c"
	"${SERVER_DIR}/sv_snapshot.c"
	"${SERVER_DIR}/sv_world.c"
)
set(SERVER_HEADERS
	"${SERVER_DIR}/server.h"
)

set(SYS_SOURCES 
	"${SYS_DIR}/con_log.c"
#	"${SYS_DIR}/con_passive.c"
#	"${SYS_DIR}/con_tty.c"
#	"${SYS_DIR}/con_win32.c"
	"${SYS_DIR}/sys_autoupdater.c"
	"${SYS_DIR}/sys_main.c"
#	"${SYS_DIR}/sys_unix.c"
#	"${SYS_DIR}/sys_win32.c"
)
set(SYS_HEADERS 
	"${SYS_DIR}/sys_loadlib.h"
	"${SYS_DIR}/sys_local.h"
	"${SYS_DIR}/win_resource.h"
)
list(APPEND SYS_RC_SOURCES 
	"${SYS_DIR}/win_manifest.xml"
	"${SYS_DIR}/win_resource.rc"
)
list(APPEND SYS_WIN_SOURCES 
	"${SYS_DIR}/con_passive.c"
	"${SYS_DIR}/sys_win32.c"
#	"${SYS_DIR}/con_win32.c"
)
list(APPEND SYS_UNIX_SOURCES 
#	"${SYS_DIR}/con_unix.c"
	"${SYS_DIR}/con_tty.c"
	"${SYS_DIR}/sys_unix.c"
)
list(APPEND SYS_OSX_SOURCES 
#	"${SYS_DIR}/sys_osx.c"
	"${SYS_DIR}/sys_osx.m"
)


if(WIN32)
	#requires parent directory for misc/quake3.ico in win_resource.rc
	list(APPEND CLIENT_INCLUDE_DIRS "${SOURCE_DIR}")
endif(WIN32)

if(MSVC)
	if(NOT SDL_FOUND AND USE_INTERNAL_SDL)
		if(ARCH STREQUAL "x86_64")
			list(APPEND SDL_LIBRARIES 
				"${LIBS_DIR}/win64/SDL2main.lib"
				"${LIBS_DIR}/win64/SDL2.lib" 
			)
			set(SDL_FOUND TRUE)
		elseif(ARCH STREQUAL "x86")
			list(APPEND SDL_LIBRARIES 
				"${LIBS_DIR}/win32/SDL2main.lib"
				"${LIBS_DIR}/win32/SDL2.lib" 
			)
			set(SDL_FOUND TRUE)

		endif()

		if(SDL_FOUND)
			foreach(FILEPATH ${SDL_LIBRARIES})
				if(NOT EXISTS "${FILEPATH}")
					message(WARNING "Could not find the file for MSVC: ${FILEPATH}\n")
				endif()
			endforeach(FILEPATH)
		endif(SDL_FOUND)
	endif()

	if(USE_CURL AND NOT USE_CURL_DLOPEN AND USE_INTERNAL_CURL)
		if(ARCH STREQUAL "x86_64")
			list(APPEND CURL_LIBRARIES "${LIBS_DIR}/win64/curl.lib")
			set(CURL_FOUND TRUE)
		elseif(ARCH STREQUAL "x86")
			list(APPEND CURL_LIBRARIES "${LIBS_DIR}/win32/curl.lib")
			set(CURL_FOUND TRUE)

		endif()
		if(CURL_FOUND)
			list(APPEND CURL_DEFINITIONS "-DCURL_STATICLIB")
			foreach(FILEPATH ${CURL_LIBRARIES})
				if(NOT EXISTS "${FILEPATH}")
					message(WARNING "Could not find the file for MSVC: ${FILEPATH}\n")
				endif()
			endforeach(FILEPATH)
		endif(CURL_FOUND)
	endif()

	include(CheckLanguage)
	check_language(ASM_MASM)
	if(CMAKE_ASM_MASM_COMPILER)
		enable_language(ASM_MASM)
		set(HAVE_MASM_COMPILER TRUE)
	endif(CMAKE_ASM_MASM_COMPILER)

	if(NOT HAVE_MASM_COMPILER)
		message(WARNING "Microsoft ASM compiler is required!")
	else()
		if(NOT (ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64"))
			message(WARNING "Only architecture x86 and x86_64 are supported for MSVC!")
		endif()
	endif()

	set(USE_MASM_SOURCES TRUE)

	list(APPEND SYS_SOURCES ${SYS_WIN_SOURCES})

	#Character Set, CharacterSet="0", Not Set, ASCII/SBCS (Single Byte Character Set)
	list(APPEND CLIENT_DEFINITIONS "-D_SBCS")

	#disable deprecation warnings about old functions like strcmp
	list(APPEND CLIENT_DEFINITIONS "-D_CRT_SECURE_NO_WARNINGS")

	#fix msvc 2019 not setting __i386__
	if(ARCH STREQUAL "x86")
		list(APPEND CLIENT_DEFINITIONS "-D__i386__")
	endif()

	#Whole program optimization, no /GL-
	list(APPEND CLIENT_CFLAGS "/GL-")

	#Warning Level /W4
	list(APPEND CLIENT_CFLAGS "/W4")

	#Enable String Pooling /GF
	list(APPEND CLIENT_CFLAGS "/GF")
	#Enable C++ Exceptions, No /EHa-
	list(APPEND CLIENT_CFLAGS "/EHa-")
	#Enable Function-Level Linking /Gy
	list(APPEND CLIENT_CFLAGS "/Gy")
	#Floating Point Model /fp:fast
	list(APPEND CLIENT_CFLAGS "/fp:fast")

	#Compile as, C Code /TC
	list(APPEND CLIENT_CFLAGS "/TC")

	#Disable specific warnings
	#warning C4706: assignment within conditional expression
	list(APPEND CLIENT_CFLAGS "/wd4706")
	#warning C4389: '!=' : signed/unsigned mismatch
	list(APPEND CLIENT_CFLAGS "/wd4389")
	#warning C4100: 'opaque' : unreferenced formal parameter
	list(APPEND CLIENT_CFLAGS "/wd4100")
	#warning C4131: 'fill_fopen_filefunc' : uses old-style declarator
	list(APPEND CLIENT_CFLAGS "/wd4131")
	#warning C4245: 'return' : conversion from 'int' to 'uLong', signed/unsigned mismatch
	list(APPEND CLIENT_CFLAGS "/wd4245")

	#Subsystem /SUBSYSTEM:WINDOWS
	list(APPEND CLIENT_LDFLAGS "/SUBSYSTEM:WINDOWS")
	#Stack Reserve Size /STACK:8388608
	list(APPEND CLIENT_LDFLAGS "/STACK:8388608")
	#Randomized Base Address /DYNAMICBASE:NO
	list(APPEND CLIENT_LDFLAGS "/DYNAMICBASE:NO")
	#Data Execution Prevenbtion (DEP) /NXCOMPAT:NO
	list(APPEND CLIENT_LDFLAGS "/NXCOMPAT:NO")
	#Image Has Safe Exception Handlers, is no longer default no in msvc 2013, /SAFESEH:NO
	list(APPEND CLIENT_LDFLAGS "/SAFESEH:NO")

	#not generating manifest, using win_manifest.xml
	list(APPEND CLIENT_LDFLAGS "/MANIFEST:NO")

	#time functions winmm.lib
	#EnumProcesses function psapi.lib
	#windows socket functions ws2_32.lib
	list(APPEND CLIENT_LIBRARIES "winmm" "psapi" "ws2_32")

	#requires directory misc/msvc for ../quake3.ico in win_resource.rc
	list(APPEND CLIENT_INCLUDE_DIRS "${SOURCE_DIR}/misc/msvc")

endif(MSVC)

if(MINGW AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	if(NOT SDL_FOUND AND USE_INTERNAL_SDL)
		if(ARCH STREQUAL "x86_64")
			list(APPEND SDL_LIBRARIES 
				"${LIBS_DIR}/win64/libSDL264main.a"
				"${LIBS_DIR}/win64/libSDL264.dll.a" 
			)
			set(SDL_FOUND TRUE)
		elseif(ARCH STREQUAL "x86")
			list(APPEND SDL_LIBRARIES 
				"${LIBS_DIR}/win32/libSDL2main.a"
				"${LIBS_DIR}/win32/libSDL2.dll.a" 
			)
			set(SDL_FOUND TRUE)

		endif()
		if(SDL_FOUND)
			foreach(FILEPATH ${SDL_LIBRARIES})
				if(NOT EXISTS "${FILEPATH}")
					message(WARNING "Could not find the file for MinGW: ${FILEPATH}\n")
				endif()
			endforeach(FILEPATH)
		endif(SDL_FOUND)
	endif()

	if(USE_CURL AND NOT USE_CURL_DLOPEN AND USE_INTERNAL_CURL)
		if(ARCH STREQUAL "x86_64")
			list(APPEND CURL_LIBRARIES "${LIBS_DIR}/win64/libcurl.a")
			set(CURL_FOUND TRUE)
		elseif(ARCH STREQUAL "x86")
			list(APPEND CURL_LIBRARIES "${LIBS_DIR}/win32/libcurl.a")
			set(CURL_FOUND TRUE)

		endif()
		if(CURL_FOUND)
			list(APPEND CURL_DEFINITIONS "-DCURL_STATICLIB")

			foreach(FILEPATH ${CURL_LIBRARIES})
				if(NOT EXISTS "${FILEPATH}")
					message(WARNING "Could not find the file for MinGW: ${FILEPATH}\n")
				endif()
			endforeach(FILEPATH)
			#MinGW: These libs need to be after libcurl
			list(APPEND CURL_LIBRARIES "Ws2_32" "Crypt32")
		endif(CURL_FOUND)
	endif()

	list(APPEND SYS_SOURCES ${SYS_WIN_SOURCES})

	#link the MinGW library first and especially before SDL
	#32-bit and 64-bit link to mingw32
	list(INSERT CLIENT_LIBRARIES 0 "mingw32")

	#time functions winmm.lib
	#EnumProcesses function psapi.lib
	#socket functions Ws2_32.lib
	#ImmGetIMEFileName imm32.lib
	#GetFileVersionInfoSize Version.lib
	#Certificate functions Crypt32.lib
	list(APPEND CLIENT_LIBRARIES "imm32" "Version" "winmm" "psapi" "Ws2_32")

	#WINVER is the minimum operating system version the code supports for the Windows SDK.
	# 0x0501 for Windows XP, 0x0502 for Windows Server 2003,
	# 0x0600 for Windows Vista, 0x0601 for Windows 7,
	# 0x0602 for Windows 8, 0x603 for Windows 8.1 and 0x0A00 for Windows 10.
	list(APPEND CLIENT_DEFINITIONS "-DWINVER=0x0501")

endif()

if(PLATFORM STREQUAL "darwin" AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	if(NOT SDL_FOUND AND USE_INTERNAL_SDL)
		list(APPEND SDL_LIBRARIES
			"${LIBS_DIR}/macosx/libSDL2main.a"
			"${LIBS_DIR}/macosx/libSDL2-2.0.0.dylib" 
		)
		set(SDL_FOUND TRUE)

		foreach(FILEPATH ${SDL_LIBRARIES})
			if(NOT EXISTS "${FILEPATH}")
				message(WARNING "Could not find the file for MacOS: ${FILEPATH}\n")
			endif()
		endforeach(FILEPATH)

	endif()

	if(ENV{MACOSX_VERSION_MIN})
		set(MACOSX_VERSION_MIN "$ENV{MACOSX_VERSION_MIN}")
	elseif(NOT MACOSX_VERSION_MIN)
		set(MACOSX_VERSION_MIN "10.7")
	endif()

	string(REPLACE "." ";" VERSION_LIST ${MACOSX_VERSION_MIN})
	list(GET VERSION_LIST 0 MACOSX_MAJOR)
	list(GET VERSION_LIST 1 MACOSX_MINOR)
#	list(GET VERSION_LIST 2 MACOSX_PATCH)

	if(${MACOSX_MINOR} GREATER 9)
		# Multiply and then remove decimal. 10.10 -> 101000.0 -> 101000
		math(EXPR MAC_OS_X_VERSION_MIN_REQUIRED "${MACOSX_MAJOR} * 10000 + ${MACOSX_MINOR} * 100")
	else()
		# Multiply by 100 and then remove decimal. 10.7 -> 1070.0 -> 1070
		math(EXPR MAC_OS_X_VERSION_MIN_REQUIRED "${MACOSX_MAJOR} * 100 + ${MACOSX_MINOR} * 10")
	endif()

	list(APPEND CLIENT_DEFINITIONS "-DMAC_OS_X_VERSION_MIN_REQUIRED=${MAC_OS_X_VERSION_MIN_REQUIRED}")
	list(APPEND CLIENT_CFLAGS "-mmacosx-version-min=${MACOSX_VERSION_MIN}")
	list(APPEND CLIENT_LDFLAGS "-mmacosx-version-min=${MACOSX_VERSION_MIN}")


	list(APPEND SYS_SOURCES ${SYS_OSX_SOURCES})
	set_source_files_properties("${SYS_DIR}/sys_osx.m" PROPERTIES LANGUAGE C)

	list(APPEND CLIENT_DEFINITIONS "-DMACOS_X")
	#NSConstantString NSString
	find_library(COCOA_LIBRARY Cocoa)
	if(COCOA_LIBRARY)
		list(APPEND CLIENT_LIBRARIES "${COCOA_LIBRARY}")
	else()
		list(APPEND CLIENT_LDFLAGS "-framework Cocoa")
	endif()

endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	if(ARCH STREQUAL "x86") #not for x86_64
		set(USE_ASM_SOURCES TRUE)
	endif()
	if(ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64")
		set(USE_INLINE_ASM_SOURCES TRUE)
	endif()

	CHECK_C_SOURCE_COMPILES("int foo(const char *str, ...) __attribute__((format(printf, 1, 2))); int main() {return 0;}" HAVE_GCC_FORMAT)
	if(HAVE_GCC_FORMAT)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_GCC_FORMAT")
	endif(HAVE_GCC_FORMAT)

	if(NOT MINGW)
		list(APPEND SYS_SOURCES ${SYS_UNIX_SOURCES})
	endif()

endif()

if(USE_INLINE_ASM_SOURCES)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_inline_asm")
	list(APPEND CLIENT_LIBRARIES ${INTERNAL_INLINE_ASM_LIBRARIES})
endif()

if(USE_MASM_SOURCES)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_masm")
	list(APPEND CLIENT_LIBRARIES ${INTERNAL_MASM_LIBRARIES})
endif()

if(USE_ASM_SOURCES)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_asm")
	list(APPEND CLIENT_LIBRARIES ${INTERNAL_ASM_LIBRARIES})
endif()

ADD_CLIENT_DEFINITIONS(CLIENT_DEFINITIONS)
ADD_CLIENT_CFLAGS(CLIENT_CFLAGS)
ADD_STRIP_LDFLAG(CLIENT_LDFLAGS)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	list(APPEND CLIENT_CFLAGS "-Qunused-arguments")
endif()

GET_VM_SOURCES(VM_SOURCES)
if(VM_SOURCES)
	list(APPEND QCOMMON_SOURCES ${VM_SOURCES})
else()
	set(NO_VM_COMPILED TRUE)
endif()
if(NO_VM_COMPILED)
	list(APPEND CLIENT_DEFINITIONS "-DNO_VM_COMPILED")
endif()



if(USE_GIT_REV AND GIT_REV)
	list(APPEND CLIENT_DEFINITIONS "-DPRODUCT_VERSION=\"${PRODUCT_VERSION}_GIT_${GIT_REV}\"")
else()
	list(APPEND CLIENT_DEFINITIONS "-DPRODUCT_VERSION=\"${PRODUCT_VERSION}\"")
endif()

if(DEFINED ENV{SOURCE_DATE_EPOCH})
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.0")
		string(TIMESTAMP PRODUCT_DATE "%b %d %Y" UTC) #Nov 28 2021
		list(APPEND CLIENT_DEFINITIONS "-DPRODUCT_DATE=\"${PRODUCT_DATE}\"")
	else()
		message(WARNING "Update CMake to version 3.8 or later for SOURCE_DATE_EPOCH support!")
	endif()
endif()

if(USE_INTERNAL_SDL)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_SDL")
	if(ARCH STREQUAL "ppc" OR ARCH STREQUAL "ppc64")
		list(APPEND SDL_INCLUDE_DIRS "${LIBSDL_DIR}/include-macppc")
	else()
		list(APPEND SDL_INCLUDE_DIRS "${LIBSDL_DIR}/include")
	endif()
	list(APPEND CLIENT_C_HEADERS ${LIBSDL_HEADERS})

elseif(NOT SDL_FOUND)
	#find_package sets *_FOUND *_INCLUDE_DIRS *_LIBRARIES (*_INCLUDE_DIR *_LIBRARY)
	find_package(SDL REQUIRED)
	if(DEFINED SDL_VERSION AND NOT SDL_VERSION STREQUAL "")
		if(${SDL_VERSION} VERSION_LESS 2)
			message(FATAL_ERROR "The SDL library version is ${SDL_VERSION} but required is at least version 2.")
		endif()
	endif()

endif(USE_INTERNAL_SDL)

list(APPEND CLIENT_DEFINITIONS ${SDL_DEFINITIONS})
if(SDL_INCLUDE_DIR)
	list(APPEND CLIENT_INCLUDE_DIRS "${SDL_INCLUDE_DIR}")
endif(SDL_INCLUDE_DIR)
list(APPEND CLIENT_INCLUDE_DIRS ${SDL_INCLUDE_DIRS})
list(APPEND CLIENT_LIBRARIES "${SDL_LIBRARY}")
list(APPEND CLIENT_LIBRARIES ${SDL_LIBRARIES})

if(USE_INTERNAL_ZLIB)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_ZLIB")
	if(NOT DEFINED INTERNAL_ZLIB_LIBRARIES)
		add_subdirectory("${CMAKELIST_DIR}/internal_zlib" "internal_zlib")
	endif()
	list(APPEND CLIENT_DEFINITIONS ${INTERNAL_ZLIB_DEFINITIONS})
	list(APPEND CLIENT_INCLUDE_DIRS ${INTERNAL_ZLIB_INCLUDE_DIRS})
	list(APPEND CLIENT_LIBRARIES ${INTERNAL_ZLIB_LIBRARIES})
else()
	find_package(ZLIB REQUIRED)
	list(APPEND CLIENT_DEFINITIONS ${ZLIB_DEFINITIONS})
	list(APPEND CLIENT_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS})
	list(APPEND CLIENT_LIBRARIES ${ZLIB_LIBRARIES})
endif(USE_INTERNAL_ZLIB)

if(USE_CODEC_OPUS)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_CODEC_OPUS")
	set(NEED_OGG TRUE)

	if(USE_INTERNAL_OPUS)
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_opusfile")
#		list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_OPUS")
		list(APPEND CLIENT_DEFINITIONS ${INTERNAL_OPUSFILE_DEFINITIONS})
		list(APPEND CLIENT_INCLUDE_DIRS ${INTERNAL_OPUSFILE_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES ${INTERNAL_OPUSFILE_LIBRARIES})
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_opus")
#		list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_OPUSFILE")
		list(APPEND CLIENT_DEFINITIONS ${INTERNAL_OPUS_DEFINITIONS})
		list(APPEND CLIENT_INCLUDE_DIRS ${INTERNAL_OPUS_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES ${INTERNAL_OPUS_LIBRARIES})
	else()
		find_package(OPUSFILE REQUIRED)
		find_package(OPUS REQUIRED)
		list(APPEND CLIENT_DEFINITIONS ${OPUSFILE_DEFINITIONS})
		if(OPUSFILE_INCLUDE_DIR)
			list(APPEND CLIENT_INCLUDE_DIRS "${OPUSFILE_INCLUDE_DIR}")
		endif(OPUSFILE_INCLUDE_DIR)
		list(APPEND CLIENT_INCLUDE_DIRS ${OPUSFILE_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES "${OPUSFILE_LIBRARY}")
		list(APPEND CLIENT_LIBRARIES ${OPUSFILE_LIBRARIES})

		list(APPEND CLIENT_DEFINITIONS ${OPUS_DEFINITIONS})
		if(OPUS_INCLUDE_DIR)
			list(APPEND CLIENT_INCLUDE_DIRS "${OPUS_INCLUDE_DIR}")
		endif(OPUS_INCLUDE_DIR)
		list(APPEND CLIENT_INCLUDE_DIRS ${OPUS_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES "${OPUS_LIBRARY}")
		list(APPEND CLIENT_LIBRARIES ${OPUS_LIBRARIES})

	endif(USE_INTERNAL_OPUS)

endif(USE_CODEC_OPUS)

if(USE_CODEC_VORBIS)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_CODEC_VORBIS")
	set(NEED_OGG TRUE)

	if(USE_INTERNAL_VORBIS)
#		list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_VORBIS")
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_vorbis")
		list(APPEND CLIENT_DEFINITIONS ${INTERNAL_VORBIS_DEFINITIONS})
		list(APPEND CLIENT_INCLUDE_DIRS ${INTERNAL_VORBIS_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES ${INTERNAL_VORBIS_LIBRARIES})
	else()
		find_package(VORBIS REQUIRED)
		list(APPEND CLIENT_DEFINITIONS ${VORBIS_DEFINITIONS})
		if(VORBIS_INCLUDE_DIR)
			list(APPEND CLIENT_INCLUDE_DIRS "${VORBIS_INCLUDE_DIR}")
		endif(VORBIS_INCLUDE_DIR)
		list(APPEND CLIENT_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES "${VORBIS_LIBRARY}")
		list(APPEND CLIENT_LIBRARIES ${VORBIS_LIBRARIES})
	endif(USE_INTERNAL_VORBIS)

endif(USE_CODEC_VORBIS)

if(NEED_OGG)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_CODEC_OGG")

	if(USE_INTERNAL_OGG)
#		list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_OGG")
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_ogg")
		list(APPEND CLIENT_DEFINITIONS ${INTERNAL_OGG_DEFINITIONS})
		list(APPEND CLIENT_INCLUDE_DIRS ${INTERNAL_OGG_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES ${INTERNAL_OGG_LIBRARIES})
	else()
		find_package(OGG REQUIRED)
		list(APPEND CLIENT_DEFINITIONS ${OGG_DEFINITIONS})
		if(OGG_INCLUDE_DIR)
			list(APPEND CLIENT_INCLUDE_DIRS "${OGG_INCLUDE_DIR}")
		endif(OGG_INCLUDE_DIR)
		list(APPEND CLIENT_INCLUDE_DIRS ${OGG_INCLUDE_DIRS})
		list(APPEND CLIENT_LIBRARIES "${OGG_LIBRARY}")
		list(APPEND CLIENT_LIBRARIES ${OGG_LIBRARIES})
	endif(USE_INTERNAL_OGG)

endif(NEED_OGG)

if(USE_MUMBLE)
	CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)
	if(HAVE_UNISTD_H)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_UNISTD_H")
	endif(HAVE_UNISTD_H)

	CHECK_INCLUDE_FILE("sys/mman.h" HAVE_SYS_MMAN_H)
	if(HAVE_SYS_MMAN_H)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_SYS_MMAN_H")
	endif(HAVE_SYS_MMAN_H)

	#For mode constants
	CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)
	if(HAVE_SYS_STAT_H)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_SYS_STAT_H")
	endif(HAVE_SYS_STAT_H)

	#For O_* constants
	CHECK_INCLUDE_FILE("fcntl.h" HAVE_FCNTL_H)
	if(HAVE_FCNTL_H)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_FCNTL_H")
	endif(HAVE_FCNTL_H)

	CHECK_C_SOURCE_COMPILES("#include <sys/mman.h>\n int main () {int shmfd;shmfd = shm_open(\"\", 0, 0);return 0;}" HAVE_SHM_OPEN)
	if(HAVE_SHM_OPEN)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_SHM_OPEN")
	endif(HAVE_SHM_OPEN)

	set(CMAKE_REQUIRED_LIBRARIES "rt")
	CHECK_C_SOURCE_COMPILES("#include <sys/mman.h>\n int main () {int shmfd;shmfd = shm_open(\"\", 0, 0);return 0;}" HAVE_SHM_OPEN_IN_RT)
	set(CMAKE_REQUIRED_LIBRARIES)
	if(HAVE_SHM_OPEN_IN_RT)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_SHM_OPEN")
		list(APPEND CLIENT_LIBRARIES "rt")
	endif(HAVE_SHM_OPEN_IN_RT)

	CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)
	if(HAVE_STDINT_H)
		list(APPEND CLIENT_DEFINITIONS "-DHAVE_STDINT_H")
	endif(HAVE_STDINT_H)

	list(APPEND CLIENT_DEFINITIONS "-DUSE_MUMBLE")
	list(APPEND CLIENT_C_SOURCES ${CLIENT_MUMBLE_SOURCES})

endif(USE_MUMBLE)

if(USE_VOIP)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_VOIP")
endif(USE_VOIP)

if(USE_FREETYPE)
	find_package(Freetype REQUIRED)
	if(DEFINED FREETYPE_VERSION AND NOT FREETYPE_VERSION STREQUAL "")
		if(${FREETYPE_VERSION} VERSION_LESS 2)
			message(FATAL_ERROR "The freetype library version is ${FREETYPE_VERSION} but required is at least version 2.")
		endif()
	endif()

	list(APPEND CLIENT_DEFINITIONS "-DBUILD_FREETYPE")

	list(APPEND CLIENT_DEFINITIONS ${FREETYPE_DEFINITIONS})
	if(FREETYPE_INCLUDE_DIR)
		list(APPEND CLIENT_INCLUDE_DIRS "${FREETYPE_INCLUDE_DIR}")
	endif(FREETYPE_INCLUDE_DIR)
	list(APPEND CLIENT_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS})
	list(APPEND CLIENT_LIBRARIES "${FREETYPE_LIBRARY}")
	list(APPEND CLIENT_LIBRARIES ${FREETYPE_LIBRARIES})

endif(USE_FREETYPE)

#required for l_precomp.c and l_script.c
list(APPEND CLIENT_DEFINITIONS "-DBOTLIB")

if(USE_CURL)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_CURL")
	if(USE_CURL_DLOPEN)
		list(APPEND CLIENT_DEFINITIONS "-DUSE_CURL_DLOPEN")
	endif(USE_CURL_DLOPEN)

	if(USE_INTERNAL_CURL)
		list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_CURL")
		list(APPEND CURL_INCLUDE_DIRS "${CURL_DIR}")

	elseif(NOT CURL_FOUND)
		#CURLOPT_PROTOCOLS was added in 7.19.4
		find_package(CURL REQUIRED)
	endif(USE_INTERNAL_CURL)

	list(APPEND CLIENT_DEFINITIONS ${CURL_DEFINITIONS})
	if(CURL_INCLUDE_DIR)
		list(APPEND CLIENT_INCLUDE_DIRS "${CURL_INCLUDE_DIR}")
	endif(CURL_INCLUDE_DIR)
	list(APPEND CLIENT_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
	list(APPEND CLIENT_LIBRARIES "${CURL_LIBRARY}")
	list(APPEND CLIENT_LIBRARIES ${CURL_LIBRARIES})

endif(USE_CURL)

if(USE_ICON)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_ICON")
endif()

if(USE_LOCAL_HEADERS)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_LOCAL_HEADERS")
endif(USE_LOCAL_HEADERS)

if(USE_OPENAL)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_OPENAL")
	if(USE_OPENAL_DLOPEN)
		list(APPEND CLIENT_DEFINITIONS "-DUSE_OPENAL_DLOPEN")
	endif(USE_OPENAL_DLOPEN)
	if(USE_INTERNAL_OPENAL)
		list(APPEND CLIENT_DEFINITIONS "-DUSE_INTERNAL_OPENAL")
	endif(USE_INTERNAL_OPENAL)
endif(USE_OPENAL)

if(USE_RENDERER_DLOPEN)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_RENDERER_DLOPEN")
endif(USE_RENDERER_DLOPEN)

if(BUILD_STANDALONE)
	list(APPEND CLIENT_DEFINITIONS "-DSTANDALONE")
endif(BUILD_STANDALONE)

if(DEFAULT_BASEDIR)
	list(APPEND CLIENT_DEFINITIONS "-DDEFAULT_BASEDIR=\"${DEFAULT_BASEDIR}\"")
endif(DEFAULT_BASEDIR)

if(USE_AUTOUPDATER)
	list(APPEND CLIENT_DEFINITIONS "-DUSE_AUTOUPDATER")
	if(NOT DEFINED AUTOUPDATER_BIN)
		set(AUTOUPDATER_BIN ${DEFAULT_AUTOUPDATER_BIN})
	endif()
	list(APPEND CLIENT_DEFINITIONS "-DAUTOUPDATER_BIN=\"${AUTOUPDATER_BIN}\"")
endif(USE_AUTOUPDATER)

list(APPEND CLIENT_C_SOURCES ${SDL_SOURCES} ${SERVER_SOURCES} ${BOTLIB_SOURCES} ${QCOMMON_SOURCES} ${CLIENT_SOURCES} ${SYS_SOURCES})
list(APPEND CLIENT_C_HEADERS ${SDL_HEADERS} ${SERVER_HEADERS} ${BOTLIB_HEADERS} ${QCOMMON_HEADERS} ${CLIENT_HEADERS} ${SYS_HEADERS})

#IDE grouping
source_group("botlib\\Source Files" FILES ${BOTLIB_SOURCES})
source_group("botlib\\Header Files" FILES ${BOTLIB_HEADERS})
source_group("client\\Source Files" FILES ${CLIENT_SOURCES})
source_group("client\\Header Files" FILES ${CLIENT_HEADERS})
source_group("libsdl\\Header Files" FILES ${LIBSDL_HEADERS})
source_group("mumble" FILES ${CLIENT_MUMBLE_SOURCES})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})
source_group("server\\Source Files" FILES ${SERVER_SOURCES})
source_group("server\\Header Files" FILES ${SERVER_HEADERS})
source_group("sys\\Source Files" FILES ${SYS_SOURCES})
source_group("sys\\Header Files" FILES ${SYS_HEADERS})
source_group("sys" FILES ${SYS_RC_SOURCES})
source_group("sdl\\Source Files" FILES ${SDL_SOURCES})
source_group("sdl\\Header Files" FILES ${SDL_HEADERS})

add_executable(${CLIENT_TARGET} ${CLIENT_C_SOURCES} ${CLIENT_C_HEADERS} ${SYS_RC_SOURCES})

target_compile_definitions(${CLIENT_TARGET} PRIVATE ${CLIENT_DEFINITIONS})

target_include_directories(${CLIENT_TARGET} PRIVATE ${CLIENT_INCLUDE_DIRS})

compat_target_compile_options(${CLIENT_TARGET} PRIVATE ${CLIENT_CFLAGS})

target_link_libraries(${CLIENT_TARGET} ${CLIENT_LIBRARIES})

compat_target_link_options(${CLIENT_TARGET} PRIVATE ${CLIENT_LDFLAGS})

set_target_properties(${CLIENT_TARGET} PROPERTIES RUNTIME_OUTPUT_NAME "${CLIENT_BIN}${BIN_EXT}")

set(RENDERERGL1_SUBDIR "${CMAKELIST_DIR}/renderergl1")
if(NOT EXISTS "${RENDERERGL1_SUBDIR}/CMakeLists.txt")
	message(WARNING "Could not find renderergl1 in: ${RENDERERGL1_SUBDIR}")
endif()
if(NOT USE_RENDERER_DLOPEN)
	add_subdirectory("${RENDERERGL1_SUBDIR}" "${CMAKE_CURRENT_BINARY_DIR}/renderergl1")
endif()

STRIP_TARGET(${CLIENT_TARGET})

set(DESTINATION_DIR "${REL_INSTALL_DEST}")
if(INSTALL_DIR)
	set(DESTINATION_DIR "${INSTALL_DIR}")
endif(INSTALL_DIR)

install(TARGETS ${CLIENT_TARGET}
	RUNTIME
	DESTINATION "${DESTINATION_DIR}"
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
		GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
	CONFIGURATIONS Release
	COMPONENT client
)
include(CPack)
