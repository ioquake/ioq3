project(ui)
set(SRCS
	ui_addbots.c
	ui_atoms.c
	ui_cdkey.c
	ui_cinematics.c
	ui_confirm.c
	ui_connect.c
	ui_controls2.c
	ui_credits.c
	ui_demo2.c
	ui_display.c
	ui_gameinfo.c
	ui_ingame.c
	ui_loadconfig.c
	ui_local.h
	#ui_login.c
	ui_main.c
	ui_menu.c
	ui_mfield.c
	ui_mods.c
	ui_network.c
	ui_options.c
	ui_playermodel.c
	ui_players.c
	ui_playersettings.c
	ui_preferences.c
	ui_qmenu.c
	#ui_rankings.c
	#ui_rankstatus.c
	ui_removebots.c
	ui_saveconfig.c
	ui_serverinfo.c
	ui_servers2.c
	ui_setup.c
	#ui_signup.c
	ui_sound.c
	ui_sparena.c
	#ui_specifyleague.c
	ui_specifyserver.c
	ui_splevel.c
	ui_sppostgame.c
	ui_spreset.c
	ui_spskill.c
	ui_startserver.c
	ui_team.c
	ui_teamorders.c
	ui_video.c

	../game/bg_misc.c
	../game/bg_lib.c
	../qcommon/q_math.c
	../qcommon/q_shared.c
)
set(LIB_SRCS ../ui/ui_syscalls.c)
set(QVM_SRCS ${SRCS} ../ui/ui_syscalls.asm)
list(APPEND SRCS ${LIB_SRCS})

q3_add_library(TARGET ui GAME_NAME baseq3 SRCS ${SRCS})
target_compile_definitions(baseq3-ui PRIVATE UI)
add_qvm(${PROJECT_NAME} baseq3 ${QVM_SRCS})

q3_add_library(TARGET ui GAME_NAME missionpack SRCS ${SRCS})
target_compile_definitions(missionpack-ui PRIVATE UI MISSIONPACK)
add_qvm(${PROJECT_NAME} missionpack ${QVM_SRCS})
