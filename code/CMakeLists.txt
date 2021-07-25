add_subdirectory(AL)
add_subdirectory(curl-7.54.0)
add_subdirectory(jpeg-8c)
add_subdirectory(libogg-1.3.3)
add_subdirectory(libvorbis-1.3.6)
add_subdirectory(opus-1.2.1)
add_subdirectory(opusfile-0.9)
add_subdirectory(zlib)
add_subdirectory(SDL2)

check_compiler_flag(-Wall)
check_compiler_flag(-Wextra)
check_compiler_flag(-Wno-unused-parameter)
check_compiler_flag(-Wno-sign-compare)

set(QCOMMON_COLLISION_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_load.c
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_local.h
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_patch.c
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_patch.h
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_polylib.c
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_polylib.h
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_public.h
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_test.c
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/cm_trace.c
)
set(QCOMMON_VM_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/vm.c
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/vm_interpreted.c
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/vm_local.h
	${CMAKE_CURRENT_SOURCE_DIR}/qcommon/vm_x86.c
)

find_program(GDB_EXECUTABLE gdb)
find_program(LLDB_EXECUTABLE lldb)
if (GDB_EXECUTABLE)
	set(DEBUGGER ${GDB_EXECUTABLE} CACHE STRING "Which debugger should be used")
elseif (LLDB_EXECUTABLE)
	set(DEBUGGER ${LLDB_EXECUTABLE} CACHE STRING "Which debugger should be used")
else()
	set(DEBUGGER "unknown" CACHE STRING "Which debugger should be used")
	message(STATUS "No debugger (gdb or lldb) was found")
endif()
set_property(CACHE DEBUGGER PROPERTY STRINGS gdb lldb)

function(add_debuggger TARGET)
	if (${DEBUGGER} MATCHES "gdb")
		add_custom_target(${TARGET}-debug)
		add_custom_command(TARGET ${TARGET}-debug
			COMMAND ${GDB_EXECUTABLE} -ex run --args $<TARGET_FILE:${TARGET}>
			COMMENT "Starting debugger session for ${TARGET}"
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			DEPENDS ${TARGET}
		)
	elseif (${DEBUGGER} MATCHES "lldb")
		add_custom_target(${TARGET}-debug)
		add_custom_command(TARGET ${TARGET}-debug
			COMMAND CG_CONTEXT_SHOW_BACKTRACE=1 ${LLDB_EXECUTABLE} -b -o run $<TARGET_FILE:${TARGET}>
			COMMENT "Starting debugger session for ${TARGET}"
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			DEPENDS ${TARGET}
		)
	endif()
endfunction()

