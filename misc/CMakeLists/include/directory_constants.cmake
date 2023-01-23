
set(MOUNT_DIR "${SOURCE_DIR}/code" CACHE PATH "path to the code directory")

set(ASM_DIR "${MOUNT_DIR}/asm")
set(BOTLIB_DIR "${MOUNT_DIR}/botlib")
set(CGAME_DIR "${MOUNT_DIR}/cgame")
set(GAME_DIR "${MOUNT_DIR}/game")
set(LIBS_DIR "${MOUNT_DIR}/libs")
set(Q3_UI_DIR "${MOUNT_DIR}/q3_ui")
set(QCOMMON_DIR "${MOUNT_DIR}/qcommon")
set(SERVER_DIR "${MOUNT_DIR}/server")
set(SYS_DIR "${MOUNT_DIR}/sys")
set(UI_DIR "${MOUNT_DIR}/ui")

#client and renderer
set(LIBSDL_DIR "${MOUNT_DIR}/SDL2")
set(SDL_DIR "${MOUNT_DIR}/sdl")

#client only
set(CLIENT_DIR "${MOUNT_DIR}/client")
set(CURL_DIR "${MOUNT_DIR}/curl-7.54.0")

#server only
set(NULL_DIR "${MOUNT_DIR}/null")

#renderer
set(RENDERERCOMMON_DIR "${MOUNT_DIR}/renderercommon")
set(RENDERERGL1_DIR "${MOUNT_DIR}/renderergl1")
set(RENDERERGL2_DIR "${MOUNT_DIR}/renderergl2")
set(GLSL_DIR "${RENDERERGL2_DIR}/glsl")

#internal libs
set(JPEG_DIR "${MOUNT_DIR}/jpeg-8c")
set(OGG_DIR "${MOUNT_DIR}/libogg-1.3.3")
set(OPUS_DIR "${MOUNT_DIR}/opus-1.2.1")
set(OPUSFILE_DIR "${MOUNT_DIR}/opusfile-0.9")
set(SPEEX_DIR "${MOUNT_DIR}/libspeex")
set(VORBIS_DIR "${MOUNT_DIR}/libvorbis-1.3.6")
set(ZLIB_DIR "${MOUNT_DIR}/zlib")
