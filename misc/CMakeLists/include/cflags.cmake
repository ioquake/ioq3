include(CheckCCompilerFlag)

macro(ADD_SUPPORTED_CFLAG IN_CFLAG OUT_LIST_CFLAGS)
	string(REGEX REPLACE "[^A-Za-z0-9_-]" "_" CURRENT_VAR_FLAG ${IN_CFLAG})
	CHECK_C_COMPILER_FLAG("${IN_CFLAG}" COMPILER_SUPPORTS_${CURRENT_VAR_FLAG})
	if(COMPILER_SUPPORTS_${CURRENT_VAR_FLAG})
		list(APPEND ${OUT_LIST_CFLAGS} ${IN_CFLAG})
	endif(COMPILER_SUPPORTS_${CURRENT_VAR_FLAG})
endmacro()

macro(ADD_SUPPORTED_CFLAGS LIST_CFLAGS OUT_LIST_CFLAGS)
	foreach(CURRENT_FLAG IN LISTS ${LIST_CFLAGS})
		ADD_SUPPORTED_CFLAG("${CURRENT_FLAG}" ${OUT_LIST_CFLAGS})
	endforeach()
endmacro()

macro(ADD_FIRST_SUPPORTED_CFLAG LIST_CFLAGS OUT_LIST_CFLAGS)
	foreach(CURRENT_FLAG IN LISTS ${LIST_CFLAGS})
		string(REGEX REPLACE "[^A-Za-z0-9_-]" "_" CURRENT_VAR_FLAG "${CURRENT_FLAG}")
		CHECK_C_COMPILER_FLAG("${CURRENT_FLAG}" COMPILER_SUPPORTS_${CURRENT_VAR_FLAG})
		if(COMPILER_SUPPORTS_${CURRENT_VAR_FLAG})
			list(APPEND ${OUT_LIST_CFLAGS} "${CURRENT_FLAG}")
			break()
		endif(COMPILER_SUPPORTS_${CURRENT_VAR_FLAG})
	endforeach()
endmacro()

macro(ADD_SUPPORTED_RELEASE_CFLAGS LIST_CFLAGS OUT_LIST_CFLAGS)
	set(SUPPORTED_CFLAGS)
	ADD_SUPPORTED_CFLAGS(${LIST_CFLAGS} SUPPORTED_CFLAGS)
	foreach(CURRENT_FLAG IN LISTS SUPPORTED_CFLAGS)
		if(NOT ${CMAKE_VERSION} VERSION_LESS "3.13.0")
			list(APPEND ${OUT_LIST_CFLAGS} $<$<CONFIG:Release>:${CURRENT_FLAG}>)
		elseif(CMAKE_BUILD_TYPE MATCHES "Release|release")
			list(APPEND ${OUT_LIST_CFLAGS} ${CURRENT_FLAG})
		endif()
	endforeach()
endmacro()

macro(ADD_BASE_DEFINITIONS BASE_DEFINITIONS)
	if(MSVC)
		#Character Set, CharacterSet="0", Not Set, ASCII/SBCS (Single Byte Character Set)
		list(APPEND ${BASE_DEFINITIONS} "-D_SBCS")

		#disable deprecation warnings about old functions like strcmp
		list(APPEND ${BASE_DEFINITIONS} "-D_CRT_SECURE_NO_WARNINGS")
		list(APPEND ${BASE_DEFINITIONS} "-D_CRT_NONSTDC_NO_DEPRECATE")
	endif()
	if(PLATFORM STREQUAL "openbsd"
		OR PLATFORM STREQUAL "freebsd"
		OR PLATFORM STREQUAL "openbsd")

		list(APPEND ${BASE_DEFINITIONS} "-DMAP_ANONYMOUS=MAP_ANON")
	endif()

	if(ARCH STREQUAL "darwin")
		list(APPEND ${BASE_DEFINITIONS} "-D_THREAD_SAFE=1")
	endif()

endmacro()

