if(NOT BUILD_CLIENT)
    return()
endif()

include(utils/add_git_dependency)
include(utils/arch)
include(utils/set_output_dirs)
include(shared_sources)

include(renderer_common)

set(CLIENT_SOURCES
    ${SOURCE_DIR}/client/cl_cgame.c
    ${SOURCE_DIR}/client/cl_cin.c
    ${SOURCE_DIR}/client/cl_console.c
    ${SOURCE_DIR}/client/cl_input.c
    ${SOURCE_DIR}/client/cl_keys.c
    ${SOURCE_DIR}/client/cl_main.c
    ${SOURCE_DIR}/client/cl_net_chan.c
    ${SOURCE_DIR}/client/cl_parse.c
    ${SOURCE_DIR}/client/cl_scrn.c
    ${SOURCE_DIR}/client/cl_ui.c
    ${SOURCE_DIR}/client/cl_avi.c
    ${SOURCE_DIR}/client/libmumblelink.c
    ${SOURCE_DIR}/client/snd_altivec.c
    ${SOURCE_DIR}/client/snd_adpcm.c
    ${SOURCE_DIR}/client/snd_dma.c
    ${SOURCE_DIR}/client/snd_mem.c
    ${SOURCE_DIR}/client/snd_mix.c
    ${SOURCE_DIR}/client/snd_wavelet.c
    ${SOURCE_DIR}/client/snd_main.c
    ${SOURCE_DIR}/client/snd_codec.c
    ${SOURCE_DIR}/client/snd_codec_wav.c
    ${SOURCE_DIR}/client/snd_codec_ogg.c
    ${SOURCE_DIR}/client/snd_codec_opus.c
    ${SOURCE_DIR}/client/qal.c
    ${SOURCE_DIR}/client/snd_openal.c
    ${SOURCE_DIR}/sdl/sdl_input.c
    ${SOURCE_DIR}/sdl/sdl_snd.c
    ${CLIENT_PLATFORM_SOURCES}
)

add_git_dependency(${SOURCE_DIR}/client/cl_console.c)

if(USE_ARCHLESS_FILENAMES)
    set(CLIENT_BINARY ${CLIENT_NAME})
    list(APPEND CLIENT_DEFINITIONS USE_ARCHLESS_FILENAMES)
else()
    set(CLIENT_BINARY ${CLIENT_NAME}.${ARCH})
endif()

list(APPEND CLIENT_DEFINITIONS BOTLIB)

if(BUILD_STANDALONE)
    list(APPEND CLIENT_DEFINITIONS STANDALONE)
endif()

if(USE_RENDERER_DLOPEN)
    list(APPEND CLIENT_DEFINITIONS USE_RENDERER_DLOPEN)
endif()

if(USE_HTTP)
    list(APPEND CLIENT_DEFINITIONS USE_HTTP)
endif()

if(USE_VOIP)
    list(APPEND CLIENT_DEFINITIONS USE_VOIP)
endif()

if(USE_MUMBLE)
    list(APPEND CLIENT_DEFINITIONS USE_MUMBLE)
    list(APPEND CLIENT_LIBRARY_SOURCES ${SOURCE_DIR}/client/libmumblelink.c)
endif()

list(APPEND CLIENT_BINARY_SOURCES
    ${SERVER_SOURCES}
    ${CLIENT_SOURCES}
    ${COMMON_SOURCES}
    ${BOTLIB_SOURCES}
    ${SYSTEM_SOURCES}
    ${ASM_SOURCES}
    ${CLIENT_ASM_SOURCES}
    ${CLIENT_LIBRARY_SOURCES})

add_executable(${CLIENT_BINARY} ${CLIENT_EXECUTABLE_OPTIONS} ${CLIENT_BINARY_SOURCES})

target_include_directories(     ${CLIENT_BINARY} PRIVATE ${CLIENT_INCLUDE_DIRS})
target_compile_definitions(     ${CLIENT_BINARY} PRIVATE ${CLIENT_DEFINITIONS})
target_compile_options(         ${CLIENT_BINARY} PRIVATE ${CLIENT_COMPILE_OPTIONS})
target_link_libraries(          ${CLIENT_BINARY} PRIVATE ${COMMON_LIBRARIES} ${CLIENT_LIBRARIES})
target_link_options(            ${CLIENT_BINARY} PRIVATE ${CLIENT_LINK_OPTIONS})

set_output_dirs(${CLIENT_BINARY})

if(NOT USE_RENDERER_DLOPEN)
    target_sources(${CLIENT_BINARY} PRIVATE
        # These are never simultaneously populated
        ${RENDERER_GL1_BINARY_SOURCES}
        ${RENDERER_GL2_BINARY_SOURCES})

    target_include_directories( ${CLIENT_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS})
    target_compile_definitions( ${CLIENT_BINARY} PRIVATE ${RENDERER_DEFINITIONS})
    target_compile_options(     ${CLIENT_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_link_libraries(      ${CLIENT_BINARY} PRIVATE ${RENDERER_LIBRARIES})
endif()

foreach(LIBRARY IN LISTS CLIENT_DEPLOY_LIBRARIES)
    add_custom_command(TARGET ${CLIENT_BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${LIBRARY}
            $<TARGET_FILE_DIR:${CLIENT_BINARY}>)
endforeach()

if(POST_CLIENT_CONFIGURE_FUNCTION)
    cmake_language(CALL ${POST_CLIENT_CONFIGURE_FUNCTION})
endif()

install(TARGETS ${CLIENT_BINARY}
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .)
