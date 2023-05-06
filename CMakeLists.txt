cmake_minimum_required(VERSION 3.0)

project(ioquake3 C)
set(CMAKE_C_STANDARD 23)

option(ENABLE_PROFILING "Enable LLVM Profiler" OFF)

# Set the default build type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Output the selected build type
message("Current build type is: ${CMAKE_BUILD_TYPE}")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get the architecture of the current system
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ARCH "x64")
else()
	set(ARCH "x86")
endif()

# set the number of jobs to the number of available processor cores
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

# Add libraries
set(LIBRARIES
  opengl32
)

# Find and link zlib library
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

# Find and link openal-soft library
find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})

# Find and link SDL2 library
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Find and link Curl library
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Find and link Ogg library
find_package(Ogg REQUIRED)
include_directories(${OGG_INCLUDE_DIRS})

# Find and link jpeg library
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

# Find and link png library
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

# Set the location of the source files
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/code")

# Find all source files recursively in the code directory
file(GLOB_RECURSE SOURCES
	"${SOURCE_DIR}/*.c"
	"${SOURCE_DIR}/*.h"
)

include_directories(HEADER_DIRS
	"${SOURCE_DIR}"
)

# Set the location of MSYS2 libray files
set(DLL_DIR "D:/MSYS64/clang64/bin")

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_lazy_load_dylib")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdebug-prefix-map=${CMAKE_SOURCE_DIR}=${CMAKE_BINARY_DIR}")
endif()

# Release Build Flags
set(CMAKE_C_FLAGS_RELEASE "-m64 -O3 -DNDEBUG -DBOTLIB -march=native -fomit-frame-pointer -funroll-loops -mtune=native -flto -fno-plt -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security")
#set(CMAKE_CXX_FLAGS_RELEASE "-m64 -O3 -DNDEBUG -DBOTLIB -march=native -fomit-frame-pointer -funroll-loops -mtune=native -flto -fno-plt -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -fprofile-use=${CMAKE_BINARY_DIR}/profiling")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -static-libstdc++")
message(STATUS "CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")

# Debug Build Flags
set(CMAKE_C_FLAGS_DEBUG "-m64 -g -Og -DDEBUG -DBOTLIB -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address,undefined -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -fno-eliminate-unused-debug-types")
#set(CMAKE_CXX_FLAGS_DEBUG "-m64 -g -Og -DDEBUG -DBOTLIB -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address,undefined -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -fprofile-generate -fprofile-dir=${CMAKE_BINARY_DIR}/profiling")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-static-libstdc++")
message(STATUS "CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")

# Set the compile options for all targets to include the multi-processor flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Set the version number
set(BUILD_VERSION "1.0.0")

################################################################################################
# Create Q3ASM
################################################################################################
#set(Q3ASM_NAME "q3asm${ARCH}")
#add_executable(${Q3ASM_NAME}
#${SOURCE_DIR}/tools/asm/cmdlib.c
#${SOURCE_DIR}/tools/asm/q3asm.c
#)

