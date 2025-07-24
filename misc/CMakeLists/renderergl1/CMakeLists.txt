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

project("renderer opengl1")

set(RENDERERGL1_TARGET "renderer_opengl1")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")
include("${CMAKELIST_DIR}/include/strip.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")

include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckIncludeFile)

option(USE_INTERNAL_LIBS "Use internal libraries (Ogg,Vorbis,Opus,zlib,JPEG,...)" ${DEFAULT_USE_INTERNAL_LIBS})
option(USE_INTERNAL_JPEG "Use internal JPEG library" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_SDL "Use internal SDL library" ${USE_INTERNAL_LIBS})
option(USE_LOCAL_HEADERS "Use local headers" ${USE_INTERNAL_LIBS})

option(USE_RENDERER_DLOPEN "Use renderer dynamic library opening" ${DEFAULT_USE_RENDERER_DLOPEN})

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
option(NO_STRIP "No strip (keep all symbol table and relocation information)" ${DEFAULT_NO_STRIP})
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(REL_INSTALL_DEST "${DEFAULT_REL_INSTALL_DEST}" CACHE PATH "Relative installation destination")
set(INSTALL_DIR "${DEFAULT_INSTALL_DIR}" CACHE PATH "Installation directory")

include("${CMAKELIST_DIR}/include/libsdl_headers.cmake")

set(QCOMMON_SOURCES
#	"${QCOMMON_DIR}/cm_load.c"
#	"${QCOMMON_DIR}/cm_patch.c"
#	"${QCOMMON_DIR}/cm_polylib.c"
#	"${QCOMMON_DIR}/cm_test.c"
#	"${QCOMMON_DIR}/cm_trace.c"
#	"${QCOMMON_DIR}/cmd.c"
#	"${QCOMMON_DIR}/common.c"
#	"${QCOMMON_DIR}/cvar.c"
#	"${QCOMMON_DIR}/files.c"
#	"${QCOMMON_DIR}/huffman.c"
#	"${QCOMMON_DIR}/ioapi.c"
#	"${QCOMMON_DIR}/md4.c"
#	"${QCOMMON_DIR}/md5.c"
#	"${QCOMMON_DIR}/msg.c"
#	"${QCOMMON_DIR}/net_chan.c"
#	"${QCOMMON_DIR}/net_ip.c"
	"${QCOMMON_DIR}/puff.c"
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
#	"${QCOMMON_DIR}/unzip.c"
#	"${QCOMMON_DIR}/vm.c"
#	"${QCOMMON_DIR}/vm_interpreted.c"
#	"${QCOMMON_DIR}/vm_none.c"
#	"${QCOMMON_DIR}/vm_powerpc.c"
#	"${QCOMMON_DIR}/vm_powerpc_asm.c"
#	"${QCOMMON_DIR}/vm_sparc.c"
#	"${QCOMMON_DIR}/vm_x86.c"
)
set(QCOMMON_HEADERS
#	"${QCOMMON_DIR}/cm_local.h"
#	"${QCOMMON_DIR}/cm_patch.h"
#	"${QCOMMON_DIR}/cm_polylib.h"
#	"${QCOMMON_DIR}/cm_public.h"
#	"${QCOMMON_DIR}/ioapi.h"
	"${QCOMMON_DIR}/puff.h"
#	"${QCOMMON_DIR}/q_platform.h"
	"${QCOMMON_DIR}/q_shared.h"
	"${QCOMMON_DIR}/qcommon.h"
	"${QCOMMON_DIR}/qfiles.h"
	"${QCOMMON_DIR}/surfaceflags.h"
	"${QCOMMON_DIR}/unzip.h"
	"${QCOMMON_DIR}/vm_local.h"
#	"${QCOMMON_DIR}/vm_powerpc_asm.h"
#	"${QCOMMON_DIR}/vm_sparc.h"
)
list(APPEND RENDERERCOMMON_SOURCES
	"${RENDERERCOMMON_DIR}/tr_font.c"
	"${RENDERERCOMMON_DIR}/tr_image_bmp.c"
	"${RENDERERCOMMON_DIR}/tr_image_jpg.c"
	"${RENDERERCOMMON_DIR}/tr_image_pcx.c"
	"${RENDERERCOMMON_DIR}/tr_image_png.c"
	"${RENDERERCOMMON_DIR}/tr_image_tga.c"
	"${RENDERERCOMMON_DIR}/tr_noise.c"
)
list(APPEND RENDERERCOMMON_HEADERS
	"${RENDERERCOMMON_DIR}/iqm.h"
	"${RENDERERCOMMON_DIR}/qgl.h"
	"${RENDERERCOMMON_DIR}/tr_common.h"
	"${RENDERERCOMMON_DIR}/tr_public.h"
	"${RENDERERCOMMON_DIR}/tr_types.h"
)

