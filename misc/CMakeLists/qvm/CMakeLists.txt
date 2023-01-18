project("qvm")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/default_options.cmake")

set(BASEGAME "${DEFAULT_BASEGAME}" CACHE STRING "")
set(MISSIONPACK "${DEFAULT_MISSIONPACK}" CACHE STRING "")

add_subdirectory("${BASEGAME}")
add_subdirectory("${MISSIONPACK}")
