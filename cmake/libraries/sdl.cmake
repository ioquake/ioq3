set(INTERNAL_SDL_DIR ${SOURCE_DIR}/thirdparty/SDL3-3.2.18)

include(utils/arch)

if(NOT WIN32 AND NOT APPLE)
    set(SYSTEM_SDL_REQUIRED REQUIRED)
endif()

find_package(SDL3 QUIET ${SYSTEM_SDL_REQUIRED})

if(NOT SDL3_FOUND)
    set(SDL3_INCLUDE_DIRS ${INTERNAL_SDL_DIR}/include)

    # On Windows and macOS we have internal SDL binaries we can use
    if(WIN32)
        if(ARCH STREQUAL "x86_64")
            set(LIB_DIR ${SOURCE_DIR}/thirdparty/libs/win64)
        elseif(ARCH STREQUAL "x86")
            set(LIB_DIR ${SOURCE_DIR}/thirdparty/libs/win32)
        else()
            message(FATAL_ERROR "Unknown ARCH")
        endif()

        if(MINGW)
            set(SDL3_LIBRARIES
                ${LIB_DIR}/libSDL3.dll.a)
        elseif(MSVC)
            set(SDL3_LIBRARIES
                ${LIB_DIR}/SDL3.lib)
        endif()

        list(APPEND CLIENT_DEPLOY_LIBRARIES ${LIB_DIR}/SDL3.dll)
    elseif(APPLE)
        set(SDL3_LIBRARIES
            ${SOURCE_DIR}/thirdparty/libs/macos/libSDL3.dylib)
        list(APPEND CLIENT_DEPLOY_LIBRARIES
            ${SOURCE_DIR}/thirdparty/libs/macos/libSDL3.dylib)
    else()
        message(FATAL_ERROR "SDL3 not found and no internal binaries available")
    endif()
endif()

list(APPEND CLIENT_LIBRARIES ${SDL3_LIBRARIES})
list(APPEND CLIENT_INCLUDE_DIRS ${SDL3_INCLUDE_DIRS})
list(APPEND CLIENT_COMPILE_OPTIONS ${SDL3_CFLAGS_OTHER})
list(APPEND RENDERER_LIBRARIES ${SDL3_LIBRARIES})
list(APPEND RENDERER_INCLUDE_DIRS ${SDL3_INCLUDE_DIRS})
list(APPEND RENDERER_COMPILE_OPTIONS ${SDL3_CFLAGS_OTHER})