list(APPEND RENDERERGL1_SOURCES
	"${RENDERERGL1_DIR}/tr_animation.c"
	"${RENDERERGL1_DIR}/tr_backend.c"
	"${RENDERERGL1_DIR}/tr_bsp.c"
	"${RENDERERGL1_DIR}/tr_cmds.c"
	"${RENDERERGL1_DIR}/tr_curve.c"
	"${RENDERERGL1_DIR}/tr_flares.c"
	"${RENDERERGL1_DIR}/tr_image.c"
	"${RENDERERGL1_DIR}/tr_init.c"
	"${RENDERERGL1_DIR}/tr_light.c"
	"${RENDERERGL1_DIR}/tr_main.c"
	"${RENDERERGL1_DIR}/tr_marks.c"
	"${RENDERERGL1_DIR}/tr_mesh.c"
	"${RENDERERGL1_DIR}/tr_model.c"
	"${RENDERERGL1_DIR}/tr_model_iqm.c"
	"${RENDERERGL1_DIR}/tr_scene.c"
	"${RENDERERGL1_DIR}/tr_shade.c"
	"${RENDERERGL1_DIR}/tr_shade_calc.c"
	"${RENDERERGL1_DIR}/tr_shader.c"
	"${RENDERERGL1_DIR}/tr_shadows.c"
	"${RENDERERGL1_DIR}/tr_sky.c"
	"${RENDERERGL1_DIR}/tr_subs.c"
	"${RENDERERGL1_DIR}/tr_surface.c"
	"${RENDERERGL1_DIR}/tr_world.c"
)
list(APPEND RENDERERGL1_ALTIVEC_SOURCES
	"${RENDERERGL1_DIR}/tr_altivec.c"
)
list(APPEND RENDERERGL1_HEADERS
	"${RENDERERGL1_DIR}/tr_local.h"
)

set(SDL_SOURCES
	"${SDL_DIR}/sdl_gamma.c"
	"${SDL_DIR}/sdl_glimp.c"
#	"${SDL_DIR}/sdl_input.c"
#	"${SDL_DIR}/sdl_snd.c"
)
set(SDL_HEADERS
#	"${SDL_DIR}/sdl_icon.h"
)

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
					message(WARNING "Could not find the file for MinGW: ${FILEPATH}")
				endif()
			endforeach(FILEPATH)
		endif(SDL_FOUND)
	endif()

	#Character Set, CharacterSet="0", Not Set, ASCII/SBCS (Single Byte Character Set)
	list(APPEND RENDERERGL1_DEFINITIONS "-D_SBCS")

	#disable deprecation warnings about old functions like strcmp
	list(APPEND RENDERERGL1_DEFINITIONS "-D_CRT_SECURE_NO_WARNINGS")

	#Randomized Base Address /DYNAMICBASE:NO
	list(APPEND RENDERERGL1_LDFLAGS "/DYNAMICBASE:NO")
	#Data Execution Prevenbtion (DEP) /NXCOMPAT:NO
	list(APPEND RENDERERGL1_LDFLAGS "/NXCOMPAT:NO")
	#Image Has Safe Exception Handlers, is no longer default no in msvc 2013 /SAFESEH:NO
	list(APPEND RENDERERGL1_LDFLAGS "/SAFESEH:NO")

	#requires directory misc/msvc for ../quake3.ico in win_resource.rc
	list(APPEND RENDERERGL1_INCLUDE_DIRS "${SOURCE_DIR}/misc/msvc")

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
					message(WARNING "Could not find the file for MinGW: ${FILEPATH}")
				endif()
			endforeach(FILEPATH)
		endif(SDL_FOUND)
	endif()

endif()

