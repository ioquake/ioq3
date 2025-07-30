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

project("basegame ui vm")

set(BASE_UI_VM_TARGET "basegame_ui_vm" CACHE INTERNAL "Target name")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")
include("${CMAKELIST_DIR}/include/strip.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
option(NO_STRIP "No strip (keep all symbol table and relocation information)" ${DEFAULT_NO_STRIP})
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(BASEGAME_VM_INSTALL_DIR "${DEFAULT_BASEGAME_VM_INSTALL_DIR}" CACHE PATH "Basegame VM Installation directory")

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
	"${Q3_UI_DIR}/ui_main.c"
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
	"${UI_DIR}/ui_syscalls.c"
)
set(UI_HEADERS

)

ADD_VM_DEFINITIONS(UI_DEFINITIONS)
ADD_VM_CFLAGS(UI_CFLAGS)
ADD_STRIP_LDFLAG(UI_LDFLAGS)

list(APPEND UI_DEFINITIONS "-DUI")

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

add_library(${BASE_UI_VM_TARGET} MODULE ${UI_C_SOURCES} ${UI_C_HEADERS})

target_compile_definitions(${BASE_UI_VM_TARGET} PRIVATE ${UI_DEFINITIONS})

target_include_directories(${BASE_UI_VM_TARGET} PRIVATE ${UI_INCLUDE_DIRS})

compat_target_compile_options(${BASE_UI_VM_TARGET} PRIVATE ${UI_CFLAGS})

target_link_libraries(${BASE_UI_VM_TARGET} ${UI_LIBRARIES})

compat_target_link_options(${BASE_UI_VM_TARGET} PRIVATE ${UI_LDFLAGS})

set_target_properties(${BASE_UI_VM_TARGET} PROPERTIES PREFIX "")
set_target_properties(${BASE_UI_VM_TARGET} PROPERTIES LIBRARY_OUTPUT_NAME "ui${ARCH}")
set_target_properties(${BASE_UI_VM_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/basegame/vm")

STRIP_TARGET(${BASE_UI_VM_TARGET})

set(DESTIONATION_DIR)
if(BASEGAME_VM_INSTALL_DIR)
	set(DESTIONATION_DIR "${BASEGAME_VM_INSTALL_DIR}")
endif(BASEGAME_VM_INSTALL_DIR)

if(DESTIONATION_DIR)
	install(TARGETS ${BASE_UI_VM_TARGET}
		DESTINATION "${DESTIONATION_DIR}"
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
		CONFIGURATIONS Release
		COMPONENT missionpack_vm
	)
endif(DESTIONATION_DIR)

include(CPack)
