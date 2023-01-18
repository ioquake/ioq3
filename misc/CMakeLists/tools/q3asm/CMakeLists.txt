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

project("q3asm")

set(Q3ASM_TARGET "q3asm" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

set(Q3ASM_DIR "${SOURCE_DIR}/code/tools/asm")

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(Q3ASM_SOURCES
	"${Q3ASM_DIR}/cmdlib.c"
	"${Q3ASM_DIR}/q3asm.c"
)
set(Q3ASM_HEADERS
	"${Q3ASM_DIR}/cmdlib.h"
	"${Q3ASM_DIR}/mathlib.h"
	"${Q3ASM_DIR}/opstrings.h"
)

ADD_TOOLS_DEFINITIONS(Q3ASM_DEFINITIONS)
ADD_TOOLS_CFLAGS(Q3ASM_CFLAGS)

#source_group("Source Files" FILES ${Q3ASM_SOURCES})
#source_group("Header Files" FILES ${Q3ASM_HEADERS})

add_executable(${Q3ASM_TARGET} ${Q3ASM_SOURCES} ${Q3ASM_HEADERS})

target_compile_definitions(${Q3ASM_TARGET} PRIVATE ${Q3ASM_DEFINITIONS})

target_include_directories(${Q3ASM_TARGET} PRIVATE ${Q3ASM_INCLUDE_DIRS})

target_compile_options(${Q3ASM_TARGET} PRIVATE ${Q3ASM_CFLAGS})

target_link_libraries(${Q3ASM_TARGET} ${Q3ASM_LIBRARIES})

compat_target_link_options(${Q3ASM_TARGET} PRIVATE ${Q3ASM_LDFLAGS})
