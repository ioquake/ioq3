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

project("server")

set(SERVER_TARGET "server")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")
include("${CMAKELIST_DIR}/include/git_rev.cmake")
include("${CMAKELIST_DIR}/include/strip.cmake")
include("${CMAKELIST_DIR}/include/platform.cmake")
include("${CMAKELIST_DIR}/include/vm_sources.cmake")

include(CheckCSourceCompiles)
include(CheckIncludeFile)

#cmake does append any extensions
set(SERVER_BIN "${DEFAULT_SERVER_BIN}" CACHE STRING "binary/executable name")

#Note: options and build directories get cached
option(USE_INTERNAL_LIBS "Use internal libraries (zlib)" ${DEFAULT_USE_INTERNAL_LIBS})
option(USE_INTERNAL_ZLIB "Use internal zlib" ${USE_INTERNAL_LIBS})
option(USE_LOCAL_HEADERS "Use local headers" ${USE_INTERNAL_LIBS})
option(USE_VOIP "Use VOIP" ${DEFAULT_USE_VOIP})
if(NOT PLATFORM STREQUAL "darwin")
	option(USE_ICON "Use window icon" ${DEFAULT_USE_ICON})
endif()
option(USE_AUTOUPDATER "Use Autoupdater" ${DEFAULT_USE_AUTOUPDATER})
set(PRODUCT_VERSION "${DEFAULT_VERSION}" CACHE STRING "Product version")
option(USE_GIT_REV "Use Git revision for version" ${DEFAULT_USE_GIT_REV})

option(BUILD_STANDALONE "Standalone" ${DEFAULT_BUILD_STANDALONE})
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
option(NO_STRIP "No strip (keep all symbol table and relocation information)" ${DEFAULT_NO_STRIP})
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(BIN_EXT "${DEFAULT_BIN_EXT}" CACHE STRING "Binary/executable extension")

set(REL_INSTALL_DEST "${DEFAULT_REL_INSTALL_DEST}" CACHE PATH "Relative installation destination")
set(INSTALL_DIR "${DEFAULT_INSTALL_DIR}" CACHE PATH "Installation directory")

set(ASM_SOURCES

)

set(ASM_C_SOURCES
	"${ASM_DIR}/ftola.c"
	"${ASM_DIR}/snapvector.c"
)

set(ASM_MASM_SOURCES
	"${ASM_DIR}/ftola.asm"
	"${ASM_DIR}/snapvector.asm"
#	"${ASM_DIR}/vm_x86_64.asm"
)

set(ASM_S_SOURCES
	"${ASM_DIR}/matha.s"
#	"${ASM_DIR}/snd_mixa.s"
)

set(ASM_HEADERS
#	"${ASM_DIR}/qasm.h"
#	"${ASM_DIR}/qasm-inline.h"
)

