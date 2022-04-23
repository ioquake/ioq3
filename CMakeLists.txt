cmake_minimum_required(VERSION 3.11)
project(ioquake3 VERSION 1.36 LANGUAGES C ASM)

include(CheckSymbolExists)
include(CheckCCompilerFlag)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CODE_DIR ${ROOT_DIR}/code)
set(LIBS_DIR ${ROOT_DIR}/libs)

set(ENGINE_BINARY_DIR ${CMAKE_BINARY_DIR})

option(DEFAULT_BASEDIR "extra path to search for baseq3 and such" "")
option(BUILD_CLIENT "build the 'ioquake3' client binary" ON)
option(BUILD_SERVER "build the 'ioq3ded' server binary" ON)
option(BUILD_RENDERER_OPENGL2 "" ON)
option(BUILD_AUTOUPDATER "DON'T build unless you mean to!" OFF)
option(BUILD_STANDALONE "build binaries suited for stand-alone games" OFF)

set(SERVERBIN "ioq3ded" CACHE STRING "server binary")
set(CLIENTBIN "ioquake3" CACHE STRING "client binary")

option(USE_RENDERER_DLOPEN "build and use the renderer in a library" ON)
option(USE_OPENAL_DLOPEN "link with OpenAL at runtime" ON)
option(USE_CURL_DLOPEN "link with libcurl at runtime" ON)
option(USE_VOIP "enable built-in VoIP support" ON)
option(USE_MUMBLE "enable Mumble support" ON)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
    # 4244 conversion from 'float' to 'int', possible loss of data
    # 4305 truncation from 'double' to 'float'
    # 4820 padding
    # 5045 spectre instruction
    # 4668 unknown macro definition
    # 4061 explicit switch case enum mention
    # 4242 possible loss of data (convert int to short)
    # 4464 relative include path
    # 4619 warning id is not available
    # 4245 return signed/unsigned conflict
    # 4100 unreferenced formal parameter
    # 4255 invalid function prototype - missing void
    # 4389 comparison signed/unsigned
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4267 /wd4244 /wd4305 /wd4820 /wd5045 /wd4668 /wd4061 /wd4242 /wd4464 /wd4619 /wd4245 /wd4100 /wd4255 /wd4389")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
    add_compile_definitions(MAC_OS_X_VERSION_MIN_REQUIRED=1070)
endif()

set(ARCH_STRING x86)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_STRING x86_64)
endif()

add_compile_definitions(PRODUCT_VERSION="${CMAKE_PROJECT_VERSION}" ARCH_STRING="${ARCH_STRING}")

if (BUILD_STANDALONE)
    add_compile_definitions(STANDALONE)
endif()

macro(check_compiler_flag flag)
  string(REGEX REPLACE "[-=+]" "_" _flag ${flag})
  string(TOUPPER ${_flag} _flagfinal)
  check_c_compiler_flag("${flag}" COMPILER_SUPPORTS_${_flagfinal})
  if (COMPILER_SUPPORTS_${_flagfinal})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
  endif()
endmacro()

check_compiler_flag(-Wformat=2)
check_compiler_flag(-Wno-format-zero-length)
check_compiler_flag(-Wformat-security)
check_compiler_flag(-Wno-format-nonliteral)
check_compiler_flag(-Wstrict-aliasing=2)
check_compiler_flag(-Wmissing-format-attribute)
check_compiler_flag(-Wdisabled-optimization)
check_compiler_flag(-Werror-implicit-function-declaration)

add_subdirectory(code)
