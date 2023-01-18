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

project("autoupdater")

set(AUTOUPDATER_TARGET "autoupdater")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

set(MOUNT_DIR "${SOURCE_DIR}/code" CACHE PATH "path to the code directory")

set(AUTOUPDATER_SRC_DIR "${MOUNT_DIR}/autoupdater")
set(LIBTOMCRYPT_SRC_DIR "${AUTOUPDATER_SRC_DIR}/rsa_tools/libtomcrypt-1.17")
set(TOMSFASTMATH_SRC_DIR "${AUTOUPDATER_SRC_DIR}/rsa_tools/tomsfastmath-0.13.1")

include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include(CheckCCompilerFlag)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()

set(AUTOUPDATER_SOURCES
	"${AUTOUPDATER_SRC_DIR}/autoupdater.c"

)
set(AUTOUPDATER_HEADERS

)

find_package(TOMCRYPT 1.17 QUIET)
if(TOMCRYPT_FOUND)
	list(APPEND AUTOUPDATER_DEFINITIONS ${TOMCRYPT_DEFINITIONS})
	if(TOMCRYPT_INCLUDE_DIR)
		list(APPEND AUTOUPDATER_INCLUDE_DIRS "${TOMCRYPT_INCLUDE_DIR}")
	endif(TOMCRYPT_INCLUDE_DIR)
	list(APPEND AUTOUPDATER_INCLUDE_DIRS ${TOMCRYPT_INCLUDE_DIRS})
	list(APPEND AUTOUPDATER_LIBRARIES "${TOMCRYPT_LIBRARY}")
	list(APPEND AUTOUPDATER_LIBRARIES ${TOMCRYPT_LIBRARIES})
else(TOMCRYPT_FOUND)
	list(APPEND AUTOUPDATER_LIBRARIES "tomcrypt")
	link_directories("${LIBTOMCRYPT_SRC_DIR}")
	list(APPEND AUTOUPDATER_INCLUDE_DIRS "${LIBTOMCRYPT_SRC_DIR}/src/headers")
endif(TOMCRYPT_FOUND)

find_package(TOMSFASTMATH 0.13.1 QUIET)
if(TOMSFASTMATH_FOUND)
	list(APPEND AUTOUPDATER_DEFINITIONS ${TOMSFASTMATH_DEFINITIONS})
	if(TOMSFASTMATH_INCLUDE_DIR)
		list(APPEND AUTOUPDATER_INCLUDE_DIRS "${TOMSFASTMATH_INCLUDE_DIR}")
	endif(TOMSFASTMATH_INCLUDE_DIR)
	list(APPEND AUTOUPDATER_INCLUDE_DIRS ${TOMSFASTMATH_INCLUDE_DIRS})
	list(APPEND AUTOUPDATER_LIBRARIES "${TOMSFASTMATH_LIBRARY}")
	list(APPEND AUTOUPDATER_LIBRARIES ${TOMSFASTMATH_LIBRARIES})
else(TOMSFASTMATH_FOUND)
	list(APPEND AUTOUPDATER_LIBRARIES "tfm")
	link_directories("${TOMSFASTMATH_SRC_DIR}")
	list(APPEND AUTOUPDATER_INCLUDE_DIRS "${TOMSFASTMATH_SRC_DIR}/src/headers")
endif(TOMSFASTMATH_FOUND)

if(MINGW AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	#InternetOpen, HttpQueryInfo, InternetOpenUrl,
	#InternetCloseHandle, InternetReadFile functions 
	list(APPEND AUTOUPDATER_LIBRARIES "wininet")

endif()

if(MSVC)

	#disable deprecation warnings about old functions like strcmp
	list(APPEND AUTOUPDATER_DEFINITIONS "-D_CRT_SECURE_NO_WARNINGS")

	if(BUILD_STATIC_CRT)
		list(APPEND AUTOUPDATER_CFLAGS "/MT")
	endif()

endif(MSVC)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")

#	list(APPEND AUTOUPDATER_CFLAGS_OPTIMIZE "-g")
#	list(APPEND AUTOUPDATER_CFLAGS_OPTIMIZE "-Wall")
#	list(APPEND AUTOUPDATER_CFLAGS_OPTIMIZE "-fno-strict-aliasing")

	CHECK_C_COMPILER_FLAG("-MMD" COMPILER_SUPPORTS_-MMD)
	if(GENERATE_DEPENDENCIES AND COMPILER_SUPPORTS_-MMD)
		list(APPEND AUTOUPDATER_CFLAGS "-MMD")
	endif()

endif()

#source_group("Source Files" FILES ${AUTOUPDATER_SOURCES})
#source_group("Header Files" FILES ${AUTOUPDATER_HEADERS})

add_executable(${AUTOUPDATER_TARGET} ${AUTOUPDATER_SOURCES} ${AUTOUPDATER_HEADERS})

target_compile_definitions(${AUTOUPDATER_TARGET} PRIVATE ${AUTOUPDATER_DEFINITIONS})

target_include_directories(${AUTOUPDATER_TARGET} PRIVATE ${AUTOUPDATER_INCLUDE_DIRS})

compat_target_compile_options(${AUTOUPDATER_TARGET} PRIVATE ${AUTOUPDATER_CFLAGS})

target_link_libraries(${AUTOUPDATER_TARGET} ${AUTOUPDATER_LIBRARIES})

compat_target_link_options(${AUTOUPDATER_TARGET} PRIVATE ${AUTOUPDATER_LDFLAGS})