################################################################################################
# END OF Q3ASM                                                                                 #
################################################################################################
# //////////////////////////////////////////////////////////////////////////////////////////// #
################################################################################################
# Create CGAME shared library
################################################################################################
set(CGAME_NAME "cgame${ARCH}")
add_library(${CGAME_NAME} SHARED 
	${SOURCE_DIR}/cgame/cg_consolecmds.c
	${SOURCE_DIR}/cgame/cg_draw.c
	${SOURCE_DIR}/cgame/cg_drawtools.c
	${SOURCE_DIR}/cgame/cg_effects.c
	${SOURCE_DIR}/cgame/cg_ents.c
	${SOURCE_DIR}/cgame/cg_event.c
	${SOURCE_DIR}/cgame/cg_info.c
	${SOURCE_DIR}/cgame/cg_local.h
	${SOURCE_DIR}/cgame/cg_localents.c
	${SOURCE_DIR}/cgame/cg_main.c
	${SOURCE_DIR}/cgame/cg_marks.c
	#${SOURCE_DIR}/cgame/cg_newDraw.c # only for Missionpack
	${SOURCE_DIR}/cgame/cg_particles.c
	${SOURCE_DIR}/cgame/cg_players.c
	${SOURCE_DIR}/cgame/cg_playerstate.c
	${SOURCE_DIR}/cgame/cg_predict.c
	${SOURCE_DIR}/cgame/cg_public.h
	${SOURCE_DIR}/cgame/cg_scoreboard.c
	${SOURCE_DIR}/cgame/cg_servercmds.c
	${SOURCE_DIR}/cgame/cg_snapshot.c
	${SOURCE_DIR}/cgame/cg_syscalls.c
	${SOURCE_DIR}/cgame/cg_view.c
	${SOURCE_DIR}/cgame/cg_weapons.c
	${SOURCE_DIR}/game/bg_lib.c
	${SOURCE_DIR}/game/bg_misc.c
	${SOURCE_DIR}/game/bg_pmove.c
	${SOURCE_DIR}/game/bg_public.h
	${SOURCE_DIR}/game/bg_slidemove.c
	${SOURCE_DIR}/qcommon/q_math.c
	${SOURCE_DIR}/qcommon/q_shared.c
	${SOURCE_DIR}/qcommon/q_shared.h
	${SOURCE_DIR}/qcommon/surfaceflags.h
	${SOURCE_DIR}/ui/ui_shared.c
	${CMAKE_CURRENT_SOURCE_DIR}/ui/menudef.h
)

# Add the include directory
target_include_directories(${CGAME_NAME} PUBLIC
${HEADER_DIRS})

# Set the library version
set_target_properties(${CGAME_NAME} PROPERTIES 
	VERSION ${BUILD_VERSION}
	SOVERSION 1
	PREFIX ""
)
################################################################################################
# END OF CGAME                                                                                 #
################################################################################################
# //////////////////////////////////////////////////////////////////////////////////////////// #
################################################################################################
# Create CGAME shared library                                                                  #
################################################################################################
set(GAME_NAME "game${ARCH}")
add_library(${GAME_NAME} SHARED 
	${SOURCE_DIR}/botlib/be_aas.h
	${SOURCE_DIR}/botlib/be_ai_char.h
	${SOURCE_DIR}/botlib/be_ai_chat.h
	${SOURCE_DIR}/botlib/be_ai_gen.h
	${SOURCE_DIR}/botlib/be_ai_goal.h
	${SOURCE_DIR}/botlib/be_ai_move.h
	${SOURCE_DIR}/botlib/be_ai_weap.h
	${SOURCE_DIR}/botlib/be_ea.h
	${SOURCE_DIR}/botlib/botlib.h
	${SOURCE_DIR}/game/ai_chat.c
	${SOURCE_DIR}/game/ai_chat.h
	${SOURCE_DIR}/game/ai_cmd.c
	${SOURCE_DIR}/game/ai_cmd.h
	${SOURCE_DIR}/game/ai_dmnet.c
	${SOURCE_DIR}/game/ai_dmnet.h
	${SOURCE_DIR}/game/ai_dmq3.c
	${SOURCE_DIR}/game/ai_dmq3.h
	${SOURCE_DIR}/game/ai_main.c
	${SOURCE_DIR}/game/ai_main.h
	${SOURCE_DIR}/game/ai_team.c
	${SOURCE_DIR}/game/ai_team.h
	${SOURCE_DIR}/game/ai_vcmd.c
	${SOURCE_DIR}/game/ai_vcmd.h
	${SOURCE_DIR}/game/bg_lib.c
	${SOURCE_DIR}/game/bg_local.h
	${SOURCE_DIR}/game/bg_misc.c
	${SOURCE_DIR}/game/bg_pmove.c
	${SOURCE_DIR}/game/bg_public.h
	${SOURCE_DIR}/game/bg_slidemove.c
	${SOURCE_DIR}/game/chars.h
	${SOURCE_DIR}/game/g_active.c
	${SOURCE_DIR}/game/g_arenas.c
	${SOURCE_DIR}/game/g_bot.c
	${SOURCE_DIR}/game/g_client.c
	${SOURCE_DIR}/game/g_cmds.c
	${SOURCE_DIR}/game/g_combat.c
	${SOURCE_DIR}/game/g_items.c
	${SOURCE_DIR}/game/g_local.h
	${SOURCE_DIR}/game/g_main.c
	${SOURCE_DIR}/game/g_mem.c
	${SOURCE_DIR}/game/g_misc.c
	${SOURCE_DIR}/game/g_missile.c
	${SOURCE_DIR}/game/g_mover.c
	${SOURCE_DIR}/game/g_public.h
	${SOURCE_DIR}/game/g_session.c
	${SOURCE_DIR}/game/g_spawn.c
	${SOURCE_DIR}/game/g_svcmds.c
	${SOURCE_DIR}/game/g_syscalls.c
	${SOURCE_DIR}/game/g_target.c
	${SOURCE_DIR}/game/g_team.c
	${SOURCE_DIR}/game/g_team.h
	${SOURCE_DIR}/game/g_trigger.c
	${SOURCE_DIR}/game/g_utils.c
	${SOURCE_DIR}/game/g_weapon.c
	${SOURCE_DIR}/game/inv.h
	${SOURCE_DIR}/game/match.h
	${SOURCE_DIR}/game/syn.h
	${SOURCE_DIR}/qcommon/q_math.c
	${SOURCE_DIR}/qcommon/q_shared.c
	${SOURCE_DIR}/qcommon/q_shared.h
	${SOURCE_DIR}/qcommon/surfaceflags.h
	${CMAKE_CURRENT_SOURCE_DIR}/ui/menudef.h
)