set(BOTLIB_SOURCES
	"${BOTLIB_DIR}/be_aas_bspq3.c"
	"${BOTLIB_DIR}/be_aas_cluster.c"
	"${BOTLIB_DIR}/be_aas_debug.c"
	"${BOTLIB_DIR}/be_aas_entity.c"
	"${BOTLIB_DIR}/be_aas_file.c"
	"${BOTLIB_DIR}/be_aas_main.c"
	"${BOTLIB_DIR}/be_aas_move.c"
	"${BOTLIB_DIR}/be_aas_optimize.c"
	"${BOTLIB_DIR}/be_aas_reach.c"
	"${BOTLIB_DIR}/be_aas_route.c"
	"${BOTLIB_DIR}/be_aas_routealt.c"
	"${BOTLIB_DIR}/be_aas_sample.c"
	"${BOTLIB_DIR}/be_ai_char.c"
	"${BOTLIB_DIR}/be_ai_chat.c"
	"${BOTLIB_DIR}/be_ai_gen.c"
	"${BOTLIB_DIR}/be_ai_goal.c"
	"${BOTLIB_DIR}/be_ai_move.c"
	"${BOTLIB_DIR}/be_ai_weap.c"
	"${BOTLIB_DIR}/be_ai_weight.c"
	"${BOTLIB_DIR}/be_ea.c"
	"${BOTLIB_DIR}/be_interface.c"
	"${BOTLIB_DIR}/l_crc.c"
	"${BOTLIB_DIR}/l_libvar.c"
	"${BOTLIB_DIR}/l_log.c"
	"${BOTLIB_DIR}/l_memory.c"
	"${BOTLIB_DIR}/l_precomp.c"
	"${BOTLIB_DIR}/l_script.c"
	"${BOTLIB_DIR}/l_struct.c"
)
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
set(NULL_SOURCES
	"${NULL_DIR}/null_client.c"
	"${NULL_DIR}/null_input.c"
	"${NULL_DIR}/null_snddma.c"
)
set(NULL_HEADERS

)
set(QCOMMON_SOURCES
	"${QCOMMON_DIR}/cm_load.c"
	"${QCOMMON_DIR}/cm_patch.c"
	"${QCOMMON_DIR}/cm_polylib.c"
	"${QCOMMON_DIR}/cm_test.c"
	"${QCOMMON_DIR}/cm_trace.c"
	"${QCOMMON_DIR}/cmd.c"
	"${QCOMMON_DIR}/common.c"
	"${QCOMMON_DIR}/cvar.c"
	"${QCOMMON_DIR}/files.c"
	"${QCOMMON_DIR}/huffman.c"
	"${QCOMMON_DIR}/ioapi.c"
	"${QCOMMON_DIR}/md4.c"
	"${QCOMMON_DIR}/md5.c"
	"${QCOMMON_DIR}/msg.c"
	"${QCOMMON_DIR}/net_chan.c"
	"${QCOMMON_DIR}/net_ip.c"
	"${QCOMMON_DIR}/puff.c"
	"${QCOMMON_DIR}/q_math.c"
	"${QCOMMON_DIR}/q_shared.c"
	"${QCOMMON_DIR}/unzip.c"
	"${QCOMMON_DIR}/vm.c"
	"${QCOMMON_DIR}/vm_interpreted.c"
#	"${QCOMMON_DIR}/vm_none.c"
#	"${QCOMMON_DIR}/vm_powerpc.c"
#	"${QCOMMON_DIR}/vm_powerpc_asm.c"
#	"${QCOMMON_DIR}/vm_sparc.c"
#	"${QCOMMON_DIR}/vm_x86.c"
)
set(QCOMMON_HEADERS
	"${QCOMMON_DIR}/cm_local.h"
	"${QCOMMON_DIR}/cm_patch.h"
	"${QCOMMON_DIR}/cm_polylib.h"
	"${QCOMMON_DIR}/cm_public.h"
	"${QCOMMON_DIR}/ioapi.h"
	"${QCOMMON_DIR}/puff.h"
	"${QCOMMON_DIR}/q_platform.h"
	"${QCOMMON_DIR}/q_shared.h"
	"${QCOMMON_DIR}/qcommon.h"
	"${QCOMMON_DIR}/qfiles.h"
	"${QCOMMON_DIR}/surfaceflags.h"
	"${QCOMMON_DIR}/unzip.h"
	"${QCOMMON_DIR}/vm_local.h"
#	"${QCOMMON_DIR}/vm_powerpc_asm.h"
#	"${QCOMMON_DIR}/vm_sparc.h"
)

set(SERVER_SOURCES
	"${SERVER_DIR}/sv_bot.c"
	"${SERVER_DIR}/sv_ccmds.c"
	"${SERVER_DIR}/sv_client.c"
	"${SERVER_DIR}/sv_game.c"
	"${SERVER_DIR}/sv_init.c"
	"${SERVER_DIR}/sv_main.c"
	"${SERVER_DIR}/sv_net_chan.c"
#	"${SERVER_DIR}/sv_rankings.c"
	"${SERVER_DIR}/sv_snapshot.c"
	"${SERVER_DIR}/sv_world.c"
)
set(SERVER_HEADERS
	"${SERVER_DIR}/server.h"
)

