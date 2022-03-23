project(game)
set(SRCS
	ai_chat.c ai_chat.h
	ai_cmd.c ai_cmd.h
	ai_dmnet.c ai_dmnet.h
	ai_dmq3.c ai_dmq3.h
	ai_main.c ai_main.h
	ai_team.c ai_team.h
	ai_vcmd.c ai_vcmd.h
	bg_lib.c bg_lib.h
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
	g_main.c
	g_mem.c
	g_misc.c
	g_missile.c
	g_mover.c
	g_public.h
	#g_rankings.c g_rankings.h
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

	../qcommon/q_math.c
	../qcommon/q_shared.c
)
set(LIB_SRCS g_syscalls.c)
set(QVM_SRCS ${SRCS} g_syscalls.asm)
list(APPEND SRCS ${LIB_SRCS})
q3_add_library(TARGET qagame GAME_NAME baseq3 SRCS ${SRCS})
target_compile_definitions(baseq3-qagame PRIVATE QAGAME)
add_qvm(qagame baseq3 ${QVM_SRCS})

q3_add_library(TARGET qagame GAME_NAME missionpack SRCS ${SRCS})
target_compile_definitions(missionpack-qagame PRIVATE QAGAME MISSIONPACK)
add_qvm(qagame missionpack ${QVM_SRCS})