# Add the include directory
target_include_directories(${GAME_NAME} PUBLIC
${HEADER_DIRS}
)

# Set the library version
set_target_properties(${GAME_NAME} PROPERTIES 
	VERSION ${BUILD_VERSION}
	SOVERSION 1
	PREFIX ""
)
################################################################################################
# END OF GAME                                                                                  #
################################################################################################
# //////////////////////////////////////////////////////////////////////////////////////////// #
################################################################################################
# Create Q3_UI shared library                                                                  #
################################################################################################
set(Q3UI_NAME "ui${ARCH}")
add_library(${Q3UI_NAME} SHARED 
${SOURCE_DIR}/game/bg_misc.c
${SOURCE_DIR}/q3_ui/ui_addbots.c
${SOURCE_DIR}/q3_ui/ui_atoms.c
${SOURCE_DIR}/q3_ui/ui_cdkey.c
${SOURCE_DIR}/q3_ui/ui_cinematics.c
${SOURCE_DIR}/q3_ui/ui_confirm.c
${SOURCE_DIR}/q3_ui/ui_connect.c
${SOURCE_DIR}/q3_ui/ui_controls2.c
${SOURCE_DIR}/q3_ui/ui_credits.c
${SOURCE_DIR}/q3_ui/ui_demo2.c
${SOURCE_DIR}/q3_ui/ui_display.c
${SOURCE_DIR}/q3_ui/ui_gameinfo.c
${SOURCE_DIR}/q3_ui/ui_ingame.c
${SOURCE_DIR}/q3_ui/ui_loadconfig.c
${SOURCE_DIR}/q3_ui/ui_main.c
${SOURCE_DIR}/q3_ui/ui_menu.c
${SOURCE_DIR}/q3_ui/ui_mfield.c
${SOURCE_DIR}/q3_ui/ui_mods.c
${SOURCE_DIR}/q3_ui/ui_network.c
${SOURCE_DIR}/q3_ui/ui_options.c
${SOURCE_DIR}/q3_ui/ui_playermodel.c
${SOURCE_DIR}/q3_ui/ui_players.c
${SOURCE_DIR}/q3_ui/ui_playersettings.c
${SOURCE_DIR}/q3_ui/ui_preferences.c
${SOURCE_DIR}/q3_ui/ui_qmenu.c
${SOURCE_DIR}/q3_ui/ui_removebots.c
${SOURCE_DIR}/q3_ui/ui_saveconfig.c
${SOURCE_DIR}/q3_ui/ui_serverinfo.c
${SOURCE_DIR}/q3_ui/ui_servers2.c
${SOURCE_DIR}/q3_ui/ui_setup.c
${SOURCE_DIR}/q3_ui/ui_sound.c
${SOURCE_DIR}/q3_ui/ui_sparena.c
${SOURCE_DIR}/q3_ui/ui_specifyserver.c
${SOURCE_DIR}/q3_ui/ui_splevel.c
${SOURCE_DIR}/q3_ui/ui_sppostgame.c
${SOURCE_DIR}/q3_ui/ui_spreset.c
${SOURCE_DIR}/q3_ui/ui_spskill.c
${SOURCE_DIR}/q3_ui/ui_startserver.c
${SOURCE_DIR}/q3_ui/ui_team.c
${SOURCE_DIR}/q3_ui/ui_teamorders.c
${SOURCE_DIR}/q3_ui/ui_video.c
${SOURCE_DIR}/qcommon/q_math.c
${SOURCE_DIR}/qcommon/q_shared.c
${SOURCE_DIR}/qcommon/q_shared.h
${SOURCE_DIR}/ui/ui_syscalls.c
)