set(SYS_SOURCES
	"${SYS_DIR}/con_log.c"
#	"${SYS_DIR}/con_passive.c"
#	"${SYS_DIR}/con_tty.c"
#	"${SYS_DIR}/con_win32.c"
	"${SYS_DIR}/sys_autoupdater.c"
	"${SYS_DIR}/sys_main.c"
#	"${SYS_DIR}/sys_unix.c"
#	"${SYS_DIR}/sys_win32.c"
)
set(SYS_HEADERS
	"${SYS_DIR}/sys_loadlib.h"
	"${SYS_DIR}/sys_local.h"
	"${SYS_DIR}/win_resource.h"
)
list(APPEND SYS_RC_SOURCES
	"${SYS_DIR}/win_resource.rc"
	"${SYS_DIR}/win_manifest.xml"
)
list(APPEND SYS_WIN_SOURCES
	"${SYS_DIR}/con_passive.c"
	"${SYS_DIR}/sys_win32.c"
#	"${SYS_DIR}/con_win32.c"
)
list(APPEND SYS_UNIX_SOURCES
#	"${SYS_DIR}/con_unix.c"
	"${SYS_DIR}/con_tty.c"
	"${SYS_DIR}/sys_unix.c"
)
list(APPEND SYS_OSX_SOURCES
#	"${SYS_DIR}/sys_osx.c"
	"${SYS_DIR}/sys_osx.m"
)

CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
if(HAVE_DLFCN_H)
	list(APPEND SERVER_DEFINITIONS "-DHAVE_DLFCN_H")
endif(HAVE_DLFCN_H)

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
CHECK_C_SOURCE_COMPILES("#include <dlfcn.h>\n int main () {void *dl = dlopen(\"libm.so\", RTLD_NOW);return 0;}" HAVE_DLOPEN_IN_DLFCN_H)
set(CMAKE_REQUIRED_LIBRARIES)
if(HAVE_DLOPEN_IN_DLFCN_H)
	list(APPEND SERVER_DEFINITIONS "-DHAVE_DLOPEN")
	list(APPEND SERVER_LIBRARIES ${CMAKE_DL_LIBS})
endif(HAVE_DLOPEN_IN_DLFCN_H)

if(WIN32)
	#requires parent directory for misc/quake3.ico in win_resource.rc
	list(APPEND SERVER_INCLUDE_DIRS "${SOURCE_DIR}")
endif(WIN32)