if(PLATFORM STREQUAL "darwin" AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	list(APPEND RENDERERGL1_DEFINITIONS "-DMACOS_X")

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

	find_library(COCOA_LIBRARY Cocoa)
	if(COCOA_LIBRARY)
		list(APPEND RENDERERGL1_LIBRARIES "${COCOA_LIBRARY}")
	else()
		list(APPEND RENDERERGL1_LDFLAGS "-framework Cocoa")
	endif()

endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	if(ARCH STREQUAL "ppc" OR ARCH STREQUAL "ppc64")
		list(APPEND RENDERERGL1_SOURCES ${RENDERERGL1_ALTIVEC_SOURCES})
		foreach(ALTIVEC_SOURCE IN LISTS RENDERERGL1_ALTIVEC_SOURCES)
			set_source_files_properties("${ALTIVEC_SOURCE}" PROPERTIES COMPILE_FLAGS "-maltivec")
		endforeach()
	endif()

#handled by q_shared.h
#	CHECK_C_SOURCE_COMPILES("int __attribute__((visibility(\"default\"))) foo;int main() {return(0);}" HAVE_ATTRIBUTE_VISIBILITY_DEFAULT)
#	if(HAVE_ATTRIBUTE_VISIBILITY_DEFAULT)
#		list(APPEND RENDERERGL1_DEFINITIONS "-DHAVE_ATTRIBUTE_VISIBILITY_DEFAULT")
#	endif(HAVE_ATTRIBUTE_VISIBILITY_DEFAULT)

endif()

ADD_RENDER_DEFINITIONS(RENDERERGL1_DEFINITIONS)
ADD_RENDER_CFLAGS(RENDERERGL1_CFLAGS)
ADD_STRIP_LDFLAG(RENDERERGL1_LDFLAGS)

if(USE_INTERNAL_SDL)
	list(APPEND RENDERERGL1_DEFINITIONS "-DUSE_INTERNAL_SDL")

	if(ARCH STREQUAL "ppc" OR ARCH STREQUAL "ppc64")
		list(APPEND SDL_INCLUDE_DIRS "${LIBSDL_DIR}/include-macppc")
	else()
		list(APPEND SDL_INCLUDE_DIRS "${LIBSDL_DIR}/include")
	endif()
	list(APPEND RENDERERGL1_C_HEADERS ${LIBSDL_HEADERS})

elseif(NOT SDL_FOUND)
	find_package(SDL REQUIRED)
	if(DEFINED SDL_VERSION AND NOT SDL_VERSION STREQUAL "")
		if(${SDL_VERSION} VERSION_LESS 2)
			message(FATAL_ERROR "The SDL library version is ${SDL_VERSION} but required is at least version 2.")
		endif()
	endif()

endif(USE_INTERNAL_SDL)

list(APPEND RENDERERGL1_DEFINITIONS ${SDL_DEFINITIONS})
if(SDL_INCLUDE_DIR)
	list(APPEND RENDERERGL1_INCLUDE_DIRS "${SDL_INCLUDE_DIR}")
endif(SDL_INCLUDE_DIR)
list(APPEND RENDERERGL1_INCLUDE_DIRS ${SDL_INCLUDE_DIRS})
list(APPEND RENDERERGL1_LIBRARIES "${SDL_LIBRARY}")
list(APPEND RENDERERGL1_LIBRARIES ${SDL_LIBRARIES})

if(NOT OPENGL_FOUND)
	find_package(OpenGL REQUIRED)
endif(NOT OPENGL_FOUND)

list(APPEND RENDERERGL1_DEFINITIONS ${OPENGL_DEFINITIONS})
if(OPENGL_INCLUDE_DIR)
	list(APPEND RENDERERGL1_INCLUDE_DIRS "${OPENGL_INCLUDE_DIR}")
endif(OPENGL_INCLUDE_DIR)
list(APPEND RENDERERGL1_INCLUDE_DIRS ${OPENGL_INCLUDE_DIRS})
list(APPEND RENDERERGL1_LIBRARIES "${OPENGL_LIBRARY}")
list(APPEND RENDERERGL1_LIBRARIES ${OPENGL_LIBRARIES})

if(USE_INTERNAL_JPEG)
	list(APPEND RENDERERGL1_DEFINITIONS "-DUSE_INTERNAL_JPEG")
	if(NOT DEFINED INTERNAL_JPEG_LIBRARIES)
		add_subdirectory("${CMAKELIST_DIR}/internal_jpeg" "internal_jpeg")
	endif()
	list(APPEND RENDERERGL1_DEFINITIONS ${INTERNAL_JPEG_DEFINITIONS})
	list(APPEND RENDERERGL1_INCLUDE_DIRS ${INTERNAL_JPEG_INCLUDE_DIRS})
	list(APPEND RENDERERGL1_LIBRARIES ${INTERNAL_JPEG_LIBRARIES})
else()
	#tr_image_jpg.c JPEG_LIB_VERSION >= 80
	find_package(JPEG REQUIRED)
	if(DEFINED JPEG_VERSION AND NOT JPEG_VERSION STREQUAL "")
		if(${JPEG_VERSION} VERSION_LESS 80)
			message(FATAL_ERROR "The JPEG library version is ${JPEG_VERSION} but required is at least version 80.")
		endif()
	endif()
endif(USE_INTERNAL_JPEG)

list(APPEND RENDERERGL1_DEFINITIONS ${JPEG_DEFINITIONS})
if(JPEG_INCLUDE_DIR)
	list(APPEND RENDERERGL1_INCLUDE_DIRS "${JPEG_INCLUDE_DIR}")
endif(JPEG_INCLUDE_DIR)
list(APPEND RENDERERGL1_INCLUDE_DIRS ${JPEG_INCLUDE_DIRS})
list(APPEND RENDERERGL1_LIBRARIES "${JPEG_LIBRARY}")
list(APPEND RENDERERGL1_LIBRARIES ${JPEG_LIBRARIES})

if(USE_RENDERER_DLOPEN)
	list(APPEND RENDERERGL1_DEFINITIONS "-DUSE_RENDERER_DLOPEN")
endif(USE_RENDERER_DLOPEN)

if(USE_LOCAL_HEADERS)
	list(APPEND RENDERERGL1_DEFINITIONS "-DUSE_LOCAL_HEADERS")
endif(USE_LOCAL_HEADERS)

if(DEFINED ENV{SOURCE_DATE_EPOCH})
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.0")
		string(TIMESTAMP PRODUCT_DATE "%b %d %Y" UTC) #Nov 28 2021
		list(APPEND RENDERERGL1_DEFINITIONS "-DPRODUCT_DATE=\"${PRODUCT_DATE}\"")
	else()
		message(WARNING "Update CMake to version 3.8 or later for SOURCE_DATE_EPOCH support!")
	endif()
endif()

list(APPEND RENDERERGL1_C_SOURCES ${RENDERERCOMMON_SOURCES} ${RENDERERGL1_SOURCES} ${SDL_SOURCES} ${QCOMMON_SOURCES})
list(APPEND RENDERERGL1_C_HEADERS ${RENDERERCOMMON_HEADERS} ${RENDERERGL1_HEADERS} ${SDL_HEADERS} ${QCOMMON_HEADERS})

source_group("libsdl\\Header Files" FILES ${LIBSDL_HEADERS})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})
source_group("renderercommon\\Source Files" FILES ${RENDERERCOMMON_SOURCES})
source_group("renderercommon\\Header Files" FILES ${RENDERERCOMMON_HEADERS})
source_group("renderergl1\\Source Files" FILES ${RENDERERGL1_SOURCES})
source_group("renderergl1\\Header Files" FILES ${RENDERERGL1_HEADERS})
source_group("sdl\\Header Files" FILES ${SDL_HEADERS})
source_group("sdl\\Source Files" FILES ${SDL_SOURCES})

