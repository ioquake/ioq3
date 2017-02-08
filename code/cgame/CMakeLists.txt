
#
##  ____                                           _      
## / ___|__ _  __ _ _ __ ___   ___    ___ ___   __| | ___ 
##| |   / _` |/ _` | '_ ` _ \ / _ \  / __/ _ \ / _` |/ _ \
##| |__| (_| | (_| | | | | | |  __/ | (_| (_) | (_| |  __/
## \____\__, |\__,_|_| |_| |_|\___|  \___\___/ \__,_|\___|
##      |___/                                             
#

set(CMAKE_INSTALL_NAME_DIR ${PROJECT_BINARY_DIR}/baseq3)

set(BG_SOURCE_DIR ../game)
set(QC_SOURCE_DIR ../qcommon)
set(UI_SOURCE_DIR ../ui)
set(RC_SOURCE_DIR ../renderercommon)

add_definitions( -DCGAME -DMISSIONPACK )

set( CGAME_SOURCES
    cg_main.c # MUST BE FIRST
    cg_consolecmds.c
    cg_draw.c
    cg_drawtools.c
    cg_effects.c
    cg_ents.c
    cg_event.c
    cg_info.c
    cg_local.h
    cg_localents.c
    cg_marks.c
    cg_newdraw.c
    cg_particles.c
    cg_players.c
    cg_playerstate.c
    cg_predict.c
    cg_public.h
    cg_scoreboard.c
    cg_servercmds.c
    cg_snapshot.c
    cg_syscalls.asm
    cg_syscalls.c
    cg_view.c
    cg_weapons.c
    ${BG_SOURCE_DIR}/bg_lib.h
    ${BG_SOURCE_DIR}/bg_public.h
    ${BG_SOURCE_DIR}/bg_lib.c
    ${BG_SOURCE_DIR}/bg_misc.c
    ${BG_SOURCE_DIR}/bg_pmove.c
    ${BG_SOURCE_DIR}/bg_slidemove.c
    #
    ${UI_SOURCE_DIR}/ui_shared.h
    ${UI_SOURCE_DIR}/ui_shared.c
    #
    ${QC_SOURCE_DIR}/q_shared.h
    ${QC_SOURCE_DIR}/q_shared.c
    ${QC_SOURCE_DIR}/q_math.c
    #
    ${RC_SOURCE_DIR}/tr_types.h
    )

add_library( cgame SHARED ${CGAME_SOURCES} cg_syscalls.c )

target_include_directories(
    cgame PUBLIC
    ${BG_SOURCE_DIR}
    ${QC_SOURCE_DIR}
    ${RC_SOURCE_DIR}
    ${UI_SOURCE_DIR}
    )

include( ${CMAKE_SOURCE_DIR}/cmake/AddQVM.cmake )
add_qvm( cgame ${CGAME_SOURCES} cg_syscalls.asm )


add_custom_command(
    TARGET cgame POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/libcgame${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/baseq3/cgame${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
