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

project("q3rcc")

set(Q3RCC_TARGET "q3rcc" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

set(Q3RCC_DIR "${SOURCE_DIR}/code/tools/lcc/src")

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(DAG_FILE "${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c")

set(Q3RCC_SOURCES
	"${Q3RCC_DIR}/alloc.c"
	"${Q3RCC_DIR}/bind.c"
	"${Q3RCC_DIR}/bytecode.c"
	"${Q3RCC_DIR}/dag.c"
#	"${Q3RCC_DIR}/dagcheck.c"
	"${DAG_FILE}"
	"${Q3RCC_DIR}/decl.c"
	"${Q3RCC_DIR}/enode.c"
	"${Q3RCC_DIR}/error.c"
	"${Q3RCC_DIR}/event.c"
	"${Q3RCC_DIR}/expr.c"
	"${Q3RCC_DIR}/gen.c"
	"${Q3RCC_DIR}/init.c"
	"${Q3RCC_DIR}/inits.c"
	"${Q3RCC_DIR}/input.c"
	"${Q3RCC_DIR}/lex.c"
	"${Q3RCC_DIR}/list.c"
	"${Q3RCC_DIR}/main.c"
	"${Q3RCC_DIR}/null.c"
	"${Q3RCC_DIR}/output.c"
	"${Q3RCC_DIR}/prof.c"
	"${Q3RCC_DIR}/profio.c"
	"${Q3RCC_DIR}/simp.c"
	"${Q3RCC_DIR}/stmt.c"
	"${Q3RCC_DIR}/string.c"
	"${Q3RCC_DIR}/sym.c"
	"${Q3RCC_DIR}/symbolic.c"
	"${Q3RCC_DIR}/trace.c"
	"${Q3RCC_DIR}/tree.c"
	"${Q3RCC_DIR}/types.c"

)
set(Q3RCC_HEADERS
	"${Q3RCC_DIR}/config.h"
	"${Q3RCC_DIR}/token.h"
	"${Q3RCC_DIR}/c.h"
)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set_source_files_properties("${DAG_FILE}" PROPERTIES COMPILE_FLAGS "-Wno-unused")

endif()

ADD_TOOLS_DEFINITIONS(Q3RCC_DEFINITIONS)
ADD_TOOLS_CFLAGS(Q3RCC_CFLAGS)

#dagcheck.c requires c.h
list(APPEND Q3RCC_INCLUDE_DIRS "${Q3RCC_DIR}")

source_group("Source Files" FILES ${Q3RCC_SOURCES})
source_group("Header Files" FILES ${Q3RCC_HEADERS})

add_executable(${Q3RCC_TARGET} ${Q3RCC_SOURCES} ${Q3RCC_HEADERS})

target_compile_definitions(${Q3RCC_TARGET} PRIVATE ${Q3RCC_DEFINITIONS})

target_include_directories(${Q3RCC_TARGET} PRIVATE ${Q3RCC_INCLUDE_DIRS})

compat_target_compile_options(${Q3RCC_TARGET} PRIVATE ${Q3RCC_CFLAGS})

target_link_libraries(${Q3RCC_TARGET} ${Q3RCC_LIBRARIES})

compat_target_link_options(${Q3RCC_TARGET} PRIVATE ${Q3RCC_LDFLAGS})

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c"
	COMMAND "$<TARGET_FILE:${LBURG_TARGET}>"
		ARGS "${Q3RCC_DIR}/dagcheck.md" "${DAG_FILE}"
	DEPENDS "${Q3RCC_DIR}/dagcheck.md" "${LBURG_TARGET}"
)
