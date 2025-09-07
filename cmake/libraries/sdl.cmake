if(NOT BUILD_CLIENT)
    return()
endif()

if(EMSCRIPTEN)
    # Emscripten provides its own self contained SDL setup
    list(APPEND CLIENT_COMPILE_OPTIONS -sUSE_SDL=2)
    list(APPEND CLIENT_LINK_OPTIONS -sUSE_SDL=2)
    return()
endif()

set(INTERNAL_SDL_DIR ${SOURCE_DIR}/thirdparty/SDL2-2.32.8)

include(utils/arch)

if(WIN32 OR APPLE)
    # On Windows and macOS we have internal SDL binaries we can use
    set(HAVE_INTERNAL_SDL true)
endif()

if(USE_INTERNAL_SDL AND HAVE_INTERNAL_SDL)
    set(SDL2_INCLUDE_DIRS ${INTERNAL_SDL_DIR}/include)
    list(APPEND CLIENT_DEFINITIONS USE_INTERNAL_SDL_HEADERS)
    list(APPEND RENDERER_DEFINITIONS USE_INTERNAL_SDL_HEADERS)

    if(WIN32)
        if(ARCH STREQUAL "x86_64")
            set(LIB_DIR ${SOURCE_DIR}/thirdparty/libs/win64)
        elseif(ARCH STREQUAL "x86")
            set(LIB_DIR ${SOURCE_DIR}/thirdparty/libs/win32)
        else()
            message(FATAL_ERROR "Unknown ARCH")
        endif()

        if(MINGW)
            set(SDL2_LIBRARIES
                ${LIB_DIR}/libSDL2main.a
                ${LIB_DIR}/libSDL2.dll.a)
        elseif(MSVC)
            set(SDL2_LIBRARIES
                ${LIB_DIR}/SDL2main.lib
                ${LIB_DIR}/SDL2.lib)
        endif()

        list(APPEND CLIENT_DEPLOY_LIBRARIES ${LIB_DIR}/SDL2.dll)
    elseif(APPLE)
        set(SDL2_LIBRARIES
            ${SOURCE_DIR}/thirdparty/libs/macos/libSDL2main.a
            ${SOURCE_DIR}/thirdparty/libs/macos/libSDL2-2.0.0.dylib)
        list(APPEND CLIENT_DEPLOY_LIBRARIES
            ${SOURCE_DIR}/thirdparty/libs/macos/libSDL2-2.0.0.dylib)
    else()
        message(FATAL_ERROR "HAVE_INTERNAL_SDL set incorrectly; file a bug")
    endif()
else()
    find_package(SDL2 REQUIRED)
endif()

list(APPEND CLIENT_LIBRARIES ${SDL2_LIBRARIES})
list(APPEND CLIENT_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
list(APPEND CLIENT_COMPILE_OPTIONS ${SDL2_CFLAGS_OTHER})
list(APPEND RENDERER_LIBRARIES ${SDL2_LIBRARIES})
list(APPEND RENDERER_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
list(APPEND RENDERER_COMPILE_OPTIONS ${SDL2_CFLAGS_OTHER})
