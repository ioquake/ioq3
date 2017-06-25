#  oo          MM'"""""`MMM                   dP                d8888b.
#              M  .mmm,  MM                   88                    `88
#  dP .d8888b. M  MMMMM  MM dP    dP .d8888b. 88  .dP  .d8888b.  aaad8'
#  88 88'  `88 M  MM  M  MM 88    88 88'  `88 88888"   88ooood8     `88
#  88 88.  .88 M  `MM    MM 88.  .88 88.  .88 88  `8b. 88.  ...     .88
#  dP `88888P' MM.    .. `M `88888P' `88888P8 dP   `YP `88888P' d88888P
#              MMMMMMMMMMMM

cmake_minimum_required(VERSION 3.5)
project(ioQuake3 C)

set(CMAKE_CXX_FLAGS -std=c11)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

option(USE_RENDERER_DLOPEN "" ON)

if (APPLE OR WIN32)
  option(USE_INTERNAL_SDL2 "" ON)
else ()
  option(USE_INTERNAL_SDL2 "" OFF)
endif()

option(USE_INTERNAL_JPEG "" ON)
option(USE_OPENAL "" ON)
option(USE_OPENAL_DLOPEN "" ON)
option(USE_VOIP "" OFF)
option(USE_MUMBLE "" ON)
option(USE_CODEC_OPUS "" ON)
option(BUILD_MISSIONPACK "" ON) # FIXME Unused
option(USE_AUTOUPDATER "" ON)

include(${CMAKE_SOURCE_DIR}/cmake/build_dir)
include(${CMAKE_SOURCE_DIR}/cmake/debug_cflags)

# builtins 
add_subdirectory(code/zlib)
add_subdirectory(code/jpeg-8c)
add_subdirectory(code/opus-1.1.4)
add_subdirectory(code/opusfile-0.8)
add_subdirectory(code/libogg-1.3.2)
add_subdirectory(code/libvorbis-1.3.5)

# engine
add_subdirectory(code/renderercommon)
add_subdirectory(code/renderergl1)
add_subdirectory(code/renderergl2)
add_subdirectory(code/botlib)

#         .                    .
#   ___   |   `   ___  , __   _/_
# .'   `  |   | .'   ` |'  `.  |
# |       |   | |----' |    |  |
#  `._.' /\__ / `.___, /    |  \__/
#

add_subdirectory(code/client)

#   ____   ___  .___  _   __   ___  .___
#  (     .'   ` /   \ |   /  .'   ` /   \
#  `--.  |----' |   ' `  /   |----' |   '
# \___.' `.___, /      \/    `.___, /
#
 
add_subdirectory(code/server)

#  _______ ___ ___ _______                              __ __
# |       |   |   |   |   | .----.-----.--------.-----.|__|  |.-----.----.
# |   -  _|   |   |       | |  __|  _  |        |  _  ||  |  ||  -__|   _|
# |_______|\_____/|__|_|__| |____|_____|__|__|__|   __||__|__||_____|__|
#                                               |__|
# TODO: DO NOT build unless building a QVM

add_subdirectory(code/tools)

#
# ,   . `
# |   | |
# |   | |
# `._/| /
#

add_subdirectory(code/ui)

#   ___.   ___  , _ , _     ___
# .'   `  /   ` |' `|' `. .'   `
# |    | |    | |   |   | |----'
#  `---| `.__/| /   '   / `.___,
#  \___/
#

add_subdirectory(code/game)

#   ___    ___.   ___  , _ , _     ___
# .'   ` .'   `  /   ` |' `|' `. .'   `
# |      |    | |    | |   |   | |----'
#  `._.'  `---| `.__/| /   '   / `.___,
#         \___/
#

add_subdirectory(code/cgame)

# Author: Victor Roemer  wtfbbqhax, <victor@badsec.org>. 
#  Date: Wed Feb  8, 2017 

