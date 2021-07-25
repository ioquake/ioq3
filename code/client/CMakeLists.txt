set(SRCS
	cl_cgame.c
	cl_cin.c
	cl_console.c
	cl_input.c
	cl_keys.c
	cl_main.c
	cl_net_chan.c
	cl_parse.c
	cl_scrn.c
	cl_ui.c
	cl_avi.c
	snd_altivec.c
	snd_adpcm.c
	snd_dma.c
	snd_mem.c
	snd_mix.c
	snd_wavelet.c
	snd_main.c
	snd_codec.c
	snd_codec_wav.c
	snd_codec_ogg.c
	snd_codec_opus.c
	qal.c
	snd_openal.c
	cl_curl.c
	../qcommon/cm_load.c
	../qcommon/cm_patch.c
	../qcommon/cm_polylib.c
	../qcommon/cm_test.c
	../qcommon/cm_trace.c
	../qcommon/cmd.c
	../qcommon/common.c
	../qcommon/cvar.c
	../qcommon/files.c
	../qcommon/md4.c
	../qcommon/md5.c
	../qcommon/msg.c
	../qcommon/net_chan.c
	../qcommon/net_ip.c
	../qcommon/huffman.c
	../qcommon/q_math.c
	../qcommon/q_shared.c
	../qcommon/unzip.c
	../qcommon/ioapi.c
	../qcommon/puff.c
	../qcommon/vm.c
	../qcommon/vm_interpreted.c
	../qcommon/vm_x86.c
	../server/sv_bot.c
	../server/sv_ccmds.c
	../server/sv_client.c
	../server/sv_game.c
	../server/sv_init.c
	../server/sv_main.c
	../server/sv_net_chan.c
	../server/sv_snapshot.c
	../server/sv_world.c
	../sdl/sdl_input.c
	../sdl/sdl_snd.c
	../sys/con_log.c
	../sys/sys_main.c
	../sys/sys_autoupdater.c
)

if (WIN32)
	list(APPEND SRCS ../sys/sys_win32.c)
	list(APPEND SRCS ../sys/con_passive.c)
	list(APPEND SRCS ../sys/win_resource.rc)
else()
	if(APPLE)
		list(APPEND SRCS ../sys/sys_osx.m)
	endif()
	list(APPEND SRCS ../sys/sys_unix.c)
	list(APPEND SRCS ../sys/con_tty.c)
endif()

if (USE_MUMBLE)
	list(APPEND SRCS libmumblelink.c)
endif()

project(${CLIENTBIN})
q3_add_executable(TARGET ${PROJECT_NAME} WINDOWED SRCS ${SRCS})
add_asm(${PROJECT_NAME})
add_botlib(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} baseq3-cgame baseq3-qagame baseq3-ui missionpack-cgame missionpack-qagame missionpack-ui ${RENDERER_LIST})
set(CLIENT_DEFINES)
set(LIBS opusfile opus vorbis zlib openal ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS})
if (MSVC)
	list(APPEND LIBS ws2_32 winmm psapi gdi32 ole32)
elseif (APPLE)
	set(FRAMEWORKS Cocoa Security IOKit)
	foreach (_framework ${FRAMEWORKS})
		list(APPEND LIBS "-framework ${_framework}")
	endforeach()
	list(APPEND CLIENT_DEFINES _THREAD_SAFE=1)
else()
	set(CMAKE_REQUIRED_LIBRARIES m)
	check_symbol_exists("cosf" "math.h" HAVE_COSF)
	set(CMAKE_REQUIRED_LIBRARIES)
	list(APPEND LIBS m rt)
endif()
list(APPEND LIBS curl)
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
if (USE_VOIP)
	list(APPEND CLIENT_DEFINES USE_VOIP)
endif()
if (USE_MUMBLE)
	list(APPEND CLIENT_DEFINES USE_MUMBLE)
endif()
if (BUILD_AUTOUPDATER)
	list(APPEND CLIENT_DEFINES USE_AUTOUPDATER)
endif()
if (USE_RENDERER_DLOPEN)
	list(APPEND CLIENT_DEFINES USE_RENDERER_DLOPEN)
endif()
if (CLIENT_DEFINES)
	target_compile_definitions(${PROJECT_NAME} PRIVATE ${CLIENT_DEFINES})
endif()
