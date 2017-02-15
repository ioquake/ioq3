#
##  _   _ ___    ____          _      
## | | | |_ _|  / ___|___   __| | ___ 
## | | | || |  | |   / _ \ / _` |/ _ \
## | |_| || |  | |__| (_) | (_| |  __/
##  \___/|___|  \____\___/ \__,_|\___|
##                                    
#

set(CMAKE_INSTALL_NAME_DIR ${PROJECT_BINARY_DIR}/baseq3)

set(BG_SOURCE_DIR ../game)
set(QC_SOURCE_DIR ../qcommon)
set(RC_SOURCE_DIR ../renderercommon)
set(CLIENT_SOURCE_DIR ../client)

add_definitions( -DUI -DMISSIONPACK )

set(UI_SOURCES
    ui_main.c # MUST BE FIRST
    ui_atoms.c
    ui_gameinfo.c
    ui_local.h
    ui_players.c
    ui_public.h
    ui_shared.c
    ui_shared.h
    ${CMAKE_SOURCE_DIR}/ui/menudef.h
    ${BG_SOURCE_DIR}/bg_lib.h
    ${BG_SOURCE_DIR}/bg_public.h
    ${BG_SOURCE_DIR}/bg_lib.c
    ${BG_SOURCE_DIR}/bg_misc.c
    ${QC_SOURCE_DIR}/q_shared.h
    ${QC_SOURCE_DIR}/q_shared.c
    ${QC_SOURCE_DIR}/q_math.c
    ${RC_SOURCE_DIR}/tr_types.h
    ${CLIENT_SOURCE_DIR}/keycodes.h
    )

add_library(
    ui SHARED
    ${UI_SOURCES}
    ui_syscalls.c
    )

target_include_directories(
    ui PUBLIC
    ${QC_SOURCE_DIR}
    ${BG_SOURCE_DIR}
    )

include(${CMAKE_SOURCE_DIR}/cmake/AddQVM.cmake)
add_qvm( ui MISSIONPACK ${UI_SOURCES} ui_syscalls.asm )

add_custom_command(
    TARGET ui POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/libui${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/baseq3/ui${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
