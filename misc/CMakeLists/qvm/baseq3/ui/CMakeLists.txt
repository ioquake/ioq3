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

project("basegame ui qvm")

set(BASE_UI_QVM_TARGET "basegame_ui_qvm" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")

option(VQ3_COMPATIBLE_QVM "Produce a qvm file compatible with Q3 1.32b" ${DEFAULT_VQ3_COMPATIBLE_QVM})

set(BASEGAME_QVM_INSTALL_DIR "${DEFAULT_BASEGAME_QVM_INSTALL_DIR}" CACHE PATH "Installation directory")

set(GAME_SOURCES
	"${GAME_DIR}/bg_misc.c"
	"${GAME_DIR}/bg_lib.c"
)
set(GAME_HEADERS
	"${GAME_DIR}/bg_local.h"
	"${GAME_DIR}/bg_public.h"
	"${GAME_DIR}/bg_lib.h"
)
set(Q3_UI_SOURCES
	"${Q3_UI_DIR}/ui_main.c"

	"${Q3_UI_DIR}/ui_addbots.c"
	"${Q3_UI_DIR}/ui_atoms.c"
	"${Q3_UI_DIR}/ui_cdkey.c"
	"${Q3_UI_DIR}/ui_cinematics.c"
	"${Q3_UI_DIR}/ui_confirm.c"
	"${Q3_UI_DIR}/ui_connect.c"
	"${Q3_UI_DIR}/ui_controls2.c"
	"${Q3_UI_DIR}/ui_credits.c"
	"${Q3_UI_DIR}/ui_demo2.c"
	"${Q3_UI_DIR}/ui_display.c"
	"${Q3_UI_DIR}/ui_gameinfo.c"
	"${Q3_UI_DIR}/ui_ingame.c"
	"${Q3_UI_DIR}/ui_loadconfig.c"

	"${Q3_UI_DIR}/ui_menu.c"
	"${Q3_UI_DIR}/ui_mfield.c"
	"${Q3_UI_DIR}/ui_mods.c"
	"${Q3_UI_DIR}/ui_network.c"
	"${Q3_UI_DIR}/ui_options.c"
	"${Q3_UI_DIR}/ui_playermodel.c"
	"${Q3_UI_DIR}/ui_players.c"
	"${Q3_UI_DIR}/ui_playersettings.c"
	"${Q3_UI_DIR}/ui_preferences.c"
	"${Q3_UI_DIR}/ui_qmenu.c"
	"${Q3_UI_DIR}/ui_removebots.c"
	"${Q3_UI_DIR}/ui_saveconfig.c"
	"${Q3_UI_DIR}/ui_serverinfo.c"
	"${Q3_UI_DIR}/ui_servers2.c"
	"${Q3_UI_DIR}/ui_setup.c"
	"${Q3_UI_DIR}/ui_sound.c"
	"${Q3_UI_DIR}/ui_sparena.c"
	"${Q3_UI_DIR}/ui_specifyserver.c"
	"${Q3_UI_DIR}/ui_splevel.c"
	"${Q3_UI_DIR}/ui_sppostgame.c"
	"${Q3_UI_DIR}/ui_spskill.c"
	"${Q3_UI_DIR}/ui_startserver.c"
	"${Q3_UI_DIR}/ui_team.c"
	"${Q3_UI_DIR}/ui_teamorders.c"
	"${Q3_UI_DIR}/ui_video.c"

)
set(UI_ASM_SOURCES
	"${UI_DIR}/ui_syscalls.asm"
)

set(Q3_UI_HEADERS
	"${Q3_UI_DIR}/ui_local.h"

)
set(QCOMMON_SOURCES
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
)
set(QCOMMON_HEADERS
	"${QCOMMON_DIR}/q_shared.h"
)
set(UI_SOURCES

)
set(UI_HEADERS

)

list(APPEND UI_DEFINITIONS "-DUI")
list(APPEND UI_DEFINITIONS "-DQ3_VM")

list(APPEND UI_C_SOURCES ${Q3_UI_SOURCES} ${UI_SOURCES} ${GAME_SOURCES} ${QCOMMON_SOURCES})
list(APPEND UI_C_HEADERS ${Q3_UI_HEADERS} ${UI_HEADERS} ${GAME_HEADERS} ${QCOMMON_HEADERS})

source_group("game\\Source Files" FILES ${GAME_SOURCES})
source_group("game\\Header Files" FILES ${GAME_HEADERS})
source_group("q3_ui\\Source Files" FILES ${Q3_UI_SOURCES})
source_group("q3_ui\\Header Files" FILES ${Q3_UI_HEADERS})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})
source_group("ui\\Source Files" FILES ${UI_SOURCES})
source_group("ui\\Header Files" FILES ${UI_HEADERS})
source_group("ui" FILES ${UI_ASM_SOURCES})

set(QVM_FILENAME "ui.qvm")
set(QVM_FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/basegame/vm/${QVM_FILENAME}")

set(ASM_BDIR "${CMAKE_CURRENT_BINARY_DIR}/asm")
file(MAKE_DIRECTORY "${ASM_BDIR}")

set(UI_ASM_FILES)

set(Q3CPP_VERBOSE_OPTION $<$<CONFIG:Debug>:-v>)
set(Q3LCC_VERBOSE_OPTION $<$<CONFIG:Debug>:-v>)

foreach(SOURCE ${UI_C_SOURCES})
	get_filename_component(BASE_FILENAME "${SOURCE}" NAME_WE)
	set(ASM_FILENAME "${ASM_BDIR}/${BASE_FILENAME}.asm")

	add_custom_command(OUTPUT "${ASM_FILENAME}"
		COMMAND "$<TARGET_FILE:${Q3LCC_TARGET}>" ARGS ${Q3LCC_OPTIIONS} ${Q3LCC_VERBOSE_OPTION} ${UI_DEFINITIONS} -o "${ASM_FILENAME}" "${SOURCE}" -Wo-lrccdir=$<TARGET_FILE_DIR:${Q3RCC_TARGET}> -Wo-lcppdir=$<TARGET_FILE_DIR:${Q3CPP_TARGET}> -I${UI_DIR} -I${GAME_DIR} -I${QCOMMON_DIR}
		DEPENDS ${Q3LCC_TARGET} ${Q3RCC_TARGET} ${Q3CPP_TARGET} ${SOURCE}
	)
	list(APPEND UI_ASM_FILES "${ASM_FILENAME}")
	set_source_files_properties("${SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

foreach(SOURCE ${UI_ASM_SOURCES})
	set_source_files_properties("${SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

set(SOURCES ${UI_ASM_FILES} ${UI_ASM_SOURCES})

if(VQ3_COMPATIBLE_QVM)
	set(VQ3_OPTION "-vq3")
endif(VQ3_COMPATIBLE_QVM)

add_custom_command(OUTPUT "${QVM_FILE}"
	COMMAND "$<TARGET_FILE:${Q3ASM_TARGET}>" ${Q3CPP_OPTIIONS} ${Q3CPP_VERBOSE_OPTION} ${VQ3_OPTION} -o "${QVM_FILE}" ${SOURCES}
	DEPENDS ${Q3ASM_TARGET} ${SOURCES}
)

add_custom_target(${BASE_UI_QVM_TARGET} ALL
	DEPENDS "${QVM_FILE}"
	SOURCES ${UI_C_SOURCES} ${UI_C_HEADERS}
)
set_target_properties(${BASE_UI_QVM_TARGET} PROPERTIES QVM_FILENAME "${QVM_FILENAME}")

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
