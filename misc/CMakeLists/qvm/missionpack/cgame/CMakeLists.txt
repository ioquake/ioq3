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

project("missionpack cgame qvm")

set(MISSION_CGAME_QVM_TARGET "missionpack_cgame_qvm" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")

option(VQ3_COMPATIBLE_QVM "Produce a qvm file compatible with Q3 1.32b" ${DEFAULT_VQ3_COMPATIBLE_QVM})

set(MISSIONPACK_QVM_INSTALL_DIR "${DEFAULT_MISSIONPACK_QVM_INSTALL_DIR}" CACHE PATH "Installation directory")

set(CGAME_SOURCES
	"${CGAME_DIR}/cg_main.c"

	"${CGAME_DIR}/cg_consolecmds.c"
	"${CGAME_DIR}/cg_draw.c"
	"${CGAME_DIR}/cg_drawtools.c"
	"${CGAME_DIR}/cg_effects.c"
	"${CGAME_DIR}/cg_ents.c"
	"${CGAME_DIR}/cg_event.c"
	"${CGAME_DIR}/cg_info.c"
	"${CGAME_DIR}/cg_localents.c"

	"${CGAME_DIR}/cg_marks.c"
	"${CGAME_DIR}/cg_newdraw.c"
	"${CGAME_DIR}/cg_particles.c"
	"${CGAME_DIR}/cg_players.c"
	"${CGAME_DIR}/cg_playerstate.c"
	"${CGAME_DIR}/cg_predict.c"
	"${CGAME_DIR}/cg_scoreboard.c"
	"${CGAME_DIR}/cg_servercmds.c"
	"${CGAME_DIR}/cg_snapshot.c"
	"${CGAME_DIR}/cg_view.c"
	"${CGAME_DIR}/cg_weapons.c"

#	"${CGAME_DIR}/cg_syscalls.c"
)
set(CGAME_ASM_SOURCES
	"${CGAME_DIR}/cg_syscalls.asm"
)
set(CGAME_HEADERS
	"${CGAME_DIR}/cg_local.h"
	"${CGAME_DIR}/cg_public.h"
)
set(GAME_SOURCES
	"${GAME_DIR}/bg_misc.c"
	"${GAME_DIR}/bg_pmove.c"
	"${GAME_DIR}/bg_slidemove.c"
	"${GAME_DIR}/bg_lib.c"
)
set(GAME_HEADERS
	"${GAME_DIR}/bg_local.h"
	"${GAME_DIR}/bg_public.h"
	"${GAME_DIR}/bg_lib.h"
)
set(UI_SOURCES
	"${UI_DIR}/ui_shared.c"
)
set(UI_HEADERS
	"${UI_DIR}/ui_shared.h"
)
set(QCOMMON_SOURCES
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
)
set(QCOMMON_HEADERS
	"${QCOMMON_DIR}/q_shared.h"
)

list(APPEND CGAME_DEFINITIONS "-DCGAME")
list(APPEND CGAME_DEFINITIONS "-DQ3_VM")
list(APPEND CGAME_DEFINITIONS "-DMISSIONPACK")

list(APPEND CGAME_C_SOURCES ${CGAME_SOURCES} ${GAME_SOURCES} ${UI_SOURCES} ${QCOMMON_SOURCES})
list(APPEND CGAME_C_HEADERS ${CGAME_HEADERS} ${GAME_HEADERS} ${UI_HEADERS} ${QCOMMON_HEADERS})

source_group("cgame\\Source Files" FILES ${CGAME_SOURCES})
source_group("cgame\\Header Files" FILES ${CGAME_HEADERS})
source_group("cgame" FILES ${CGAME_ASM_SOURCES})
source_group("game\\Source Files" FILES ${GAME_SOURCES})
source_group("game\\Header Files" FILES ${GAME_HEADERS})
source_group("ui\\Source Files" FILES ${UI_SOURCES})
source_group("ui\\Header Files" FILES ${UI_HEADERS})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})

set(QVM_FILENAME "cgame.qvm")
set(QVM_FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/missionpack/vm/${QVM_FILENAME}")

set(ASM_BDIR "${CMAKE_CURRENT_BINARY_DIR}/asm")
file(MAKE_DIRECTORY "${ASM_BDIR}")

set(CGAME_ASM_FILES)

set(Q3CPP_VERBOSE_OPTION $<$<CONFIG:Debug>:-v>)
set(Q3LCC_VERBOSE_OPTION $<$<CONFIG:Debug>:-v>)

foreach(SOURCE ${CGAME_C_SOURCES})
	get_filename_component(BASE_FILENAME "${SOURCE}" NAME_WE)
	set(ASM_FILENAME "${ASM_BDIR}/${BASE_FILENAME}.asm")

	add_custom_command(OUTPUT "${ASM_FILENAME}"
		COMMAND "$<TARGET_FILE:${Q3LCC_TARGET}>" ARGS ${Q3LCC_OPTIIONS} ${Q3LCC_VERBOSE_OPTION} ${CGAME_DEFINITIONS} -o "${ASM_FILENAME}" "${SOURCE}" -Wo-lrccdir=$<TARGET_FILE_DIR:${Q3RCC_TARGET}> -Wo-lcppdir=$<TARGET_FILE_DIR:${Q3CPP_TARGET}> -I${CGAME_DIR} -I${GAME_DIR} -I${QCOMMON_DIR}
		DEPENDS ${Q3LCC_TARGET} ${Q3RCC_TARGET} ${Q3CPP_TARGET} ${SOURCE}
	)
	list(APPEND CGAME_ASM_FILES "${ASM_FILENAME}")
	set_source_files_properties("${SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

foreach(SOURCE ${CGAME_ASM_SOURCES})
	set_source_files_properties("${SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

set(SOURCES ${CGAME_ASM_FILES} ${CGAME_ASM_SOURCES})

if(VQ3_COMPATIBLE_QVM)
	set(VQ3_OPTION "-vq3")
endif(VQ3_COMPATIBLE_QVM)

add_custom_command(OUTPUT "${QVM_FILE}"
	COMMAND "$<TARGET_FILE:${Q3ASM_TARGET}>" ${Q3CPP_OPTIIONS} ${Q3CPP_VERBOSE_OPTION} ${VQ3_OPTION} -o "${QVM_FILE}" ${SOURCES}
	DEPENDS ${Q3ASM_TARGET} ${SOURCES}
)

add_custom_target(${MISSION_CGAME_QVM_TARGET} ALL
	DEPENDS "${QVM_FILE}"
	SOURCES ${CGAME_C_SOURCES} ${CGAME_C_HEADERS}
)
set_target_properties(${MISSION_CGAME_QVM_TARGET} PROPERTIES QVM_FILENAME "${QVM_FILENAME}")

set(DESTIONATION_DIR)
if(MISSIONPACK_QVM_INSTALL_DIR)
	set(DESTIONATION_DIR "${MISSIONPACK_QVM_INSTALL_DIR}")
endif(MISSIONPACK_QVM_INSTALL_DIR)

if(DESTIONATION_DIR)
	install(FILES "${QVM_FILE}"
		DESTINATION "${DESTIONATION_DIR}"
		CONFIGURATIONS Release
		COMPONENT missionpack_qvm
	)
endif(DESTIONATION_DIR)

include(CPack)
