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

project("q3lcc")

set(Q3LCC_TARGET "q3lcc" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

set(Q3LCCETC_DIR "${SOURCE_DIR}/code/tools/lcc/etc")

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(DEFAULT_TEMP_DIR "$ENV{TMP}")
if(DEFAULT_TEMP_DIR STREQUAL "")
	set(DEFAULT_TEMP_DIR "$ENV{TEMP}")
	if(DEFAULT_TEMP_DIR STREQUAL "")
		set(DEFAULT_TEMP_DIR "$ENV{TMPDIR}")
	endif()
endif()
set(TEMP_DIR "${DEFAULT_TEMP_DIR}" CACHE PATH "Temporary directory")

set(Q3LCC_SOURCES
	"${Q3LCCETC_DIR}/lcc.c"
	"${Q3LCCETC_DIR}/bytecode.c"
)
set(Q3LCC_HEADERS

)

if(TEMP_DIR)
	string(REPLACE "\\" "\\\\" TEMP_DIR_C_STRING "${TEMP_DIR}")
	list(APPEND Q3LCC_DEFINITIONS "-DTEMPDIR=\"${TEMP_DIR_C_STRING}\"")
endif()

ADD_TOOLS_DEFINITIONS(Q3LCC_DEFINITIONS)
ADD_TOOLS_CFLAGS(Q3LCC_CFLAGS)

#source_group("Source Files" FILES ${Q3LCC_SOURCES})
#source_group("Header Files" FILES ${Q3LCC_HEADERS})

add_executable(${Q3LCC_TARGET} ${Q3LCC_SOURCES} ${Q3LCC_HEADERS})

target_compile_definitions(${Q3LCC_TARGET} PRIVATE ${Q3LCC_DEFINITIONS})

target_include_directories(${Q3LCC_TARGET} PRIVATE ${Q3LCC_INCLUDE_DIRS})

compat_target_compile_options(${Q3LCC_TARGET} PRIVATE ${Q3LCC_CFLAGS})

target_link_libraries(${Q3LCC_TARGET} ${Q3LCC_LIBRARIES})

compat_target_link_options(${Q3LCC_TARGET} PRIVATE ${Q3LCC_LDFLAGS})