macro(ADD_GNUCC_BASE_CFLAGS BASE_CFLAGS)
	set(DEBUG_CFLAGS "-ggdb" "-O0")
	set(SUPPORTED_DEBUG_CFLAGS)
	ADD_SUPPORTED_CFLAGS(DEBUG_CFLAGS SUPPORTED_DEBUG_CFLAGS)
	list(APPEND ${BASE_CFLAGS} $<$<CONFIG:Debug>:${SUPPORTED_DEBUG_CFLAGS}>)

	if(GENERATE_DEPENDENCIES)
		ADD_SUPPORTED_CFLAG("-MMD" ${BASE_CFLAGS})
	endif()

	set(OPTIMIZEVM)
	list(APPEND OPTIMIZEVM "-O3")
	ADD_SUPPORTED_CFLAG("-fno-common" ${BASE_CFLAGS})
	ADD_SUPPORTED_CFLAG("-fno-strict-aliasing" ${BASE_CFLAGS})
	ADD_SUPPORTED_CFLAG("-pipe" ${BASE_CFLAGS})

	if(ARCH STREQUAL "x86")
		list(APPEND OPTIMIZEVM "-march=i586")
	endif()

	if(PLATFORM STREQUAL "sunos")
		if(ARCH STREQUAL "sparc")
			list(APPEND OPTIMIZEVM "-mno-faster-structs")
			list(APPEND OPTIMIZEVM "-mtune=ultrasparc3")
			list(APPEND OPTIMIZEVM "-mv8plus")
		endif()
	endif()

	if(PLATFORM STREQUAL "darwin")
		if(ARCH STREQUAL "ppc")
			ADD_SUPPORTED_CFLAG("-arch ppc" ${BASE_CFLAGS})
#			ADD_SUPPORTED_CFLAG("-maltivec" ${BASE_CFLAGS})
		endif()
		if(ARCH STREQUAL "ppc64")
			ADD_SUPPORTED_CFLAG("-arch ppc64" ${BASE_CFLAGS})
#			ADD_SUPPORTED_CFLAG("-maltivec" ${BASE_CFLAGS})
		endif()
		if(ARCH STREQUAL "x86")
			list(APPEND OPTIMIZEVM "-march=prescott")
			list(APPEND OPTIMIZEVM "-mfpmath=sse")
			ADD_SUPPORTED_CFLAG("-arch i386" ${BASE_CFLAGS})
			# x86 vm will crash without -mstackrealign since MMX instructions will be
			# used no matter what and they corrupt the frame pointer in VM calls
			ADD_SUPPORTED_CFLAG("-mstackrealign" ${BASE_CFLAGS})
		endif()
		if(ARCH STREQUAL "x86_64")
			list(APPEND OPTIMIZEVM "-mfpmath=sse")
			ADD_SUPPORTED_CFLAG("-arch x86_64" ${BASE_CFLAGS})
		endif()

	endif()

	ADD_SUPPORTED_RELEASE_CFLAGS(OPTIMIZEVM ${BASE_CFLAGS})


	set(WARNING_CFLAGS)
	list(APPEND WARNING_CFLAGS "-Wall")
	list(APPEND WARNING_CFLAGS "-Wimplicit")
	list(APPEND WARNING_CFLAGS "-Wstrict-prototypes")

	list(APPEND WARNING_CFLAGS "-Wformat=2")
	list(APPEND WARNING_CFLAGS "-Wno-format-zero-length")
	list(APPEND WARNING_CFLAGS "-Wformat-security")
	list(APPEND WARNING_CFLAGS "-Wno-format-nonliteral")
	list(APPEND WARNING_CFLAGS "-Wstrict-aliasing=2")
	list(APPEND WARNING_CFLAGS "-Wmissing-format-attribute")
	list(APPEND WARNING_CFLAGS "-Wdisabled-optimization")
	list(APPEND WARNING_CFLAGS "-Werror-implicit-function-declaration")

	ADD_SUPPORTED_CFLAGS(WARNING_CFLAGS ${BASE_CFLAGS})

endmacro()

macro(ADD_BASE_CFLAGS BASE_CFLAGS)
	if(MSVC)
		if(BUILD_STATIC_CRT)
			list(APPEND ${BASE_CFLAGS} "/MT")
		endif()
	elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
		ADD_GNUCC_BASE_CFLAGS(${BASE_CFLAGS})
	else()
		#other compilers like open64 support gcc cflags
		ADD_SUPPORTED_CFLAG("-O3" ${BASE_CFLAGS})
	endif()

