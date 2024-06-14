# The MIT License (MIT)
# 
# Copyright (c) 2022 github.com/Pan7
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
cmake_minimum_required(VERSION 2.8.12)

project("basegame game qvm")

set(BASE_GAME_QVM_TARGET "basegame_game_qvm" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")

option(VQ3_COMPATIBLE_QVM "Produce a qvm file compatible with Q3 1.32b" ${DEFAULT_VQ3_COMPATIBLE_QVM})

set(BASEGAME_QVM_INSTALL_DIR "${DEFAULT_BASEGAME_QVM_INSTALL_DIR}" CACHE PATH "Installation directory")

set(BOTLIB_HEADERS
	"${BOTLIB_DIR}/aasfile.h"
	"${BOTLIB_DIR}/be_aas.h"
	"${BOTLIB_DIR}/be_aas_bsp.h"
	"${BOTLIB_DIR}/be_aas_cluster.h"
	"${BOTLIB_DIR}/be_aas_debug.h"
	"${BOTLIB_DIR}/be_aas_def.h"
	"${BOTLIB_DIR}/be_aas_entity.h"
	"${BOTLIB_DIR}/be_aas_file.h"
	"${BOTLIB_DIR}/be_aas_funcs.h"
	"${BOTLIB_DIR}/be_aas_main.h"
	"${BOTLIB_DIR}/be_aas_move.h"
	"${BOTLIB_DIR}/be_aas_optimize.h"
	"${BOTLIB_DIR}/be_aas_reach.h"
	"${BOTLIB_DIR}/be_aas_route.h"
	"${BOTLIB_DIR}/be_aas_routealt.h"
	"${BOTLIB_DIR}/be_aas_sample.h"
	"${BOTLIB_DIR}/be_ai_char.h"
	"${BOTLIB_DIR}/be_ai_chat.h"
	"${BOTLIB_DIR}/be_ai_gen.h"
	"${BOTLIB_DIR}/be_ai_goal.h"
	"${BOTLIB_DIR}/be_ai_move.h"
	"${BOTLIB_DIR}/be_ai_weap.h"
	"${BOTLIB_DIR}/be_ai_weight.h"
	"${BOTLIB_DIR}/be_ea.h"
	"${BOTLIB_DIR}/be_interface.h"
	"${BOTLIB_DIR}/botlib.h"
	"${BOTLIB_DIR}/l_crc.h"
	"${BOTLIB_DIR}/l_libvar.h"
	"${BOTLIB_DIR}/l_log.h"
	"${BOTLIB_DIR}/l_memory.h"
	"${BOTLIB_DIR}/l_precomp.h"
	"${BOTLIB_DIR}/l_script.h"
	"${BOTLIB_DIR}/l_struct.h"
	"${BOTLIB_DIR}/l_utils.h"
)
set(GAME_SOURCES
	"${GAME_DIR}/g_main.c"

	"${GAME_DIR}/ai_chat.c"
	"${GAME_DIR}/ai_cmd.c"
	"${GAME_DIR}/ai_dmnet.c"
	"${GAME_DIR}/ai_dmq3.c"
	"${GAME_DIR}/ai_main.c"
	"${GAME_DIR}/ai_team.c"
	"${GAME_DIR}/ai_vcmd.c"

	"${GAME_DIR}/bg_misc.c"
	"${GAME_DIR}/bg_pmove.c"
	"${GAME_DIR}/bg_slidemove.c"
	"${GAME_DIR}/bg_lib.c"

	"${GAME_DIR}/g_active.c"
	"${GAME_DIR}/g_arenas.c"
	"${GAME_DIR}/g_bot.c"
	"${GAME_DIR}/g_client.c"
	"${GAME_DIR}/g_cmds.c"
	"${GAME_DIR}/g_combat.c"
	"${GAME_DIR}/g_items.c"

	"${GAME_DIR}/g_mem.c"
	"${GAME_DIR}/g_misc.c"
	"${GAME_DIR}/g_missile.c"
	"${GAME_DIR}/g_mover.c"
	"${GAME_DIR}/g_session.c"
	"${GAME_DIR}/g_spawn.c"
	"${GAME_DIR}/g_svcmds.c"
	"${GAME_DIR}/g_target.c"
	"${GAME_DIR}/g_team.c"
	"${GAME_DIR}/g_trigger.c"
	"${GAME_DIR}/g_utils.c"
	"${GAME_DIR}/g_weapon.c"

#	"${GAME_DIR}/g_syscalls.c"
)
set(GAME_ASM_SOURCES
	"${GAME_DIR}/g_syscalls.asm"
)
set(GAME_HEADERS
	"${GAME_DIR}/ai_chat.h"
	"${GAME_DIR}/ai_cmd.h"
	"${GAME_DIR}/ai_dmnet.h"
	"${GAME_DIR}/ai_dmq3.h"
	"${GAME_DIR}/ai_main.h"
	"${GAME_DIR}/ai_team.h"
	"${GAME_DIR}/ai_vcmd.h"
	"${GAME_DIR}/chars.h"
	"${GAME_DIR}/inv.h"
	"${GAME_DIR}/match.h"
	"${GAME_DIR}/syn.h"

	"${GAME_DIR}/bg_local.h"
	"${GAME_DIR}/bg_public.h"
	"${GAME_DIR}/bg_lib.h"

	"${GAME_DIR}/g_local.h"
	"${GAME_DIR}/g_public.h"

	"${GAME_DIR}/g_team.h"

)
set(QCOMMON_SOURCES
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
)
set(QCOMMON_HEADERS
	"${QCOMMON_DIR}/q_shared.h"
)

list(APPEND GAME_DEFINITIONS "-DQAGAME")
list(APPEND GAME_DEFINITIONS "-DQ3_VM")

#todo use add_custom_command with VERBATIM to get quotation marks working?
#-DPRODUCT_DATE="Nov 28 2021" still creates a q3lcc error cause of the blank char
if(FALSE AND DEFINED ENV{SOURCE_DATE_EPOCH})
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.0")
		string(TIMESTAMP PRODUCT_DATE "%b %d %Y" UTC) #Nov 28 2021
		list(APPEND GAME_DEFINITIONS "-DPRODUCT_DATE=\"${PRODUCT_DATE}\"")
	else()
		message(WARNING "Update CMake to version 3.8 or later for SOURCE_DATE_EPOCH support!")
	endif()
endif()

list(APPEND GAME_C_SOURCES ${GAME_SOURCES} ${QCOMMON_SOURCES})
list(APPEND GAME_C_HEADERS ${GAME_HEADERS} ${QCOMMON_HEADERS} ${BOTLIB_HEADERS})

source_group("botlib\\Header Files" FILES ${BOTLIB_HEADERS})
source_group("game\\Source Files" FILES ${GAME_SOURCES})
source_group("game\\Header Files" FILES ${GAME_HEADERS})
source_group("game" FILES ${GAME_ASM_SOURCES})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})

