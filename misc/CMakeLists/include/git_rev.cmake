
if(USE_GIT_REV AND IS_DIRECTORY "${SOURCE_DIR}/.git")
	find_package(Git)

	if(NOT GIT_FOUND)
		set(GIT_EXECUTABLE "git")
	endif()

	execute_process(
		COMMAND "${GIT_EXECUTABLE}" show -s --pretty=format:%h-%ad --date=short
		WORKING_DIRECTORY "${SOURCE_DIR}"
		OUTPUT_VARIABLE GIT_REV
		RESULT_VARIABLE GIT_REV_RESULT
		ERROR_VARIABLE GIT_REV_ERROR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(NOT GIT_REV)
		message(WARNING "Could not execute Git. ${GIT_REV_ERROR} ${GIT_REV_RESULT}")
	endif(NOT GIT_REV)
endif()