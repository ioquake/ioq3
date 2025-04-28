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
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.13.0")
		set(command_help "cmake [<options>] -S <path-to-source> -B <path-to-build>\n")
	else()
		set(command_help "mkdir build; cd build; cmake ..\n")
	endif()
	message(FATAL_ERROR "Building in the source directory gets prevented!\n"
		"Please create a build directory and run cmake there.\n"
		${command_help})
endif()

#set default build type for single configuration generators
get_property(gen_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT gen_multi AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	message("Setting CMAKE_BUILD_TYPE to: ${CMAKE_BUILD_TYPE}")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
		"Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()

project("ioquake3")

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")

include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/git_rev.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")
include("${CMAKELIST_DIR}/include/vm_sources.cmake")


if(DEFINED BUILD_GAME_SO)
	message(DEPRECATION "BUILD_GAME_SO is deprecated, please use BUILD_GAME_VM.")
	set(BUILD_GAME_VM ${BUILD_GAME_SO})
	unset(BUILD_GAME_SO CACHE)
endif()
option(BUILD_AUTOUPDATER "Build Autoupdater" ${DEFAULT_BUILD_AUTOUPDATER})
option(BUILD_BASEGAME "Build basegame (baseq3)" ${DEFAULT_BUILD_BASEGAME})
option(BUILD_CLIENT "Build clientserver" ${DEFAULT_BUILD_CLIENT})
option(BUILD_GAME_QVM "Build quake virtual machine (qvm)" ${DEFAULT_BUILD_GAME_QVM})
option(BUILD_GAME_VM "Build native VM (so/dll)" ${DEFAULT_BUILD_GAME_VM})
option(BUILD_MISSIONPACK "Build missionpack" ${DEFAULT_BUILD_MISSIONPACK})
option(BUILD_RENDERER_OPENGL2 "Build renderer opengl2" ${DEFAULT_BUILD_RENDERER_OPENGL2})
option(BUILD_SERVER "Build dedicated server" ${DEFAULT_BUILD_SERVER})

option(USE_INTERNAL_LIBS "Use internal libraries (Ogg,Vorbis,Opus,zlib,JPEG,...)" ${DEFAULT_USE_INTERNAL_LIBS})
option(USE_INTERNAL_JPEG "Use internal JPEG library" ${USE_INTERNAL_LIBS})
option(USE_INTERNAL_ZLIB "Use internal zlib" ${USE_INTERNAL_LIBS})
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()
set(PRODUCT_VERSION "${DEFAULT_VERSION}" CACHE STRING "Product version")

set(BASEGAME "${DEFAULT_BASEGAME}" CACHE STRING "Directory for the base game")
set(MISSIONPACK "${DEFAULT_MISSIONPACK}" CACHE STRING "Directory for the missionpack")

#relocate build files for easier access
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#.lib .a files
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


if(USE_INTERNAL_ZLIB AND (BUILD_CLIENT OR BUILD_SERVER))
	add_subdirectory("${CMAKELIST_DIR}/internal_zlib")
endif()

if(BUILD_AUTOUPDATER)
	add_subdirectory("${CMAKELIST_DIR}/autoupdater")
endif(BUILD_AUTOUPDATER)

if(BUILD_CLIENT)
	if(USE_INTERNAL_JPEG)
		add_subdirectory("${CMAKELIST_DIR}/internal_jpeg")
	endif()

	add_subdirectory("${CMAKELIST_DIR}/client")

	if(USE_RENDERER_DLOPEN)
		add_subdirectory("${CMAKELIST_DIR}/renderergl1")
	endif(USE_RENDERER_DLOPEN)
	if(BUILD_RENDERER_OPENGL2)
		add_subdirectory("${CMAKELIST_DIR}/renderergl2")
	endif(BUILD_RENDERER_OPENGL2)
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	add_subdirectory("${CMAKELIST_DIR}/server")
endif(BUILD_SERVER)

if(BUILD_GAME_VM)
	if(BUILD_BASEGAME)
		add_subdirectory("${CMAKELIST_DIR}/vm/${BASEGAME}")
	endif(BUILD_BASEGAME)

	if(BUILD_MISSIONPACK)
		add_subdirectory("${CMAKELIST_DIR}/vm/${MISSIONPACK}")
	endif(BUILD_MISSIONPACK)
endif(BUILD_GAME_VM)

GET_VM_SUPPORT(HAVE_VM_COMPILED)
if(BUILD_GAME_QVM AND HAVE_VM_COMPILED)
	add_subdirectory("${CMAKELIST_DIR}/tools")

	if(BUILD_BASEGAME)
		add_subdirectory("${CMAKELIST_DIR}/qvm/${BASEGAME}")
	endif(BUILD_BASEGAME)

	if(BUILD_MISSIONPACK)
		add_subdirectory("${CMAKELIST_DIR}/qvm/${MISSIONPACK}")
	endif(BUILD_MISSIONPACK)
endif()


if(GIT_REV)
	set(git_rev_string "${GIT_REV}")
else()
	set(git_rev_string "<none>")
endif(GIT_REV)
if(gen_multi)
	set(build_type_string "<unused>")
elseif(CMAKE_BUILD_TYPE)
	set(build_type_string "${CMAKE_BUILD_TYPE}")
else()
	set(build_type_string "<none>")
endif()
#CMAKE_C_FLAGS is initialized by the CFLAGS environment variable
if(CMAKE_C_FLAGS)
	set(c_flags_string "${CMAKE_C_FLAGS}")
else()
	set(c_flags_string "<none>")
endif()
if(CMAKE_C_FLAGS_RELEASE)
	set(c_flags_release_string "${CMAKE_C_FLAGS_RELEASE}")
else()
	set(c_flags_release_string "<none>")
endif()
if(MSVC)
	if(BUILD_STATIC_CRT)
		set(static_crt_string "yes (/MT)")
	else()
		set(static_crt_string "no")
	endif()
else()
	set(static_crt_string "<unused>")
endif()
if(DEFINED ENV{SOURCE_DATE_EPOCH})
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.0")
		set(source_date_epoch_string "$ENV{SOURCE_DATE_EPOCH}")
	else()
		set(source_date_epoch_string "<requires cmake 3.8 or later>")
	endif()
else()
	set(source_date_epoch_string "<not set>")
endif()

set(INTERNAL_LIBS "")
if(USE_INTERNAL_CURL)
	list(APPEND INTERNAL_LIBS "cURL")
endif()
if(USE_INTERNAL_JPEG)
	list(APPEND INTERNAL_LIBS "JPEG")
endif()
if(USE_INTERNAL_OGG)
	list(APPEND INTERNAL_LIBS "Ogg")
endif()
if(USE_INTERNAL_OPENAL)
	list(APPEND INTERNAL_LIBS "OpenAL")
endif()
if(USE_INTERNAL_OPUS)
	list(APPEND INTERNAL_LIBS "Opus")
endif()
if(USE_INTERNAL_SDL)
	list(APPEND INTERNAL_LIBS "SDL")
endif()
if(USE_INTERNAL_VORBIS)
	list(APPEND INTERNAL_LIBS "Vorbis")
endif()
if(USE_INTERNAL_ZLIB)
	list(APPEND INTERNAL_LIBS "zlib")
endif()
string(REPLACE ";" ", " internal_libs_string "${INTERNAL_LIBS}")

message("Building in ${CMAKE_BINARY_DIR}:")
message("  Project:              ${CMAKE_PROJECT_NAME}")
message("  Project version:      ${PRODUCT_VERSION}")
message("  Git revision:         ${git_rev_string}")
message("  Platform:             ${CMAKE_HOST_SYSTEM_NAME}")
message("  Platform version:     ${CMAKE_HOST_SYSTEM_VERSION}")
message("  Architecture:         ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message("  Compile platform:     ${COMPILE_PLATFORM}")
message("  Compile architecture: ${COMPILE_ARCH}")
message("  CMake generator:      ${CMAKE_GENERATOR}")
message("  C compiler ID:        ${CMAKE_C_COMPILER_ID}")
message("  C compiler version:   ${CMAKE_C_COMPILER_VERSION}")
message("  SOURCE_DATE_EPOCH:    ${source_date_epoch_string}")
message("  CMAKE_BUILD_TYPE:     ${build_type_string}")
message("  CMAKE_C_FLAGS:        ${c_flags_string}")
message("  CMAKE_C_FLAGS_RELEASE:${c_flags_release_string}")
message("  BUILD_STATIC_CRT:     ${static_crt_string}")
message("  Internal libraries:   ${internal_libs_string}")


# dist target
find_package(Git)

if(NOT GIT_FOUND)
	set(GIT_EXECUTABLE "git")
endif()

if(USE_GIT_REV AND GIT_REV)
	set(DIST_VERSION "${PRODUCT_VERSION}_GIT_${GIT_REV}")
else()
	set(DIST_VERSION "${PRODUCT_VERSION}")
endif()

add_custom_target(dist 
	COMMAND "${GIT_EXECUTABLE}" archive --format zip --output "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${DIST_VERSION}.zip" HEAD
	COMMENT "git archive --format zip --output ${CMAKE_PROJECT_NAME}-${DIST_VERSION}.zip HEAD"
	WORKING_DIRECTORY ${SOURCE_DIR})

include(CPack)

if(NOT hide_build_notice AND CMAKE_GENERATOR STREQUAL "Visual Studio 16 2019")
	message("For a Visual Studio 16 2019 32-bit build use these components:\n"
		" C++ core features\n"
		" Windows Universal C Runtime\n"
		" MSVC v142 - VS 2019 C++ x64/x86 build tools (latest)\n"
		" Python 3 32-bit (3.9.7)\n"		
		" C++ CMake tools for Windows\n"
		" Windows 11 SDK (10.0.22000.0)\n"
		" Windows Universal CRT SDK\n"
		" C++ Windows XP Support for VS 2017 (v141) tools\n"
		" MSVC v142 - VS 2019 C++ x64/x86 build tools (14.29-16.10)\n"
		" Windows 10 SDK (10.0.20348.0)")
	message("MSVC -> Tools -> Command Line -> Developer Command Prompt")
endif()
if(NOT hide_build_notice AND CMAKE_GENERATOR STREQUAL "MSYS Makefiles")
	message("For a MSYS build install these packages:\n"
		" (a CMake version that supports the MSYS generator)\n"
		" 64-bit: mingw-w64-x86_64-toolchain (gcc, make and binutils [strip, ld, ...])\n"
		" 32-bit: mingw-w64-i686-toolchain\n"
		" Optional libraries:\n"
		"  libcurl-devel, libcurl, libjpeg, libogg, openal, opus, opusfile, SDL2, libvorbis, zlib-devel, zlib\n")
endif()
set(hide_build_notice 1 CACHE INTERNAL "Only show build requirements once")