if(USE_RENDERER_DLOPEN)
	add_library(${RENDERERGL1_TARGET} SHARED ${RENDERERGL1_C_SOURCES} ${RENDERERGL1_C_HEADERS})
	set_target_properties(${RENDERERGL1_TARGET} PROPERTIES PREFIX "")
	set_target_properties(${RENDERERGL1_TARGET} PROPERTIES OUTPUT_NAME "renderer_opengl1_${ARCH}")
else()
	add_library(${RENDERERGL1_TARGET} STATIC ${RENDERERGL1_C_SOURCES} ${RENDERERGL1_C_HEADERS})
endif(USE_RENDERER_DLOPEN)

target_compile_definitions(${RENDERERGL1_TARGET} PRIVATE ${RENDERERGL1_DEFINITIONS})

target_include_directories(${RENDERERGL1_TARGET} PRIVATE ${RENDERERGL1_INCLUDE_DIRS})

compat_target_compile_options(${RENDERERGL1_TARGET} PRIVATE ${RENDERERGL1_CFLAGS})

target_link_libraries(${RENDERERGL1_TARGET} ${RENDERERGL1_LIBRARIES})

compat_target_link_options(${RENDERERGL1_TARGET} PRIVATE ${RENDERERGL1_LDFLAGS})

STRIP_TARGET(${RENDERERGL1_TARGET})

set(DESTINATION_DIR "${REL_INSTALL_DEST}")
if(INSTALL_DIR)
	set(DESTINATION_DIR "${INSTALL_DIR}")
endif(INSTALL_DIR)

if(USE_RENDERER_DLOPEN)
	#cmake 2.8.12 requires RUNTIME instead of LIBRARY DESTINATION
	install(TARGETS ${RENDERERGL1_TARGET}
		RUNTIME DESTINATION "${DESTINATION_DIR}"
		LIBRARY DESTINATION "${DESTINATION_DIR}"
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
		CONFIGURATIONS Release
		COMPONENT renderer
	)
endif(USE_RENDERER_DLOPEN)

include(CPack)
