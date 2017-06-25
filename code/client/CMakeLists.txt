#
##  .o88b. db      d888888b d88888b d8b   db d888888b
## d8P  Y8 88        `88'   88'     888o  88 `~~88~~'
## 8P      88         88    88ooooo 88V8o 88    88
## 8b      88         88    88~~~~~ 88 V8o88    88
## Y8b  d8 88booo.   .88.   88.     88  V888    88
##  `Y88P' Y88888P Y888888P Y88888P VP   V8P    YP
#

find_package(CURL)
find_package(OpenGL)
find_package(OpenAL)
include(${CMAKE_SOURCE_DIR}/cmake/SDL2.cmake)

add_definitions(
    -DUSE_LOCAL_HEADERS
    -DUSE_RESTCLIENT
    -DUSE_VOIP
    -DUSE_MUMBLE
    -DUSE_CODEC_OPUS
    -DUSE_OPENAL
    -DUSE_OPENAL_DLOPEN
    -DUSE_INTERNAL_JPEG
    -DUSE_AUTOUPDATER
    -DUSE_RENDERER_DLOPEN
    -DNDEBUG
    -DPRODUCT_VERSION="1.2.0 pre-release"
    ${SDL2_DEFINES}
    )

set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(APPLE)
set(APPLE_SOURCES ${PARENT_DIR}/sys/sys_osx.m)
endif(APPLE)

add_executable(
    ioquake3
    #
    ${PARENT_DIR}/asm/ftola.c
    ${PARENT_DIR}/asm/snapvector.c
    #
    cl_avi.c
    cl_cgame.c
    cl_cin.c
    cl_console.c
    cl_curl.c
    cl_input.c
    cl_keys.c
    cl_main.c
    cl_net_chan.c
    cl_parse.c
    cl_scrn.c
    cl_ui.c
    libmumblelink.c
    qal.c
    snd_adpcm.c
    snd_codec.c
    snd_codec_ogg.c
    snd_codec_opus.c
    snd_codec_wav.c
    snd_dma.c
    snd_main.c
    snd_mem.c
    snd_mix.c
    snd_openal.c
    snd_wavelet.c
    #
    ${PARENT_DIR}/qcommon/cm_load.c
    ${PARENT_DIR}/qcommon/cm_patch.c
    ${PARENT_DIR}/qcommon/cm_polylib.c
    ${PARENT_DIR}/qcommon/cm_test.c
    ${PARENT_DIR}/qcommon/cm_trace.c
    ${PARENT_DIR}/qcommon/cmd.c
    ${PARENT_DIR}/qcommon/common.c
    ${PARENT_DIR}/qcommon/cvar.c
    ${PARENT_DIR}/qcommon/files.c
    ${PARENT_DIR}/qcommon/huffman.c
    ${PARENT_DIR}/qcommon/ioapi.c
    ${PARENT_DIR}/qcommon/md4.c
    ${PARENT_DIR}/qcommon/md5.c
    ${PARENT_DIR}/qcommon/msg.c
    ${PARENT_DIR}/qcommon/net_chan.c
    ${PARENT_DIR}/qcommon/net_ip.c
    ${PARENT_DIR}/qcommon/puff.c
    ${PARENT_DIR}/qcommon/q_shared.c
    ${PARENT_DIR}/qcommon/q_math.c
    ${PARENT_DIR}/qcommon/unzip.c
    ${PARENT_DIR}/qcommon/vm.c
    ${PARENT_DIR}/qcommon/vm_interpreted.c
    ${PARENT_DIR}/qcommon/vm_x86.c
    #
    ${PARENT_DIR}/sdl/sdl_input.c
    ${PARENT_DIR}/sdl/sdl_snd.c
    #
    ${PARENT_DIR}/server/sv_bot.c
    ${PARENT_DIR}/server/sv_ccmds.c
    ${PARENT_DIR}/server/sv_client.c
    ${PARENT_DIR}/server/sv_game.c
    ${PARENT_DIR}/server/sv_init.c
    ${PARENT_DIR}/server/sv_main.c
    ${PARENT_DIR}/server/sv_net_chan.c
    ${PARENT_DIR}/server/sv_snapshot.c
    ${PARENT_DIR}/server/sv_world.c
    #
    ${PARENT_DIR}/sys/con_log.c
    ${PARENT_DIR}/sys/con_tty.c
    ${PARENT_DIR}/sys/sys_autoupdater.c
    ${PARENT_DIR}/sys/sys_main.c
    ${PARENT_DIR}/sys/sys_unix.c
    ${PARENT_DIR}/qcommon/cm_load.c
    ${APPLE_SOURCES}
    )

if(APPLE)
 set(FRAMEWORKS "-framework Cocoa -framework Security -framework OpenAL -framework IOKit")
else(APPLE)
 if(UNIX)
  set(SYSLIBS dl rt m)
 endif(UNIX)
endif(APPLE)

if(NOT USE_RENDERER_DLOPEN)
  if(USE_OPENGL1)
  set(RENDERER_LIBRARY renderergl1)
  endif(USE_OPENGL1)
  set(RENDERER_LIBRARY renderergl2)
endif(NOT USE_RENDERER_DLOPEN)

target_link_libraries(
    ioquake3
    botlib
    #
    zlib
    ogg
    opus
    opusfile
    #
    ${FRAMEWORKS}
    ${CURL_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${SYSLIBS}
    )

include_directories(
    ${PARENT_DIR}
    ${PARENT_DIR}/zlib
    ${PARENT_DIR}/qcommon
    ${PARENT_DIR}/SDL2/include
    ${PARENT_DIR}/jpeg-8c
    ${PARENT_DIR}/libogg-1.3.1/include
    ${PARENT_DIR}/opus-1.1/include
    ${PARENT_DIR}/opusfile-0.5/include
    ${SDL2_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    )

# TODO: Turn this into a macro 
if (USE_INTERNAL_SDL2)
add_custom_command(
    TARGET ioquake3 
    POST_BUILD COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${CMAKE_SOURCE_DIR}/code/SDL2/libs/Darwin/libSDL2-2.0.0.dylib ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libSDL2-2.0.0.dylib
    )
endif(USE_INTERNAL_SDL2)

if (USE_RENDERER_DLOPEN)
    add_custom_command(
        TARGET ioquake3 
        POST_BUILD COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${CMAKE_BINARY_DIR}/code/renderergl1/librenderergl1${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/renderer_opengl1${CMAKE_SHARED_LIBRARY_SUFFIX}
        )

    add_custom_command(
        TARGET ioquake3 
        POST_BUILD COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${CMAKE_BINARY_DIR}/code/renderergl2/librenderergl2${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/renderer_opengl2${CMAKE_SHARED_LIBRARY_SUFFIX}
        )
endif(USE_RENDERER_DLOPEN)

add_dependencies(
    ioquake3 
    ui
    game
    cgame
    renderergl1
    renderergl2
    botlib
    )