set(QVM_FILENAME "qagame.qvm")
set(QVM_FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/basegame/vm/${QVM_FILENAME}")

set(ASM_BDIR "${CMAKE_CURRENT_BINARY_DIR}/asm")
file(MAKE_DIRECTORY "${ASM_BDIR}")

set(GAME_ASM_FILES)

set(Q3CPP_VERBOSE_OPTION $<$<CONFIG:Debug>:-v>)
set(Q3LCC_VERBOSE_OPTION $<$<CONFIG:Debug>:-v>)

foreach(SOURCE ${GAME_C_SOURCES})
	get_filename_component(BASE_FILENAME "${SOURCE}" NAME_WE)
	set(ASM_FILENAME "${ASM_BDIR}/${BASE_FILENAME}.asm")

	add_custom_command(OUTPUT "${ASM_FILENAME}"
		COMMAND "$<TARGET_FILE:${Q3LCC_TARGET}>" ARGS ${Q3LCC_OPTIIONS} ${Q3LCC_VERBOSE_OPTION} ${GAME_DEFINITIONS} -o "${ASM_FILENAME}" "${SOURCE}" -Wo-lrccdir=$<TARGET_FILE_DIR:${Q3RCC_TARGET}> -Wo-lcppdir=$<TARGET_FILE_DIR:${Q3CPP_TARGET}> -I${BOTLIB_DIR} -I${GAME_DIR} -I${QCOMMON_DIR}
		DEPENDS ${Q3LCC_TARGET} ${Q3RCC_TARGET} ${Q3CPP_TARGET} ${SOURCE}
	)
	list(APPEND GAME_ASM_FILES "${ASM_FILENAME}")
	set_source_files_properties("${SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

foreach(SOURCE ${GAME_ASM_SOURCES})
	set_source_files_properties("${SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

set(SOURCES ${GAME_ASM_FILES} ${GAME_ASM_SOURCES})

if(VQ3_COMPATIBLE_QVM)
	set(VQ3_OPTION "-vq3")
endif(VQ3_COMPATIBLE_QVM)

add_custom_command(OUTPUT "${QVM_FILE}"
	COMMAND "$<TARGET_FILE:${Q3ASM_TARGET}>" ${Q3CPP_OPTIIONS} ${Q3CPP_VERBOSE_OPTION} ${VQ3_OPTION} -o "${QVM_FILE}" ${SOURCES}
	DEPENDS ${Q3ASM_TARGET} ${SOURCES}
)

add_custom_target(${BASE_GAME_QVM_TARGET} ALL
	DEPENDS "${QVM_FILE}"
	SOURCES ${GAME_C_SOURCES} ${GAME_C_HEADERS}
)
set_target_properties(${BASE_GAME_QVM_TARGET} PROPERTIES QVM_FILENAME "${QVM_FILENAME}")

set(DESTIONATION_DIR)
if(BASEGAME_QVM_INSTALL_DIR)
	set(DESTIONATION_DIR "${BASEGAME_QVM_INSTALL_DIR}")
endif(BASEGAME_QVM_INSTALL_DIR)

if(DESTIONATION_DIR)
	install(FILES "${QVM_FILE}"
		DESTINATION "${DESTIONATION_DIR}"
		CONFIGURATIONS Release
		COMPONENT basegame_qvm
	)
endif(DESTIONATION_DIR)

include(CPack)
