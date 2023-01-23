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

project("internal opusfile")

set(OPUSFILE_TARGET "internal_opusfile")

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

set(OPUSFILE_SOURCES
	"${OPUSFILE_DIR}/src/http.c"
	"${OPUSFILE_DIR}/src/info.c"
	"${OPUSFILE_DIR}/src/internal.c"
	"${OPUSFILE_DIR}/src/opusfile.c"
	"${OPUSFILE_DIR}/src/stream.c"
	"${OPUSFILE_DIR}/src/wincerts.c"
)
set(OPUSFILE_HEADERS
	"${OPUSFILE_DIR}/include/opusfile.h"
)

if(MSVC)
	#disable deprecation warnings about old functions like strcmp
	list(APPEND OPUSFILE_DEFINITIONS "-D_CRT_SECURE_NO_WARNINGS")

	#Optimization, Maximum Speed /O2
	list(APPEND OPUSFILE_CFLAGS "$<$<CONFIG:Release>:/O2>")
	#Inline Function Expansion, only Suitable /Ob2
	list(APPEND OPUSFILE_CFLAGS "$<$<CONFIG:Release>:/Ob2>")
	#Enable Intrinsic Functions /Oi
	list(APPEND OPUSFILE_CFLAGS "$<$<CONFIG:Release>:/Oi>")
	#Favor Size of Speed, Favor Fast Code /Ot
	list(APPEND OPUSFILE_CFLAGS "$<$<CONFIG:Release>:/Ot>")
	#Omit Frame Pointers, Yes /Oy
	list(APPEND OPUSFILE_CFLAGS "$<$<CONFIG:Release>:/Oy>")

	#Enable String Pooling /GF
	list(APPEND OPUSFILE_CFLAGS "/GF")
	#Buffer Security Check, No /GS-
	list(APPEND OPUSFILE_CFLAGS "/GS-")
	#Enable Function-Level Linking /Gy
	list(APPEND OPUSFILE_CFLAGS "/Gy")
	#Floating Point Model /fp:fast
	list(APPEND OPUSFILE_CFLAGS "/fp:fast")

endif(MSVC)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")

	set(OFLAGS "-O4" "-O3" "-O2" "-O")
	set(OFLAGS_SUPPORTED)
	ADD_FIRST_SUPPORTED_CFLAG("${OFLAGS}" OFLAGS_SUPPORTED)
	list(APPEND OPUSFILE_CFLAGS $<$<CONFIG:Release>:${OFLAGS_SUPPORTED}>)

	set(FLAGS "-std=c89" "-pedantic" "-Wall" "-Wextra" "-Wno-parentheses" "-Wno-long-long")

	if(MINGW)
		list(APPEND FLAGS "-U__STRICT_ANSI__")
	endif(MINGW)

	ADD_SUPPORTED_CFLAGS(FLAGS OPUSFILE_CFLAGS)

	set(DFLAGS "-O0" "-ggdb")
	set(DFLAGS_SUPPORTED)
	ADD_SUPPORTED_CFLAGS(DFLAGS DFLAGS_SUPPORTED)
	list(APPEND OPUSFILE_CFLAGS $<$<CONFIG:Debug>:${DFLAGS_SUPPORTED}>)

endif()

ADD_INTERNAL_DEFINITIONS(OPUSFILE_DEFINITIONS)
ADD_INTERNAL_CFLAGS(OPUSFILE_CFLAGS)

list(APPEND OPUSFILE_DEFINITIONS $<$<CONFIG:Debug>:-DOP_ENABLE_ASSERTIONS>)

list(APPEND OPUSFILE_INCLUDE_DIRS "${OGG_DIR}/include")
list(APPEND OPUSFILE_INCLUDE_DIRS "${OPUS_DIR}/include")
list(APPEND OPUSFILE_INCLUDE_DIRS "${OPUSFILE_DIR}/include")

#source_group("Source Files" FILES ${OPUSFILE_SOURCES})
#source_group("Header Files" FILES ${OPUSFILE_HEADERS})

add_library(${OPUSFILE_TARGET} STATIC ${OPUSFILE_SOURCES} ${OPUSFILE_HEADERS})

target_compile_definitions(${OPUSFILE_TARGET} PRIVATE ${OPUSFILE_DEFINITIONS})

target_include_directories(${OPUSFILE_TARGET} PRIVATE ${OPUSFILE_INCLUDE_DIRS})

compat_target_compile_options(${OPUSFILE_TARGET} PRIVATE ${OPUSFILE_CFLAGS})

list(APPEND OPUSFILE_LIBRARIES ${OPUSFILE_TARGET})

set(INTERNAL_OPUSFILE_DEFINITIONS "" PARENT_SCOPE)
set(INTERNAL_OPUSFILE_INCLUDE_DIRS "${OPUSFILE_INCLUDE_DIRS}" PARENT_SCOPE)
set(INTERNAL_OPUSFILE_LIBRARIES "${OPUSFILE_LIBRARIES}" PARENT_SCOPE)