if(MSVC)
	list(APPEND SYS_SOURCES ${SYS_WIN_SOURCES})

	include(CheckLanguage)
	check_language(ASM_MASM)
	if(CMAKE_ASM_MASM_COMPILER)
		enable_language(ASM_MASM)
		set(HAVE_MASM_COMPILER TRUE)
	endif(CMAKE_ASM_MASM_COMPILER)

	set(USE_MASM_SOURCES TRUE)

	#Character Set, CharacterSet="0", Not Set, ASCII/SBCS (Single Byte Character Set)
	list(APPEND SERVER_DEFINITIONS "-D_SBCS")

	#disable deprecation warnings about old functions like strcmp
	list(APPEND SERVER_DEFINITIONS "-D_CRT_SECURE_NO_WARNINGS")

	if(ARCH STREQUAL "x86")
		list(APPEND SERVER_DEFINITIONS "-D__i386__")
	endif()

	#Enable String Pooling /GF
	list(APPEND SERVER_CFLAGS "/GF")
	#Enable Function-Level Linking /Gy
	list(APPEND SERVER_CFLAGS "/Gy")
	#Warning Level /W4
	list(APPEND SERVER_CFLAGS "/W4")
	#Floating Point Model /fp:fast
	list(APPEND SERVER_CFLAGS "/fp:fast")
	#Compile as, C Code /TC
	list(APPEND SERVER_CFLAGS "/TC")
	#Enable C++ Exceptions, No /EHa-
	list(APPEND SERVER_CFLAGS "/EHa-")
	#Whole program optimization, no /GL-
	list(APPEND SERVER_CFLAGS "/GL-")

	#Disable specific warnings
	#list(APPEND SERVER_CFLAGS "/wd4244")

	#Subsystem /SUBSYSTEM:CONSOLE
	list(APPEND SERVER_LDFLAGS "/SUBSYSTEM:CONSOLE")
	#Stack Reserve Size /STACK:8388608
	list(APPEND SERVER_LDFLAGS "/STACK:8388608")
	#Randomized Base Address /DYNAMICBASE:NO
	list(APPEND SERVER_LDFLAGS "/DYNAMICBASE:NO")
	#Data Execution Prevenbtion (DEP) /NXCOMPAT:NO
	list(APPEND SERVER_LDFLAGS "/NXCOMPAT:NO")
	#Image Has Safe Exception Handlers, is no longer default no in msvc 2013 /SAFESEH:NO
	list(APPEND SERVER_LDFLAGS "/SAFESEH:NO")

	#not generating manifest, using win_manifest.xml
	list(APPEND SERVER_LDFLAGS "/MANIFEST:NO")

	#time functions winmm.lib
	#EnumProcesses function psapi.lib
	#windows socket functions ws2_32.lib
	list(APPEND SERVER_LIBRARIES "winmm" "psapi" "ws2_32")

	#requires directory misc/msvc for ../quake3.ico in win_resource.rc
	list(APPEND SERVER_INCLUDE_DIRS "${SOURCE_DIR}/misc/msvc")

endif(MSVC)

