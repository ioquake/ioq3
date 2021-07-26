set(SRCS
	server.h
	sv_bot.c
	sv_ccmds.c
	sv_client.c
	sv_game.c
	sv_init.c
	sv_main.c
	sv_net_chan.c
	sv_snapshot.c
	sv_world.c

	../null/null_client.c
	../null/null_input.c
	../null/null_snddma.c
	${QCOMMON_COLLISION_SRCS}
	../qcommon/cmd.c
	../qcommon/common.c
	../qcommon/cvar.c
	../qcommon/files.c
	../qcommon/huffman.c
	../qcommon/ioapi.c
	../qcommon/md4.c
	../qcommon/msg.c
	../qcommon/net_chan.c
	../qcommon/net_ip.c
	../qcommon/q_math.c
	../qcommon/q_shared.c
	../qcommon/unzip.c
	${QCOMMON_VM_SRCS}
	../sys/con_log.c
	../sys/sys_loadlib.h
	../sys/sys_local.h
	../sys/sys_main.c
	../sys/sys_autoupdater.c
)

if (WIN32)
	list(APPEND SRCS ../sys/sys_win32.c)
	list(APPEND SRCS ../sys/con_win32.c)
	list(APPEND SRCS ../sys/win_resource.rc)
else()
	if(APPLE)
		list(APPEND SRCS ../sys/sys_osx.m)
	endif()
	list(APPEND SRCS ../sys/sys_unix.c)
	list(APPEND SRCS ../sys/con_tty.c)
endif()

project(${SERVERBIN})
q3_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
add_asm(${PROJECT_NAME})
add_botlib(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} baseq3-qagame missionpack-qagame)
set(SERVER_DEFINES -DDEDICATED)
set(LIBS zlib ${CMAKE_DL_LIBS})
if (MSVC)
	list(APPEND LIBS ws2_32 winmm psapi)
elseif (APPLE)
	set(FRAMEWORKS Cocoa)
	foreach (_framework ${FRAMEWORKS})
		list(APPEND LIBS "-framework ${_framework}")
	endforeach()
	list(APPEND SERVER_DEFINES -D_THREAD_SAFE=1)
else()
	set(CMAKE_REQUIRED_LIBRARIES m)
	check_symbol_exists("cosf" "math.h" HAVE_COSF)
	set(CMAKE_REQUIRED_LIBRARIES)
	list(APPEND LIBS m)
endif()
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ../qcommon)
if (USE_VOIP)
	list(APPEND SERVER_DEFINES USE_VOIP)
endif()
if (BUILD_AUTOUPDATER)
	list(APPEND SERVER_DEFINES USE_AUTOUPDATER)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE ${SERVER_DEFINES})
