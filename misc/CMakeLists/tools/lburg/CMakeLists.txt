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

project("lburg")

set(LBURG_TARGET "lburg" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

set(LBURG_DIR "${SOURCE_DIR}/code/tools/lcc/lburg")

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(LBURG_SOURCES
	"${LBURG_DIR}/lburg.c"
	"${LBURG_DIR}/gram.c"
)
set(LBURG_HEADERS
	"${LBURG_DIR}/lburg.h"
)

list(APPEND ${LBURG_DEFINITIONS} "$<$<CONFIG:Debug>:-DYYDEBUG=1>")
list(APPEND ${LBURG_DEFINITIONS} "$<$<CONFIG:Debug>:-DYYERROR_VERBOSE=1>")

ADD_TOOLS_DEFINITIONS(LBURG_DEFINITIONS)
ADD_TOOLS_CFLAGS(LBURG_CFLAGS)

#source_group("Source Files" FILES ${LBURG_SOURCES})
#source_group("Header Files" FILES ${LBURG_HEADERS})

add_executable(${LBURG_TARGET} ${LBURG_SOURCES} ${LBURG_HEADERS})

target_compile_definitions(${LBURG_TARGET} PRIVATE ${LBURG_DEFINITIONS})

target_include_directories(${LBURG_TARGET} PRIVATE ${LBURG_INCLUDE_DIRS})

compat_target_compile_options(${LBURG_TARGET} PRIVATE ${LBURG_CFLAGS})

target_link_libraries(${LBURG_TARGET} ${LBURG_LIBRARIES})

compat_target_link_options(${LBURG_TARGET} PRIVATE ${LBURG_LDFLAGS})
