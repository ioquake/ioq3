
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../qcommon 
    )

add_definitions( -DBOTLIB )

add_library(
    botlib STATIC

    aasfile.h
    be_aas.h
    be_aas_bsp.h
    be_aas_bspq3.c
    be_aas_cluster.c
    be_aas_cluster.h
    be_aas_debug.c
    be_aas_debug.h
    be_aas_def.h
    be_aas_entity.c
    be_aas_entity.h
    be_aas_file.c
    be_aas_file.h
    be_aas_funcs.h
    be_aas_main.c
    be_aas_main.h
    be_aas_move.c
    be_aas_move.h
    be_aas_optimize.c
    be_aas_optimize.h
    be_aas_reach.c
    be_aas_reach.h
    be_aas_route.c
    be_aas_route.h
    be_aas_routealt.c
    be_aas_routealt.h
    be_aas_sample.c
    be_aas_sample.h
    be_ai_char.c
    be_ai_char.h
    be_ai_chat.c
    be_ai_chat.h
    be_ai_gen.c
    be_ai_gen.h
    be_ai_goal.c
    be_ai_goal.h
    be_ai_move.c
    be_ai_move.h
    be_ai_weap.c
    be_ai_weap.h
    be_ai_weight.c
    be_ai_weight.h
    be_ea.c
    be_ea.h
    be_interface.c
    be_interface.h
    botlib.h
    l_crc.c
    l_crc.h
    l_libvar.c
    l_libvar.h
    l_log.c
    l_log.h
    l_memory.c
    l_memory.h
    l_precomp.c
    l_precomp.h
    l_script.c
    l_script.h
    l_struct.c
    l_struct.h
    l_utils.h
    )

