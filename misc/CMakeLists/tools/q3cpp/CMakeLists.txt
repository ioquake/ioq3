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

project("q3cpp")

set(Q3CPP_TARGET "q3cpp" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

set(Q3CPP_DIR "${SOURCE_DIR}/code/tools/lcc/cpp")

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(Q3CPP_SOURCES
	"${Q3CPP_DIR}/cpp.c"
	"${Q3CPP_DIR}/lex.c"
	"${Q3CPP_DIR}/nlist.c"
	"${Q3CPP_DIR}/tokens.c"
	"${Q3CPP_DIR}/macro.c"
	"${Q3CPP_DIR}/eval.c"
	"${Q3CPP_DIR}/include.c"
	"${Q3CPP_DIR}/hideset.c"
	"${Q3CPP_DIR}/getopt.c"
	"${Q3CPP_DIR}/unix.c"
)
set(Q3CPP_HEADERS
	"${Q3CPP_DIR}/cpp.h"
)

if(PLATFORM STREQUAL "darwin" AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	#required to use the mac memmove function
	list(APPEND Q3CPP_DEFINITIONS "-DMACOS_X")

endif()

ADD_TOOLS_DEFINITIONS(Q3CPP_DEFINITIONS)
ADD_TOOLS_CFLAGS(Q3CPP_CFLAGS)

#source_group("Source Files" FILES ${Q3CPP_SOURCES})
#source_group("Header Files" FILES ${Q3CPP_HEADERS})

add_executable(${Q3CPP_TARGET} ${Q3CPP_SOURCES} ${Q3CPP_HEADERS})

target_compile_definitions(${Q3CPP_TARGET} PRIVATE ${Q3CPP_DEFINITIONS})

target_include_directories(${Q3CPP_TARGET} PRIVATE ${Q3CPP_INCLUDE_DIRS})

compat_target_compile_options(${Q3CPP_TARGET} PRIVATE ${Q3CPP_CFLAGS})

target_link_libraries(${Q3CPP_TARGET} ${Q3CPP_LIBRARIES})

compat_target_link_options(${Q3CPP_TARGET} PRIVATE ${Q3CPP_LDFLAGS})