endmacro()

#compiler definitions for renderer and vm libraries
macro(ADD_LIB_DEFINITIONS LIB_DEFINITIONS)

endmacro()

#compiler flags for renderer and vm libraries
macro(ADD_LIB_CFLAGS LIB_CFLAGS)
	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
		ADD_SUPPORTED_CFLAGS("-fvisibility=hidden" ${LIB_CFLAGS})

	endif()

endmacro()

macro(ADD_VM_DEFINITIONS VM_DEFINITIONS)
	ADD_BASE_DEFINITIONS(${VM_DEFINITIONS})
	ADD_LIB_DEFINITIONS(${VM_DEFINITIONS})

endmacro()

macro(ADD_VM_CFLAGS BASE_CFLAGS)
	ADD_BASE_CFLAGS(${BASE_CFLAGS})
	ADD_LIB_CFLAGS(${BASE_CFLAGS})

	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
		# According to http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=410555
		# -ffast-math will cause the client to die with SIGFPE on Alpha
		if(NOT ARCH STREQUAL "alpha")
			ADD_SUPPORTED_CFLAG("-ffast-math" ${BASE_CFLAGS})
		else()
			message(WARNING "Backwards compatibility might break without the -ffast-math compiler flag.")
		endif()
	endif()

	message(DEBUG "${BASE_CFLAGS}=${${BASE_CFLAGS}}")
endmacro()

macro(ADD_TOOLS_DEFINITIONS TOOLS_DEFINITIONS)
	ADD_BASE_DEFINITIONS(${TOOLS_DEFINITIONS})

endmacro()

macro(ADD_TOOLS_CFLAGS TOOLS_CFLAGS)
	if(MSVC)
		if(BUILD_STATIC_CRT)
			list(APPEND ${TOOLS_CFLAGS} "/MT")
		endif()
	endif()
	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
		set(TOOLS_OPTIMIZE)
		#Produce debugging information
		list(APPEND TOOLS_OPTIMIZE "-g")
		#enable all compiler warning messages
		list(APPEND TOOLS_OPTIMIZE "-Wall")
		#Causes the compiler to avoid assumptions regarding non-aliasing of objects of different types
		list(APPEND TOOLS_OPTIMIZE "-fno-strict-aliasing")

		ADD_SUPPORTED_RELEASE_CFLAGS(TOOLS_OPTIMIZE ${TOOLS_CFLAGS})

		if(GENERATE_DEPENDENCIES)
			ADD_SUPPORTED_CFLAG("-MMD" ${TOOLS_CFLAGS})
		endif()

	endif()

endmacro()

macro(ADD_RENDER_DEFINITIONS RENDER_DEFINITIONS)
	ADD_BASE_DEFINITIONS(${RENDER_DEFINITIONS})
	ADD_LIB_DEFINITIONS(${RENDER_DEFINITIONS})

endmacro()

macro(ADD_RENDER_CFLAGS RENDER_CFLAGS)
	ADD_BASE_CFLAGS(${RENDER_CFLAGS})
	ADD_LIB_CFLAGS(${RENDER_CFLAGS})

endmacro()

macro(ADD_SERVER_DEFINITIONS SERVER_DEFINITIONS)
	ADD_BASE_DEFINITIONS(${SERVER_DEFINITIONS})

endmacro()

macro(ADD_SERVER_CFLAGS SERVER_CFLAGS)
	ADD_BASE_CFLAGS(${SERVER_CFLAGS})

endmacro()

macro(ADD_CLIENT_DEFINITIONS CLIENT_DEFINITIONS)
	ADD_SERVER_DEFINITIONS(${CLIENT_DEFINITIONS})

endmacro()

macro(ADD_CLIENT_CFLAGS CLIENT_CFLAGS)
	ADD_SERVER_CFLAGS(${CLIENT_CFLAGS})

endmacro()

macro(ADD_INTERNAL_DEFINITIONS INTERNAL_DEFINITIONS)

endmacro()

macro(ADD_INTERNAL_CFLAGS INTERNAL_CFLAGS)
	if(MSVC)
		if(BUILD_STATIC_CRT)
			list(APPEND ${INTERNAL_CFLAGS} "/MT")
		endif()
	endif()

endmacro()
