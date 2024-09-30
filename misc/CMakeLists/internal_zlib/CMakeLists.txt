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

project("internal zlib")

set(ZLIB_TARGET "internal_zlib")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
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

set(ZLIB_SOURCES
	"${ZLIB_DIR}/adler32.c"
	"${ZLIB_DIR}/crc32.c"
	"${ZLIB_DIR}/inffast.c"
	"${ZLIB_DIR}/inflate.c"
	"${ZLIB_DIR}/inftrees.c"
	"${ZLIB_DIR}/zutil.c"
)
set(ZLIB_HEADERS
	"${ZLIB_DIR}/crc32.h"
	"${ZLIB_DIR}/inffast.h"
	"${ZLIB_DIR}/inffixed.h"
	"${ZLIB_DIR}/inflate.h"
	"${ZLIB_DIR}/inftrees.h"
	"${ZLIB_DIR}/zconf.h"
	"${ZLIB_DIR}/zlib.h"
	"${ZLIB_DIR}/zutil.h"
)

if(MSVC)
	#no crt warnings
	list(APPEND ZLIB_DEFINITIONS "-D_CRT_NONSTDC_NO_DEPRECATE")
	list(APPEND ZLIB_DEFINITIONS "-D_CRT_SECURE_NO_DEPRECATE")
	list(APPEND ZLIB_DEFINITIONS "-D_CRT_NONSTDC_NO_WARNINGS")

	#Character Set, CharacterSet="0", Not Set, ASCII/SBCS (Single Byte Character Set)
	list(APPEND ZLIB_DEFINITIONS "-D_SBCS")

	#switch on the WINAPI (STDCALL) convention
	#list(APPEND ZLIB_DEFINITIONS "-DZLIB_WINAPI")
	#unused
	#list(APPEND ZLIB_DEFINITIONS "-DASMV")
	#machine dependent optimization
	#list(APPEND ZLIB_DEFINITIONS "-DASMINF")

	#Inline Function Expansion, only __inline /Ob1
	list(APPEND ZLIB_CFLAGS $<$<CONFIG:Release>:/Ob1>)
	#Omit Frame Pointers, Yes /Oy
	list(APPEND ZLIB_CFLAGS $<$<CONFIG:Release>:/Oy>)

	#Enable String Pooling /GF
	list(APPEND ZLIB_CFLAGS "/GF")
	#Buffer Security Check, No /GS-
	list(APPEND ZLIB_CFLAGS "/GS-")
	#Enable Function-Level Linking /Gy
	list(APPEND ZLIB_CFLAGS "/Gy")

endif(MSVC)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")

	set(OFLAGS "-O4" "-O3" "-O2" "-O")
	set(OFLAGS_SUPPORTED)
	ADD_FIRST_SUPPORTED_CFLAG(OFLAGS OFLAGS_SUPPORTED)
	list(APPEND ZLIB_CFLAGS $<$<CONFIG:Release>:${OFLAGS_SUPPORTED}>)

	set(DFLAGS "-Wall" "-Wextra" "-Wcast-qual" "-pedantic")
	set(DFLAGS_SUPPORTED)
	ADD_SUPPORTED_CFLAGS(DFLAGS DFLAGS_SUPPORTED)
	list(APPEND ZLIB_CFLAGS $<$<CONFIG:Debug>:${DFLAGS_SUPPORTED}>)

endif()

ADD_INTERNAL_DEFINITIONS(ZLIB_DEFINITIONS)
ADD_INTERNAL_CFLAGS(ZLIB_CFLAGS)

#unused
#list(APPEND ZLIB_DEFINITIONS "-DZLIB_CONST")

list(APPEND ZLIB_DEFINITIONS "-DNO_GZIP")
#list(APPEND ZLIB_INCLUDE_DIRS "${ZLIB_DIR}")

source_group("Source Files" FILES ${ZLIB_SOURCES})
source_group("Header Files" FILES ${ZLIB_HEADERS})

add_library(${ZLIB_TARGET} STATIC ${ZLIB_SOURCES} ${ZLIB_HEADERS})

target_compile_definitions(${ZLIB_TARGET} PRIVATE ${ZLIB_DEFINITIONS})

target_include_directories(${ZLIB_TARGET} PRIVATE ${ZLIB_INCLUDE_DIRS})

compat_target_compile_options(${ZLIB_TARGET} PRIVATE ${ZLIB_CFLAGS})

set(INTERNAL_ZLIB_DEFINITIONS "" PARENT_SCOPE)
set(INTERNAL_ZLIB_INCLUDE_DIRS "${ZLIB_DIR}" PARENT_SCOPE)
set(INTERNAL_ZLIB_LIBRARIES "${ZLIB_TARGET}" PARENT_SCOPE)