if(MINGW AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	list(APPEND SYS_SOURCES ${SYS_WIN_SOURCES})

	#link the MinGW library first
	list(INSERT SERVER_LIBRARIES 0 "mingw32")

	#time functions winmm.lib
	#EnumProcesses function psapi.lib
	#socket functions ws2_32.lib
	#ImmGetIMEFileName imm32.lib
	#GetFileVersionInfoSize Version.lib
	list(APPEND SERVER_LIBRARIES "imm32" "Version" "winmm" "psapi" "ws2_32")

	#WINVER is the minimum operating system version the code supports for the Windows SDK.
	# 0x0501 for Windows XP, 0x0502 for Windows Server 2003,
	# 0x0600 for Windows Vista, 0x0601 for Windows 7,
	# 0x0602 for Windows 8, 0x603 for Windows 8.1 and 0x0A00 for Windows 10.
	list(APPEND SERVER_DEFINITIONS "-DWINVER=0x0501")

endif()

if(PLATFORM STREQUAL "darwin" AND (CMAKE_COMPILER_IS_GNUCC
	OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	list(APPEND SYS_SOURCES ${SYS_OSX_SOURCES})
	set_source_files_properties("${SYS_DIR}/sys_osx.m" PROPERTIES LANGUAGE C)

	list(APPEND SERVER_DEFINITIONS "-DMACOS_X")
	find_library(COCOA_LIBRARY Cocoa)
	if(COCOA_LIBRARY)
		list(APPEND SERVER_LIBRARIES "${COCOA_LIBRARY}")
	else()
		list(APPEND SERVER_LDFLAGS "-framework Cocoa")
	endif()

endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	if(ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64")
		set(USE_INLINE_ASM_SOURCES TRUE)
	endif()

	if(NOT MINGW)
		list(APPEND SYS_SOURCES ${SYS_UNIX_SOURCES})
	endif()

endif()

if(USE_INLINE_ASM_SOURCES)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_inline_asm")
	list(APPEND SERVER_LIBRARIES ${INTERNAL_INLINE_ASM_LIBRARIES})
endif()

if(USE_MASM_SOURCES)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/internal_masm")
	list(APPEND SERVER_LIBRARIES ${INTERNAL_MASM_LIBRARIES})
endif()

ADD_SERVER_DEFINITIONS(SERVER_DEFINITIONS)
ADD_SERVER_CFLAGS(SERVER_CFLAGS)
ADD_STRIP_LDFLAG(SERVER_LDFLAGS)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	list(APPEND SERVER_CFLAGS "-Qunused-arguments")
endif()

GET_VM_SOURCES(VM_SOURCES)
if(VM_SOURCES)
	list(APPEND QCOMMON_SOURCES ${VM_SOURCES})
else()
	set(NO_VM_COMPILED TRUE)
endif()
if(NO_VM_COMPILED)
	list(APPEND SERVER_DEFINITIONS "-DNO_VM_COMPILED")
endif()

if(USE_GIT_REV AND GIT_REV)
	list(APPEND SERVER_DEFINITIONS "-DPRODUCT_VERSION=\"${PRODUCT_VERSION}_GIT_${GIT_REV}\"")
else()
	list(APPEND SERVER_DEFINITIONS "-DPRODUCT_VERSION=\"${PRODUCT_VERSION}\"")
endif()

if(DEFINED ENV{SOURCE_DATE_EPOCH})
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.8.0")
		string(TIMESTAMP PRODUCT_DATE "%b %d %Y" UTC) #Nov 28 2021
		list(APPEND SERVER_DEFINITIONS "-DPRODUCT_DATE=\"${PRODUCT_DATE}\"")
	else()
		message(WARNING "Update CMake to version 3.8 or later for SOURCE_DATE_EPOCH support!")
	endif()
endif()

if(USE_INTERNAL_ZLIB)
	list(APPEND SERVER_DEFINITIONS "-DUSE_INTERNAL_ZLIB")
	if(NOT DEFINED INTERNAL_ZLIB_LIBRARIES)
		add_subdirectory("${CMAKELIST_DIR}/internal_zlib" "internal_zlib")
	endif()
	list(APPEND SERVER_DEFINITIONS ${INTERNAL_ZLIB_DEFINITIONS})
	list(APPEND SERVER_INCLUDE_DIRS ${INTERNAL_ZLIB_INCLUDE_DIRS})
	list(APPEND SERVER_LIBRARIES ${INTERNAL_ZLIB_LIBRARIES})
else()
	find_package(ZLIB REQUIRED)
	list(APPEND SERVER_DEFINITIONS ${ZLIB_DEFINITIONS})
	list(APPEND SERVER_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS})
	list(APPEND SERVER_LIBRARIES ${ZLIB_LIBRARIES})
endif(USE_INTERNAL_ZLIB)

if(USE_FREETYPE)
	find_package(Freetype REQUIRED)
	if(DEFINED FREETYPE_VERSION AND NOT FREETYPE_VERSION STREQUAL "")
		if(${FREETYPE_VERSION} VERSION_LESS 2)
			message(FATAL_ERROR "The Freetype library version is ${FREETYPE_VERSION} but required is at least version 2.")
		endif()
	endif()

	list(APPEND SERVER_DEFINITIONS "-DBUILD_FREETYPE")

	list(APPEND SERVER_DEFINITIONS ${FREETYPE_DEFINITIONS})
	if(FREETYPE_INCLUDE_DIR)
		list(APPEND SERVER_INCLUDE_DIRS "${FREETYPE_INCLUDE_DIR}")
	endif(FREETYPE_INCLUDE_DIR)
	list(APPEND SERVER_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS})
	list(APPEND SERVER_LIBRARIES "${FREETYPE_LIBRARY}")
	list(APPEND SERVER_LIBRARIES ${FREETYPE_LIBRARIES})

endif(USE_FREETYPE)

list(APPEND SERVER_DEFINITIONS "-DBOTLIB")
list(APPEND SERVER_DEFINITIONS "-DDEDICATED")

if(USE_ICON)
	list(APPEND SERVER_DEFINITIONS "-DUSE_ICON")
endif(USE_ICON)

if(BUILD_STANDALONE)
	list(APPEND SERVER_DEFINITIONS "-DSTANDALONE")
endif(BUILD_STANDALONE)

if(USE_LOCAL_HEADERS)
	list(APPEND SERVER_DEFINITIONS "-DUSE_LOCAL_HEADERS")
endif(USE_LOCAL_HEADERS)

if(DEFAULT_BASEDIR)
	list(APPEND SERVER_DEFINITIONS "-DDEFAULT_BASEDIR=\"${DEFAULT_BASEDIR}\"")
endif(DEFAULT_BASEDIR)

if(USE_VOIP)
	list(APPEND SERVER_DEFINITIONS "-DUSE_VOIP")
endif(USE_VOIP)

if(USE_AUTOUPDATER)
	list(APPEND SERVER_DEFINITIONS "-DUSE_AUTOUPDATER")
	if(NOT DEFINED AUTOUPDATER_BIN)
		set(AUTOUPDATER_BIN, DEFAULT_AUTOUPDATER_BIN)
	endif()
	list(APPEND SERVER_DEFINITIONS "-DAUTOUPDATER_BIN=\"${AUTOUPDATER_BIN}\"")
endif(USE_AUTOUPDATER)

list(APPEND SERVER_C_SOURCES ${NULL_SOURCES} ${SERVER_SOURCES} ${BOTLIB_SOURCES} ${QCOMMON_SOURCES} ${SYS_SOURCES})
list(APPEND SERVER_C_HEADERS ${NULL_HEADERS} ${SERVER_HEADERS} ${BOTLIB_HEADERS} ${QCOMMON_HEADERS} ${SYS_HEADERS})

#IDE grouping
source_group("asm\\Source Files" FILES ${ASM_SOURCES})
source_group("asm\\Header Files" FILES ${ASM_HEADERS})
source_group("botlib\\Source Files" FILES ${BOTLIB_SOURCES})
source_group("botlib\\Header Files" FILES ${BOTLIB_HEADERS})
source_group("qcommon\\Source Files" FILES ${QCOMMON_SOURCES})
source_group("qcommon\\Header Files" FILES ${QCOMMON_HEADERS})
source_group("server\\Source Files" FILES ${SERVER_SOURCES})
source_group("server\\Header Files" FILES ${SERVER_HEADERS})
source_group("sys\\Source Files" FILES ${SYS_SOURCES})
source_group("sys\\Header Files" FILES ${SYS_HEADERS})
source_group("sys" FILES ${SYS_RC_SOURCES})

add_executable(${SERVER_TARGET} ${SERVER_C_SOURCES} ${SERVER_C_HEADERS} ${SYS_RC_SOURCES})

target_compile_definitions(${SERVER_TARGET} PRIVATE ${SERVER_DEFINITIONS})

target_include_directories(${SERVER_TARGET} PRIVATE ${SERVER_INCLUDE_DIRS})

compat_target_compile_options(${SERVER_TARGET} PRIVATE ${SERVER_CFLAGS})

target_link_libraries(${SERVER_TARGET} ${SERVER_LIBRARIES})

compat_target_link_options(${SERVER_TARGET} PRIVATE ${SERVER_LDFLAGS})

set_target_properties(${SERVER_TARGET} PROPERTIES RUNTIME_OUTPUT_NAME "${SERVER_BIN}${BIN_EXT}")

STRIP_TARGET(${SERVER_TARGET})

set(DESTINATION_DIR "${REL_INSTALL_DEST}")
if(INSTALL_DIR)
	set(DESTINATION_DIR "${INSTALL_DIR}")
endif(INSTALL_DIR)

install(TARGETS ${SERVER_TARGET}
	RUNTIME
	DESTINATION "${DESTINATION_DIR}"
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
		GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
	CONFIGURATIONS Release
	COMPONENT server
)
include(CPack)
