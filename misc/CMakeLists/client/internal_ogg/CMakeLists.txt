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

project("internal ogg")

set(OGG_TARGET "internal_ogg")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(OGG_SOURCES
	"${OGG_DIR}/src/bitwise.c"
	"${OGG_DIR}/src/framing.c"
)
set(OGG_HEADERS
	"${OGG_DIR}/include/ogg/config_types.h"
	"${OGG_DIR}/include/ogg/ogg.h"
	"${OGG_DIR}/include/ogg/os_types.h"
)


if(MSVC)
	#Optimization, Maximum Speed /O2
	list(APPEND OGG_CFLAGS $<$<CONFIG:Release>:/O2>)
	#Inline Function Expansion, only Suitable /Ob2
	list(APPEND OGG_CFLAGS $<$<CONFIG:Release>:/Ob2>)
	#Enable Intrinsic Functions /Oi
	list(APPEND OGG_CFLAGS $<$<CONFIG:Release>:/Oi>)
	#Favor Size of Speed, Favor Fast Code /Ot
	list(APPEND OGG_CFLAGS $<$<CONFIG:Release>:/Ot>)

	#Enable String Pooling /GF
	list(APPEND OGG_CFLAGS "/GF")
	#Buffer Security Check, No /GS-
	list(APPEND OGG_CFLAGS "/GS-")

	#Calling Convention, __cdecl /Gd
	list(APPEND OGG_CFLAGS "/Gd")	
	#Compile as, C Code /TC
	list(APPEND OGG_CFLAGS "/TC")
	#Disable specific warnings
	list(APPEND OGG_CFLAGS "/wd4244")

endif(MSVC)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")

	if(PLATFORM MATCHES "linux")
		list(APPEND OGG_RELEASE_CFLAGS "-Wall" "-fsigned-char" "-O20" "-ffast-math")
		list(APPEND OGG_DEBUG_CFLAGS "-Wall" "-fsigned-char" "-g")
	elseif(PLATFORM MATCHES "darwin")
		list(APPEND OGG_RELEASE_CFLAGS "-Wall" "-fno-common" "-fsigned-char" "-ffast-math" "-O4")
		list(APPEND OGG_DEBUG_CFLAGS "-Wall" "-fno-common" "-fsigned-char" "-g")
	elseif(ARCH STREQUAL "sparc")
		list(APPEND OGG_RELEASE_CFLAGS "-fsigned-char" "-O20")
		list(APPEND OGG_DEBUG_CFLAGS "-fsigned-char" "-g" "-Wall")
	else()
		list(APPEND OGG_RELEASE_CFLAGS "-fsigned-char" "-O20")
		list(APPEND OGG_DEBUG_CFLAGS "-fsigned-char" "-g" "-Wall")
	endif()

	list(APPEND VORBIS_DEFINITIONS $<$<CONFIG:Release>:${OGG_RELEASE_DEFINITIONS}>)
	list(APPEND OGG_CFLAGS $<$<CONFIG:Release>:${OGG_RELEASE_CFLAGS}>)
	list(APPEND VORBIS_DEFINITIONS $<$<CONFIG:Debug>:${OGG_DEBUG_DEFINITIONS}>)
	list(APPEND OGG_CFLAGS $<$<CONFIG:Debug>:${OGG_DEBUG_CFLAGS}>)

endif()

ADD_INTERNAL_DEFINITIONS(OGG_DEFINITIONS)
ADD_INTERNAL_CFLAGS(OGG_CFLAGS)

list(APPEND OGG_INCLUDE_DIRS "${OGG_DIR}/include")

#source_group("Source Files" FILES ${OGG_SOURCES} ${OGG_HEADERS})
#source_group("Source Files" FILES ${OGG_SOURCES})
#source_group("Header Files" FILES ${OGG_HEADERS})

add_library(${OGG_TARGET} STATIC ${OGG_SOURCES} ${OGG_HEADERS})

target_compile_definitions(${OGG_TARGET} PRIVATE ${OGG_DEFINITIONS})

target_include_directories(${OGG_TARGET} PRIVATE ${OGG_INCLUDE_DIRS})

compat_target_compile_options(${OGG_TARGET} PRIVATE ${OGG_CFLAGS})

set(INTERNAL_OGG_DEFINITIONS "" PARENT_SCOPE)
set(INTERNAL_OGG_INCLUDE_DIRS "${OGG_DIR}/include" PARENT_SCOPE)
set(INTERNAL_OGG_LIBRARIES "${OGG_TARGET}" PARENT_SCOPE)
