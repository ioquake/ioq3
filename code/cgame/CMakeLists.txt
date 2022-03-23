project(cgame)
set(SRCS
	cg_consolecmds.c
	cg_draw.c
	cg_drawtools.c
	cg_effects.c
	cg_ents.c
	cg_event.c
	cg_info.c
	cg_localents.c
	cg_local.h
	cg_main.c
	cg_marks.c
	cg_particles.c
	cg_players.c
	cg_playerstate.c
	cg_predict.c
	cg_public.h
	cg_scoreboard.c
	cg_servercmds.c
	cg_snapshot.c
	cg_view.c
	cg_weapons.c

	../game/bg_misc.c
	../game/bg_pmove.c
	../game/bg_slidemove.c
	../game/bg_lib.c

	../qcommon/q_math.c
	../qcommon/q_shared.c
)
set(LIB_SRCS cg_syscalls.c)
set(QVM_SRCS ${SRCS} cg_syscalls.asm)
list(APPEND SRCS ${LIB_SRCS})

q3_add_library(TARGET cgame GAME_NAME baseq3 SRCS ${SRCS})
target_compile_definitions(baseq3-cgame PRIVATE CGAME)
add_qvm(${PROJECT_NAME} baseq3 ${QVM_SRCS})

list(APPEND SRCS cg_newdraw.c)
q3_add_library(TARGET cgame GAME_NAME missionpack SRCS ${SRCS})
target_compile_definitions(missionpack-cgame PRIVATE CGAME MISSIONPACK)
add_qvm(${PROJECT_NAME} missionpack ${QVM_SRCS})