# Add the include directory
target_include_directories(${Q3UI_NAME} PUBLIC
${HEADER_DIRS}
)

# Set the library version
set_target_properties(${Q3UI_NAME} PROPERTIES 
	VERSION ${BUILD_VERSION}
	SOVERSION 1
	PREFIX ""
)
################################################################################################
# END OF GL2                                                                                   #
################################################################################################
# //////////////////////////////////////////////////////////////////////////////////////////// #
################################################################################################
# Create GL2 shared library                                                                    #
################################################################################################

# Print message confirming the task
message("Shaders are being stringified.")

execute_process(
	COMMAND python glsl_stringify.py
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Print message upon successful completion of the task
message("Shader files have been saved to ${shader_build_dir}.")

set(GL2_NAME "renderer_opengl2_${ARCH}")
add_library(${GL2_NAME} SHARED 
${CMAKE_BINARY_DIR}/dynamic/renderergl2/bokeh_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/bokeh_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/calclevels4x_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/calclevels4x_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/depthblur_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/depthblur_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/dlight_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/dlight_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/down4x_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/down4x_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/fogpass_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/fogpass_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/generic_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/generic_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/lightall_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/lightall_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/pshadow_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/pshadow_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/shadowfill_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/shadowfill_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/shadowmask_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/shadowmask_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/ssao_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/ssao_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/texturecolor_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/texturecolor_vp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/tonemap_fp.c
${CMAKE_BINARY_DIR}/dynamic/renderergl2/tonemap_vp.c
${SOURCE_DIR}/client/snd_local.h
${SOURCE_DIR}/client/snd_public.h
${SOURCE_DIR}/qcommon/puff.c
${SOURCE_DIR}/qcommon/puff.h
${SOURCE_DIR}/qcommon/q_math.c
${SOURCE_DIR}/qcommon/q_shared.c
${SOURCE_DIR}/qcommon/q_shared.h
${SOURCE_DIR}/qcommon/qcommon.h
${SOURCE_DIR}/qcommon/qfiles.h
${SOURCE_DIR}/qcommon/surfaceflags.h
#${SOURCE_DIR}/qcommon/unzip.h
${SOURCE_DIR}/qcommon/vm_local.h
${SOURCE_DIR}/renderercommon/qgl.h
${SOURCE_DIR}/renderercommon/tr_font.c
${SOURCE_DIR}/renderercommon/tr_image_bmp.c
${SOURCE_DIR}/renderercommon/tr_image_jpg.c
${SOURCE_DIR}/renderercommon/tr_image_pcx.c
${SOURCE_DIR}/renderercommon/tr_image_png.c
${SOURCE_DIR}/renderercommon/tr_image_tga.c
${SOURCE_DIR}/renderercommon/tr_noise.c
${SOURCE_DIR}/renderercommon/tr_public.h
${SOURCE_DIR}/renderergl2/tr_animation.c
${SOURCE_DIR}/renderergl2/tr_backend.c
${SOURCE_DIR}/renderergl2/tr_bsp.c
${SOURCE_DIR}/renderergl2/tr_cmds.c
${SOURCE_DIR}/renderergl2/tr_curve.c
${SOURCE_DIR}/renderergl2/tr_dsa.c
${SOURCE_DIR}/renderergl2/tr_extensions.c
${SOURCE_DIR}/renderergl2/tr_extramath.c
${SOURCE_DIR}/renderergl2/tr_extramath.h
${SOURCE_DIR}/renderergl2/tr_extratypes.h
${SOURCE_DIR}/renderergl2/tr_fbo.c
${SOURCE_DIR}/renderergl2/tr_fbo.h
${SOURCE_DIR}/renderergl2/tr_flares.c
${SOURCE_DIR}/renderergl2/tr_glsl.c
${SOURCE_DIR}/renderergl2/tr_image_dds.c
${SOURCE_DIR}/renderergl2/tr_image.c
${SOURCE_DIR}/renderergl2/tr_init.c
${SOURCE_DIR}/renderergl2/tr_light.c
${SOURCE_DIR}/renderergl2/tr_local.h
${SOURCE_DIR}/renderergl2/tr_main.c
${SOURCE_DIR}/renderergl2/tr_marks.c
${SOURCE_DIR}/renderergl2/tr_mesh.c
${SOURCE_DIR}/renderergl2/tr_model_iqm.c
${SOURCE_DIR}/renderergl2/tr_model.c
${SOURCE_DIR}/renderergl2/tr_postprocess.c
${SOURCE_DIR}/renderergl2/tr_postprocess.h
${SOURCE_DIR}/renderergl2/tr_scene.c
${SOURCE_DIR}/renderergl2/tr_shade_calc.c
${SOURCE_DIR}/renderergl2/tr_shade.c
${SOURCE_DIR}/renderergl2/tr_shader.c
${SOURCE_DIR}/renderergl2/tr_shadows.c
${SOURCE_DIR}/renderergl2/tr_sky.c
${SOURCE_DIR}/renderergl2/tr_subs.c
${SOURCE_DIR}/renderergl2/tr_surface.c
${SOURCE_DIR}/renderergl2/tr_vbo.c
${SOURCE_DIR}/renderergl2/tr_world.c
${SOURCE_DIR}/sdl/sdl_gamma.c
${SOURCE_DIR}/sdl/sdl_glimp.c
${SOURCE_DIR}/server/server.h
${SOURCE_DIR}/ui/ui_public.h
)

#Link the libraries to the targets
target_link_libraries(${GL2_NAME} PRIVATE ${LIBRARIES} jpeg SDL2 SDL2main)

#Add the include directory
target_include_directories(${GL2_NAME} PUBLIC
${HEADER_DIRS}
${JPEG_INCLUDE_DIR}
${SDL2_INCLUDE_DIR}
)

#Set the library version
set_target_properties(${GL2_NAME} PROPERTIES 
   VERSION ${BUILD_VERSION}
   SOVERSION 1
	PREFIX ""
)
################################################################################################
# END OF GL2                                                                                  #
################################################################################################

set(ASSEMBLY_FILES
	${SOURCE_DIR}/asm/ftola.c
	${SOURCE_DIR}/asm/snapvector.c
)

# Configure the header file
configure_file(${SOURCE_DIR}/resource/version.h.in ${SOURCE_DIR}/version.h @ONLY)

# Set the icon file for the target
set(ICON ${SOURCE_DIR}/resource/icon.rc)

set(IOQ3_SRC
${ASSEMBLY_FILES}
${SOURCE_DIR}/botlib/aasfile.h
${SOURCE_DIR}/botlib/be_aas_bsp.h
${SOURCE_DIR}/botlib/be_aas_bspq3.c
${SOURCE_DIR}/botlib/be_aas_cluster.c
${SOURCE_DIR}/botlib/be_aas_cluster.h
${SOURCE_DIR}/botlib/be_aas_debug.c
${SOURCE_DIR}/botlib/be_aas_debug.h
${SOURCE_DIR}/botlib/be_aas_def.h
${SOURCE_DIR}/botlib/be_aas_entity.c
${SOURCE_DIR}/botlib/be_aas_entity.h
${SOURCE_DIR}/botlib/be_aas_main.c
${SOURCE_DIR}/botlib/be_aas_main.h
${SOURCE_DIR}/botlib/be_aas_file.c
${SOURCE_DIR}/botlib/be_aas_file.h
${SOURCE_DIR}/botlib/be_aas_funcs.h
${SOURCE_DIR}/botlib/be_aas_move.c
${SOURCE_DIR}/botlib/be_aas_move.h
${SOURCE_DIR}/botlib/be_aas_optimize.c
${SOURCE_DIR}/botlib/be_aas_optimize.h
${SOURCE_DIR}/botlib/be_aas_reach.c
${SOURCE_DIR}/botlib/be_aas_reach.h
${SOURCE_DIR}/botlib/be_aas_route.c
${SOURCE_DIR}/botlib/be_aas_route.h
${SOURCE_DIR}/botlib/be_aas_routealt.c
${SOURCE_DIR}/botlib/be_aas_routealt.h
${SOURCE_DIR}/botlib/be_aas_sample.c
${SOURCE_DIR}/botlib/be_aas_sample.h
${SOURCE_DIR}/botlib/be_ai_char.c
${SOURCE_DIR}/botlib/be_ai_chat.c
${SOURCE_DIR}/botlib/be_ai_gen.c
${SOURCE_DIR}/botlib/be_ai_goal.c
${SOURCE_DIR}/botlib/be_ai_move.c
${SOURCE_DIR}/botlib/be_ai_weap.c
${SOURCE_DIR}/botlib/be_ai_weight.c
${SOURCE_DIR}/botlib/be_ai_weight.h
${SOURCE_DIR}/botlib/be_ea.c
${SOURCE_DIR}/botlib/be_interface.c
${SOURCE_DIR}/botlib/be_interface.h
${SOURCE_DIR}/botlib/l_crc.c
${SOURCE_DIR}/botlib/l_crc.h
${SOURCE_DIR}/botlib/l_libvar.c
${SOURCE_DIR}/botlib/l_libvar.h
${SOURCE_DIR}/botlib/l_log.c
${SOURCE_DIR}/botlib/l_log.h
${SOURCE_DIR}/botlib/l_memory.c
${SOURCE_DIR}/botlib/l_memory.h
${SOURCE_DIR}/botlib/l_precomp.c
${SOURCE_DIR}/botlib/l_precomp.h
${SOURCE_DIR}/botlib/l_script.c
${SOURCE_DIR}/botlib/l_script.h
${SOURCE_DIR}/botlib/l_struct.c
${SOURCE_DIR}/botlib/l_struct.h
${SOURCE_DIR}/botlib/l_utils.h
${SOURCE_DIR}/cgame/cg_public.h
${SOURCE_DIR}/client/cl_avi.c
${SOURCE_DIR}/client/cl_cgame.c
${SOURCE_DIR}/client/cl_cin.c
${SOURCE_DIR}/client/cl_console.c
${SOURCE_DIR}/client/cl_curl.c
${SOURCE_DIR}/client/cl_curl.h
${SOURCE_DIR}/client/cl_input.c
${SOURCE_DIR}/client/cl_keys.c
${SOURCE_DIR}/client/cl_main.c
${SOURCE_DIR}/client/cl_net_chan.c
${SOURCE_DIR}/client/cl_parse.c
${SOURCE_DIR}/client/cl_scrn.c
${SOURCE_DIR}/client/cl_ui.c
${SOURCE_DIR}/client/client.h
${SOURCE_DIR}/client/keycodes.h
${SOURCE_DIR}/client/keys.h
${SOURCE_DIR}/client/qal.c
${SOURCE_DIR}/client/qal.h
${SOURCE_DIR}/client/snd_adpcm.c
${SOURCE_DIR}/client/snd_codec_ogg.c
${SOURCE_DIR}/client/snd_codec_wav.c
${SOURCE_DIR}/client/snd_codec.c
${SOURCE_DIR}/client/snd_dma.c
${SOURCE_DIR}/client/snd_local.h
${SOURCE_DIR}/client/snd_main.c
${SOURCE_DIR}/client/snd_mem.c
${SOURCE_DIR}/client/snd_mix.c
${SOURCE_DIR}/client/snd_openal.c
${SOURCE_DIR}/client/snd_public.h
${SOURCE_DIR}/client/snd_wavelet.c
${SOURCE_DIR}/game/bg_public.h
${SOURCE_DIR}/game/g_public.h
${SOURCE_DIR}/qcommon/cm_load.c
${SOURCE_DIR}/qcommon/cm_local.h
${SOURCE_DIR}/qcommon/cm_patch.c
${SOURCE_DIR}/qcommon/cm_patch.h
${SOURCE_DIR}/qcommon/cm_polylib.c
${SOURCE_DIR}/qcommon/cm_polylib.h
${SOURCE_DIR}/qcommon/cm_public.h
${SOURCE_DIR}/qcommon/cm_test.c
${SOURCE_DIR}/qcommon/cm_trace.c
${SOURCE_DIR}/qcommon/cmd.c
${SOURCE_DIR}/qcommon/common.c
${SOURCE_DIR}/qcommon/cvar.c
${SOURCE_DIR}/qcommon/files.c
${SOURCE_DIR}/qcommon/huffman.c
#${SOURCE_DIR}/qcommon/ioapi.c
${SOURCE_DIR}/qcommon/md4.c
${SOURCE_DIR}/qcommon/md5.c
${SOURCE_DIR}/qcommon/msg.c
${SOURCE_DIR}/qcommon/net_chan.c
${SOURCE_DIR}/qcommon/net_ip.c
${SOURCE_DIR}/qcommon/puff.c
${SOURCE_DIR}/qcommon/puff.h
${SOURCE_DIR}/qcommon/q_math.c
${SOURCE_DIR}/qcommon/q_shared.c
${SOURCE_DIR}/qcommon/q_shared.h
${SOURCE_DIR}/qcommon/qcommon.h
${SOURCE_DIR}/qcommon/qfiles.h
${SOURCE_DIR}/qcommon/surfaceflags.h
#${SOURCE_DIR}/qcommon/unzip.c
#${SOURCE_DIR}/qcommon/unzip.h
${SOURCE_DIR}/qcommon/vm_interpreted.c
${SOURCE_DIR}/qcommon/vm_local.h
${SOURCE_DIR}/qcommon/vm_x86.c
${SOURCE_DIR}/qcommon/vm.c
${SOURCE_DIR}/renderercommon/qgl.h
${SOURCE_DIR}/renderercommon/tr_public.h
${SOURCE_DIR}/renderercommon/tr_types.h
${SOURCE_DIR}/sdl/sdl_input.c
${SOURCE_DIR}/sdl/sdl_snd.c
${SOURCE_DIR}/server/server.h
${SOURCE_DIR}/server/sv_bot.c
${SOURCE_DIR}/server/sv_ccmds.c
${SOURCE_DIR}/server/sv_client.c
${SOURCE_DIR}/server/sv_game.c
${SOURCE_DIR}/server/sv_init.c
${SOURCE_DIR}/server/sv_main.c
${SOURCE_DIR}/server/sv_net_chan.c
${SOURCE_DIR}/server/sv_snapshot.c
${SOURCE_DIR}/server/sv_world.c
${SOURCE_DIR}/sys/con_log.c
${SOURCE_DIR}/sys/con_passive.c
${SOURCE_DIR}/sys/sys_autoupdater.c
${SOURCE_DIR}/sys/sys_main.c
${SOURCE_DIR}/sys/sys_win32.c
${SOURCE_DIR}/sys/win_resource.h
${SOURCE_DIR}/ui/ui_public.h
)

# Set the output directory for the binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/app")

# Add the targets
add_executable(${PROJECT_NAME}${ARCH} ${IOQ3_SRC} ${ICON})
set_target_properties(${PROJECT_NAME}${ARCH} PROPERTIES LINK_FLAGS "-Wl,/SUBSYSTEM:WINDOWS -Wl,/ENTRY:mainCRTStartup")

set(CMAKE_PROFILE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/profiling)

if(ENABLE_PROFILING)
	message("PROFILING ON")
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		target_compile_options(${PROJECT_NAME}${ARCH} PRIVATE "-fprofile-generate")
	elseif(CMAKE_BUILD_TYPE MATCHES Release)
		target_compile_options(${PROJECT_NAME}${ARCH} PRIVATE "-fprofile-use=${CMAKE_PROFILE_OUTPUT_DIR}/default.profdata")
	endif()

	if(ENABLE_PROFILING)
		add_custom_command(TARGET ${PROJECT_NAME}${ARCH}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/profiling/default.profraw $<TARGET_FILE:${PROJECT_NAME}${ARCH}> -skip-functions=__tmainCRTStartup,_WinMainCRTStartup,BaseThreadInitThunk,RtlUserThreadStart
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/app"
		COMMENT "Running ${PROJECT_NAME}${ARCH} for profiling"
		VERBATIM
	)
	endif()
else()
	message("PROFILING OFF")
endif()

target_compile_definitions(${PROJECT_NAME}${ARCH} PRIVATE idx64 BOTLIB HAVE_LRINTF FLOATING_POINT FLOAT_APPROX USE_CURL USE_CURL_DLOPEN CURL_STATICLIB USE_OPENAL USE_OPENAL_DLOPEN USE_VOIP OPUS_BUILD USE_ALLOCA)

#Link LLVM profiler
target_link_libraries(${PROJECT_NAME}${ARCH} PRIVATE "-fprofile-instr-generate=${CMAKE_BINARY_DIR}/profiling/default.profraw" "-fprofile-instr-use=${CMAKE_BINARY_DIR}/profiling/default.profdata" "-fprofile-instr-use=libclang_rt.profile-x86_64.a")
# Link the libraries to the target
target_link_libraries(${PROJECT_NAME}${ARCH} PRIVATE
shell32 user32 advapi32 winmm wsock32 ws2_32 psapi gdi32 opengl32 crypt32 nghttp2 idn2 psl ssh2 ssl crypto gdi32 wldap32 zstd brotlidec z
ZLIB::ZLIB
OpenAL::OpenAL
opus
opusfile
#SDL2::SDL2-static
SDL2
SDL2main
curl
ogg
vorbis
minizip
${JPEG_LIBRARIES}
png16
renderer_opengl2_x64
)

# Add the include directory
target_include_directories(${PROJECT_NAME}${ARCH} PUBLIC
${HEADER_DIRS}
${JPEG_INCLUDE_DIR}
${SDL2_INCLUDE_DIR}
)

# Copy DLL files to build output folder
if (WIN32)
	# Create app and app/baseq3 directories if they don't exist
	add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/app/baseq3")

	# Copy SDL2 DLL files from MSYS2
	add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${DLL_DIR}/libopenal-1.dll"
		$<TARGET_FILE:${PROJECT_NAME}${ARCH}> "${CMAKE_CURRENT_SOURCE_DIR}/app/")

	# Rename libopenal-1.dll to oal_soft.dll
	add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${DLL_DIR}/libopenal-1.dll" "${CMAKE_CURRENT_SOURCE_DIR}/app/oal_soft.dll"
		COMMENT "Renaming libopenal-1.dll to oal_soft.dll"
		VERBATIM)

	# Copy executable to app directory
	add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}${ARCH}> "${CMAKE_CURRENT_SOURCE_DIR}/app/${PROJECT_NAME}${ARCH}.exe")

	# Copy other DLLs to app/baseq3 directory if missing
	set(DLLS_TO_COPY
		${CGAME_NAME}
		${GAME_NAME}
		${Q3UI_NAME}
	)

	foreach(DLL ${DLLS_TO_COPY})
		add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${DLL}> "${CMAKE_CURRENT_SOURCE_DIR}/app/baseq3/"
			COMMENT "Copying ${DLL} to app/baseq3 directory"
			VERBATIM
		)
	endforeach()

	# Copy GL2_NAME DLL to app directory
   add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
	   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${GL2_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/app/"
	   COMMENT "Copying ${GL2_NAME} to app directory"
	   VERBATIM
   )

	# Remove file_to_remove.txt from the app folder after build
	add_custom_command(TARGET ${PROJECT_NAME}${ARCH} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_SOURCE_DIR}/app/libopenal-1.dll"
	COMMENT "Removing libopenal-1.dll from app directory after build"
	VERBATIM
	)
endif()

# Set the working directory for debugging
set_property(TARGET ${PROJECT_NAME}${ARCH} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/app")

