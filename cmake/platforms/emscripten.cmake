# Emscripten specific settings

if(NOT EMSCRIPTEN)
    return()
endif()

set(CMAKE_EXECUTABLE_SUFFIX ".js")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".wasm")

# Disable options that don't make sense for emscripten
set(BUILD_SERVER OFF CACHE INTERNAL "")
set(BUILD_RENDERER_GL1 OFF CACHE INTERNAL "")
set(USE_RENDERER_DLOPEN OFF CACHE INTERNAL "")
set(USE_OPENAL_DLOPEN OFF CACHE INTERNAL "")
set(BUILD_GAME_LIBRARIES OFF CACHE INTERNAL "")
set(USE_HTTP OFF CACHE INTERNAL "")

list(APPEND CLIENT_LINK_OPTIONS
    -sTOTAL_MEMORY=256MB
    -sSTACK_SIZE=5MB
    -sMIN_WEBGL_VERSION=1
    -sMAX_WEBGL_VERSION=2
    -sEXPORTED_RUNTIME_METHODS=FS,addRunDependency,removeRunDependency
    -sEXIT_RUNTIME=1
    -sEXPORT_ES6
    -sEXPORT_NAME=${CLIENT_NAME}
)

option(EMSCRIPTEN_PRELOAD_FILE "Preload game files into .data file" OFF)

if(EMSCRIPTEN_PRELOAD_FILE)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${BASEGAME}")
        message(FATAL_ERROR "No files in '${BASEGAME}' directory for emscripten to preload.")
    endif()
    list(APPEND CLIENT_LINK_OPTIONS "--preload-file ${BASEGAME}")
endif()

set(POST_CLIENT_CONFIGURE_FUNCTION deploy_shell_files)

function(deploy_shell_files)
    configure_file(${SOURCE_DIR}/web/client.html.in
        ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CLIENT_NAME}.html @ONLY)

    if(NOT EMSCRIPTEN_PRELOAD_FILE)
        configure_file(${SOURCE_DIR}/web/client-config.json
            ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CLIENT_NAME}-config.json COPYONLY)
    endif()
endfunction()
