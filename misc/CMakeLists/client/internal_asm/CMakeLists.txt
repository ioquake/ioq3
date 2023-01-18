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

project("internal asm")

set(ASM_TARGET "internal_client_asm")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")

set(ASM_SOURCES
	"${ASM_DIR}/matha.s"
	"${ASM_DIR}/snd_mixa.s"
)
set(ASM_HEADERS
	"${ASM_DIR}/qasm.h"
	"${ASM_DIR}/qasm-inline.h"
)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	list(APPEND ASM_CFLAGS "-x assembler-with-cpp")
endif()

source_group("asm" FILES ${ASM_SOURCES} ${ASM_HEADERS})
#source_group("asm\\Source Files" FILES ${ASM_SOURCES})
#source_group("asm\\Header Files" FILES ${ASM_HEADERS})

enable_language(ASM)

add_library(${ASM_TARGET} STATIC ${ASM_SOURCES} ${ASM_HEADERS})

target_compile_definitions(${ASM_TARGET} PRIVATE ${ASM_DEFINITIONS})

compat_target_compile_options(${ASM_TARGET} PRIVATE ${ASM_CFLAGS})

set_target_properties(${ASM_TARGET} PROPERTIES LINKER_LANGUAGE C)

set(INTERNAL_ASM_INCLUDE_DIRS "" PARENT_SCOPE)
set(INTERNAL_ASM_DEFINITIONS "" PARENT_SCOPE)
set(INTERNAL_ASM_LIBRARIES "${ASM_TARGET}" PARENT_SCOPE)
