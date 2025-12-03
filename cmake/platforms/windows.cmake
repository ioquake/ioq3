# Windows specific settings

if(NOT WIN32)
    return()
endif()

list(APPEND SYSTEM_PLATFORM_SOURCES
    ${SOURCE_DIR}/sys/sys_win32.c
    ${SOURCE_DIR}/sys/win_resource.rc
)

list(APPEND CLIENT_PLATFORM_SOURCES ${SOURCE_DIR}/sys/con_passive.c)
list(APPEND SERVER_PLATFORM_SOURCES ${SOURCE_DIR}/sys/con_win32.c)

if(USE_HTTP)
    list(APPEND CLIENT_PLATFORM_SOURCES ${SOURCE_DIR}/client/cl_http_windows.c)
    list(APPEND CLIENT_LIBRARIES wininet)
endif()

list(APPEND COMMON_LIBRARIES
    ws2_32 # Windows Sockets 2
    winmm  # timeBeginPeriod/timeEndPeriod
    psapi  # EnumProcesses
)

if(MINGW)
    list(APPEND COMMON_LIBRARIES mingw32)
endif()

list(APPEND CLIENT_DEFINITIONS USE_ICON)

set_source_files_properties(${SOURCE_DIR}/sys/win_resource.rc
    PROPERTIES COMPILE_DEFINITIONS WINDOWS_ICON_PATH="${WINDOWS_ICON_PATH}")

if(MSVC)
    # We have our own manifest, disable auto creation
    list(APPEND SERVER_LINK_OPTIONS "/MANIFEST:NO")
    list(APPEND CLIENT_LINK_OPTIONS "/MANIFEST:NO")
endif()

set(CLIENT_EXECUTABLE_OPTIONS WIN32)

# It's only necessary to set this on Windows; elsewhere
# CMAKE_EXECUTABLE_SUFFIX will be empty anyway, or we want
# HOST_EXECUTABLE_SUFFIX to be empty for other reasons
set(HOST_EXECUTABLE_SUFFIX ${CMAKE_EXECUTABLE_SUFFIX})

set(CPACK_GENERATOR NSIS)
set(CPACK_NSIS_MUI_ICON ${WINDOWS_ICON_PATH})
set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
