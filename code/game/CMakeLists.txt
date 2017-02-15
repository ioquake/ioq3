
#
##    ____                                      _      
##   / ___| __ _ _ __ ___   ___    ___ ___   __| | ___ 
##  | |  _ / _` | '_ ` _ \ / _ \  / __/ _ \ / _` |/ _ \
##  | |_| | (_| | | | | | |  __/ | (_| (_) | (_| |  __/
##   \____|\__,_|_| |_| |_|\___|  \___\___/ \__,_|\___|
##                                                     
#

set(CMAKE_INSTALL_NAME_DIR ${PROJECT_BINARY_DIR}/baseq3)

set(QC_SOURCE_DIR ../qcommon)

add_definitions( -DGAME -DMISSIONPACK )

set( GAME_SOURCES
    g_main.c # MUST BE FIRST
    ai_chat.c
    ai_chat.h
    ai_cmd.c
    ai_cmd.h
    ai_dmnet.c
    ai_dmnet.h
    ai_dmq3.c
    ai_dmq3.h
    ai_main.c
    ai_main.h
    ai_team.c
    ai_team.h
    ai_vcmd.c
    ai_vcmd.h
    bg_lib.c
    bg_lib.h
    bg_local.h
    bg_misc.c
    bg_pmove.c
    bg_public.h
    bg_slidemove.c
    chars.h
    g_active.c
    g_arenas.c
    g_bot.c
    g_client.c
    g_cmds.c
    g_combat.c
    g_items.c
    g_local.h
    g_mem.c
    g_misc.c
    g_missile.c
    g_mover.c
    g_public.h
    g_rankings.h
    g_session.c
    g_spawn.c
    g_svcmds.c
    g_target.c
    g_team.c
    g_team.h
    g_trigger.c
    g_utils.c
    g_weapon.c
    inv.h
    match.h
    syn.h
    ${QC_SOURCE_DIR}/q_shared.h 
    ${QC_SOURCE_DIR}/q_shared.c
    ${QC_SOURCE_DIR}/q_math.c
    )

add_library( game SHARED ${GAME_SOURCES} g_syscalls.c )

include( ${CMAKE_SOURCE_DIR}/cmake/AddQVM.cmake )
add_qvm( game MISSIONPACK ${GAME_SOURCES} g_syscalls.asm )

add_custom_command(
    TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/libgame${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/baseq3/game${CMAKE_SHARED_LIBRARY_SUFFIX}
    )

