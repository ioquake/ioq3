
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR)

if(NOT DEFINED COMPILE_ARCH)
	if(SYSTEM_PROCESSOR STREQUAL "unknown")
		message(FATAL_ERROR "CMAKE_SYSTEM_PROCESSOR is unknown! Check your CMake generator or toolchain.")
	endif()
endif()

#converting arch types to gcc -march types (also -arch on Darwin)
#https://stackoverflow.com/questions/70475665/what-are-the-possible-values-of-cmake-system-processor
if(SYSTEM_PROCESSOR MATCHES "powerpc")
	set(COMPILE_ARCH "ppc")
elseif(SYSTEM_PROCESSOR MATCHES "powerpc64|ppc64le")
	set(COMPILE_ARCH "ppc64")
elseif(SYSTEM_PROCESSOR MATCHES "axp")
	set(COMPILE_ARCH "alpha")
elseif(SYSTEM_PROCESSOR MATCHES "i386|i686|(i.86)|i86pc|x86pc|x86at")
	set(COMPILE_ARCH "x86")
elseif(SYSTEM_PROCESSOR MATCHES "x86-64|ia64|em64t|amd64|x64")
	set(COMPILE_ARCH "x86_64")
elseif(SYSTEM_PROCESSOR MATCHES "armv8b|armv8l|aarch64_be|aarch64")
	set(COMPILE_ARCH "arm64")
elseif(SYSTEM_PROCESSOR MATCHES "armv7l|armv6l")
	set(COMPILE_ARCH "arm")
endif()


if(NOT DEFINED COMPILE_ARCH)
	set(COMPILE_ARCH "${SYSTEM_PROCESSOR}")
endif()
if(NOT DEFINED ARCH)
	string(TOLOWER "${COMPILE_ARCH}" ARCH)
endif()

set(COMPILE_PLATFORM "${CMAKE_SYSTEM_NAME}")
if(NOT DEFINED PLATFORM)
	string(TOLOWER "${COMPILE_PLATFORM}" PLATFORM)
endif()

if(NOT PLATFORM STREQUAL COMPILE_PLATFORM)
	set(CROSS_COMPILING TRUE)
else()
	set(CROSS_COMPILING FALSE)
	if(NOT ARCH STREQUAL COMPILE_ARCH)
		set(CROSS_COMPILING TRUE)
	endif()
endif()



if(NOT DEFINED DEFAULT_BIN_EXT)
	if(ARCH)
		set(DEFAULT_BIN_EXT ".${ARCH}")
	endif(ARCH)
endif()
