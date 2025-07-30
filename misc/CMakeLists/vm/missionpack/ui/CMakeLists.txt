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

project("missionpack ui vm")

set(MISSION_UI_VM_TARGET "missionpack_ui_vm" CACHE INTERNAL "Target name")

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

set(MISSIONPACK_VM_INSTALL_DIR "${DEFAULT_MISSIONPACK_VM_INSTALL_DIR}" CACHE PATH "Missionpack VM Installation directory")

set(GAME_SOURCES
	"${GAME_DIR}/bg_misc.c"
	"${GAME_DIR}/bg_lib.c"
)
set(GAME_HEADERS
	"${GAME_DIR}/bg_local.h"
	"${GAME_DIR}/bg_public.h"
	"${GAME_DIR}/bg_lib.h"
)
set(UI_SOURCES
	"${UI_DIR}/ui_atoms.c"
	"${UI_DIR}/ui_main.c"
	"${UI_DIR}/ui_gameinfo.c"
	"${UI_DIR}/ui_players.c"
	"${UI_DIR}/ui_shared.c"

	"${UI_DIR}/ui_syscalls.c"
)
set(UI_HEADERS
	"${UI_DIR}/ui_local.h"
	"${UI_DIR}/ui_public.h"

	"${UI_DIR}/ui_shared.h"

)
set(QCOMMON_SOURCES
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
)
set(QCOMMON_HEADERS
	"${QCOMMON_DIR}/q_shared.h"
)

ADD_VM_DEFINITIONS(UI_DEFINITIONS)
ADD_VM_CFLAGS(UI_CFLAGS)
ADD_STRIP_LDFLAG(UI_LDFLAGS)

list(APPEND UI_DEFINITIONS "-DUI")
list(APPEND UI_DEFINITIONS "-DMISSIONPACK")

list(APPEND UI_C_SOURCES ${UI_SOURCES} ${GAME_SOURCES} ${QCOMMON_SOURCES})
list(APPEND UI_C_HEADERS ${UI_HEADERS} ${GAME_HEADERS} ${QCOMMON_HEADERS})

source_group("game\\Source Files" FILES ${GAME_SOURCES})
source_group("game\\Header Files" FILES ${GAME_HEADERS})
source_group("ui\\Source Files" FILES ${UI_SOURCES})
source_group("ui\\Header Files" FILES ${UI_HEADERS})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})

add_library(${MISSION_UI_VM_TARGET} MODULE ${UI_C_SOURCES} ${UI_C_HEADERS})

target_compile_definitions(${MISSION_UI_VM_TARGET} PRIVATE ${UI_DEFINITIONS})

target_include_directories(${MISSION_UI_VM_TARGET} PRIVATE ${UI_INCLUDE_DIRS})

compat_target_compile_options(${MISSION_UI_VM_TARGET} PRIVATE ${UI_CFLAGS})

target_link_libraries(${MISSION_UI_VM_TARGET} ${UI_LIBRARIES})

compat_target_link_options(${MISSION_UI_VM_TARGET} PRIVATE ${UI_LDFLAGS})

set_target_properties(${MISSION_UI_VM_TARGET} PROPERTIES PREFIX "")
set_target_properties(${MISSION_UI_VM_TARGET} PROPERTIES LIBRARY_OUTPUT_NAME "ui${ARCH}")
set_target_properties(${MISSION_UI_VM_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/missionpack/vm")

STRIP_TARGET(${MISSION_UI_VM_TARGET})

set(DESTIONATION_DIR)
if(MISSIONPACK_VM_INSTALL_DIR)
	set(DESTIONATION_DIR "${MISSIONPACK_VM_INSTALL_DIR}")
endif(MISSIONPACK_VM_INSTALL_DIR)

if(DESTIONATION_DIR)
	install(TARGETS ${MISSION_UI_VM_TARGET}
		DESTINATION "${DESTIONATION_DIR}"
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
		CONFIGURATIONS Release
		COMPONENT missionpack_vm
	)
endif(DESTIONATION_DIR)

include(CPack)
