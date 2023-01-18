include(CheckCCompilerFlag)

macro(ADD_STRIP_LDFLAG LDFLAGS_VAR)
	if(NOT NO_STRIP)
		if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
			set(STRIP_FLAG)
			CHECK_C_COMPILER_FLAG("-s" COMPILER_SUPPORTS_-s)
			if(COMPILER_SUPPORTS_-s)
				set(STRIP_FLAG "-s")
			endif()
			list(APPEND ${LDFLAGS_VAR} $<$<CONFIG:Release>:${STRIP_FLAG}>)
		endif()
	endif()
endmacro()

macro(STRIP_TARGET STRIP_TARGET_NAME)

	if(CMAKE_STRIP)
		set(STRIP_EXECUTABLE "${CMAKE_STRIP}")
	else()
		find_program(STRIP_EXECUTABLE "strip")
	endif()
	if(STRIP_EXECUTABLE AND NOT NO_STRIP)
		add_custom_command(TARGET "${STRIP_TARGET_NAME}" POST_BUILD
			COMMAND $<$<CONFIG:Release>:${STRIP_EXECUTABLE}> ARGS $<TARGET_FILE:${STRIP_TARGET_NAME}>)
	endif()

endmacro()