function(q3_add_library)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET GAME_NAME)
	set(_MULTI_VALUE_ARGS SRCS)

	cmake_parse_arguments(_LIB "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	set(ARCH x86)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(ARCH x86_64)
	endif()

	set(OUTPUTNAME "${_LIB_TARGET}_${ARCH}")
	set(LIB_BINARY_DIR "${ENGINE_BINARY_DIR}")

	if (_LIB_GAME_NAME)
		set(LIB_TARGET ${_LIB_GAME_NAME}-${_LIB_TARGET})
		set(LIB_BINARY_DIR "${CMAKE_BINARY_DIR}/${_LIB_GAME_NAME}")
	else()
		set(LIB_TARGET ${_LIB_TARGET})
	endif()

	add_library(${LIB_TARGET} SHARED ${_LIB_SRCS})

	if (_LIB_GAME_NAME)
		set_target_properties(${LIB_TARGET} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${CMAKE_PROJECT_NAME}/${_LIB_GAME_NAME})
	else()
		set_target_properties(${LIB_TARGET} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${CMAKE_PROJECT_NAME})
	endif()

	set_target_properties(${LIB_TARGET} PROPERTIES PREFIX "")
	set_target_properties(${LIB_TARGET} PROPERTIES OUTPUT_NAME "${OUTPUTNAME}")
	set_target_properties(${LIB_TARGET} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${LIB_BINARY_DIR}"
		LIBRARY_OUTPUT_DIRECTORY "${LIB_BINARY_DIR}"
		RUNTIME_OUTPUT_DIRECTORY "${LIB_BINARY_DIR}"
	)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		set_target_properties(${LIB_TARGET} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${LIB_BINARY_DIR}"
			LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${LIB_BINARY_DIR}"
			RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${LIB_BINARY_DIR}"
		)
	endforeach()
endfunction()

function(q3_add_executable)
	set(_OPTIONS_ARGS WINDOWED)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	if (_EXE_WINDOWED)
		if (WIN32)
			add_executable(${_EXE_TARGET} WIN32 ${SRCS})
			if (MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
			endif()
		elseif(APPLE)
			add_executable(${_EXE_TARGET} MACOSX_BUNDLE ${SRCS})
		else()
			add_executable(${_EXE_TARGET} ${SRCS})
		endif()
	else()
		add_executable(${_EXE_TARGET} ${SRCS})
		if (WIN32)
			if (MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
			endif()
		endif()
	endif()

	set(OUTPUTNAME "${_EXE_TARGET}.${ARCH_STRING}")
	set_target_properties(${_EXE_TARGET} PROPERTIES OUTPUT_NAME "${OUTPUTNAME}")
	set_target_properties(${_EXE_TARGET} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${ENGINE_BINARY_DIR}"
		LIBRARY_OUTPUT_DIRECTORY "${ENGINE_BINARY_DIR}"
		RUNTIME_OUTPUT_DIRECTORY "${ENGINE_BINARY_DIR}"
	)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		set_target_properties(${_EXE_TARGET} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${ENGINE_BINARY_DIR}"
			LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${ENGINE_BINARY_DIR}"
			RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${ENGINE_BINARY_DIR}"
		)
	endforeach()

	if (APPLE)
		configure_file(${ROOT_DIR}/misc/osx/application.plist.in ${CMAKE_CURRENT_BINARY_DIR}/application.plist)
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/application.plist)
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE ON)
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "${CMAKE_PROJECT_NAME}")
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${CODESIGNIDENTITY})
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID})
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ${PROVISIONING_PROFILE_NAME})
		if (${CMAKE_GENERATOR} STREQUAL "Xcode")
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_EXECUTABLE_NAME \${EXECUTABLE_NAME})
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_PRODUCT_NAME \${PRODUCT_NAME})
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME \${PRODUCT_NAME})
		else()
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_PRODUCT_NAME "${CMAKE_PROJECT_NAME}")
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
		endif()
		configure_file(${ROOT_DIR}/misc/osx/copy_dylib.sh.in ${CMAKE_CURRENT_BINARY_DIR}/copy_dylib.sh @ONLY)
		add_custom_command(TARGET ${_EXE_TARGET} POST_BUILD COMMAND cd ${CMAKE_BINARY_DIR}\; ${CMAKE_CURRENT_BINARY_DIR}/copy_dylib.sh)
	endif()

	add_custom_target(${_EXE_TARGET}-run
		COMMAND $<TARGET_FILE:${_EXE_TARGET}>
		USES_TERMINAL
		DEPENDS ${_EXE_TARGET}
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
	add_debuggger(${_EXE_TARGET})
endfunction()

function(add_qvm TARGET_SHORT GAME_NAME)
	set(srcfiles ${ARGV})
	string(TOUPPER ${TARGET_SHORT} UPPERTARGET)
	set(TARGET ${GAME_NAME}-${TARGET_SHORT})
	set(compileflags "-D${UPPERTARGET}")
	set(QVM_SRCS)
	foreach(srcfile ${srcfiles})
		get_filename_component(ext ${srcfile} EXT)
		if ("${ext}" STREQUAL ".asm")
			list(APPEND QVM_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/${srcfile})
		elseif ("${ext}" STREQUAL ".c")
			get_filename_component(basename ${srcfile} NAME_WE)
			set(q3asm_outfile "${CMAKE_CURRENT_BINARY_DIR}/${basename}.asm")
			add_custom_command(
				OUTPUT ${q3asm_outfile}
				COMMAND q3lcc
				ARGS ${compileflags} -o "\"${q3asm_outfile}\"" "\"${CMAKE_CURRENT_SOURCE_DIR}/${srcfile}\""
				DEPENDS q3lcc ${CMAKE_CURRENT_SOURCE_DIR}/${srcfile}
			)
			set_source_files_properties(${q3asm_outfile} PROPERTIES GENERATED TRUE)
			list(APPEND QVM_SRCS ${q3asm_outfile})
		endif()
	endforeach()

	set(QVM_PATH ${GAME_NAME}/vm/${TARGET}.qvm)
	add_custom_command(
		OUTPUT "${ENGINE_BINARY_DIR}/${QVM_PATH}"
		COMMAND q3asm
		ARGS -o "${ENGINE_BINARY_DIR}/${QVM_PATH}" ${QVM_SRCS}
		DEPENDS q3asm ${QVM_SRCS}
	)
	add_custom_target(qvm_${TARGET} DEPENDS "${ENGINE_BINARY_DIR}/${QVM_PATH}")
	add_dependencies(${TARGET} qvm_${TARGET})
	set_source_files_properties(${QVM_PATH} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${QVM_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${QVM_PATH})
endfunction()

function(add_botlib TARGET)
	set(BOTLIB_SRCS
		${CODE_DIR}/botlib/be_aas_bspq3.c
		${CODE_DIR}/botlib/be_aas_cluster.c
		${CODE_DIR}/botlib/be_aas_debug.c
		${CODE_DIR}/botlib/be_aas_entity.c
		${CODE_DIR}/botlib/be_aas_file.c
		${CODE_DIR}/botlib/be_aas_main.c
		${CODE_DIR}/botlib/be_aas_move.c
		${CODE_DIR}/botlib/be_aas_optimize.c
		${CODE_DIR}/botlib/be_aas_reach.c
		${CODE_DIR}/botlib/be_aas_route.c
		${CODE_DIR}/botlib/be_aas_routealt.c
		${CODE_DIR}/botlib/be_aas_sample.c
		${CODE_DIR}/botlib/be_ai_char.c
		${CODE_DIR}/botlib/be_ai_chat.c
		${CODE_DIR}/botlib/be_ai_gen.c
		${CODE_DIR}/botlib/be_ai_goal.c
		${CODE_DIR}/botlib/be_ai_move.c
		${CODE_DIR}/botlib/be_ai_weap.c
		${CODE_DIR}/botlib/be_ai_weight.c
		${CODE_DIR}/botlib/be_ea.c
		${CODE_DIR}/botlib/be_interface.c
		${CODE_DIR}/botlib/l_crc.c
		${CODE_DIR}/botlib/l_libvar.c
		${CODE_DIR}/botlib/l_log.c
		${CODE_DIR}/botlib/l_memory.c
		${CODE_DIR}/botlib/l_precomp.c
		${CODE_DIR}/botlib/l_script.c
		${CODE_DIR}/botlib/l_struct.c
	)
	target_sources(${TARGET} PRIVATE ${BOTLIB_SRCS})
	#target_include_directories(${TARGET} PRIVATE ${CODE_DIR}/botlib)
	foreach(_file ${BOTLIB_SRCS})
		set_property(SOURCE ${_file} APPEND PROPERTY COMPILE_DEFINITIONS BOTLIB)
		set_property(SOURCE ${_file} APPEND PROPERTY INCLUDE_DIRECTORIES ${CODE_DIR}/botlib)
	endforeach()
endfunction()

function(add_asm TARGET)
	if (MSVC)
		enable_language(ASM_MASM)
		set(ASM_SRCS)
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
				list(APPEND ASM_SRCS ../asm/vm_x86_64.asm)
		endif()
		list(APPEND ASM_SRCS ../asm/snapvector.asm ../asm/ftola.asm)
		foreach(_file ${ASM_SRCS})
			if (CMAKE_SIZEOF_VOID_P EQUAL 8)
				set_property(SOURCE ${_file} APPEND PROPERTY COMPILE_DEFINITIONS idx64=1)
			endif()
			set_property(SOURCE ${_file} APPEND PROPERTY LANGUAGE ASM_MASM)
		endforeach()
		target_sources(${TARGET} PRIVATE ${ASM_SRCS})
	else()
		target_sources(${TARGET} PRIVATE ../asm/snapvector.c ../asm/ftola.c)
	endif()
endfunction()

if (DEFAULT_BASEDIR)
	add_compile_definitions(DEFAULT_BASEDIR="${DEFAULT_BASEDIR}")
endif()

add_subdirectory(tools)

set(RENDERER_LIST renderer_opengl1)
add_subdirectory(renderergl1)
if (BUILD_RENDERER_OPENGL2)
	add_subdirectory(renderergl2)
	list(APPEND RENDERER_LIST renderer_opengl2)
endif()

add_subdirectory(cgame)
add_subdirectory(game)
add_subdirectory(q3_ui)

if (BUILD_AUTOUPDATER)
	add_subdirectory(autoupdater)
endif()
if (BUILD_CLIENT)
	add_subdirectory(client)
endif()
if (BUILD_SERVER)
	add_subdirectory(server)
endif()
