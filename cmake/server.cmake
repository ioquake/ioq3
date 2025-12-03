
if(NOT BUILD_SERVER)
    return()
endif()

include(utils/set_output_dirs)
include(shared_sources)

set(NULL_SOURCES
    ${SOURCE_DIR}/null/null_client.c
    ${SOURCE_DIR}/null/null_input.c
    ${SOURCE_DIR}/null/null_snddma.c
)

set(SERVER_BINARY ${SERVER_NAME})

list(APPEND SERVER_DEFINITIONS DEDICATED)
list(APPEND SERVER_DEFINITIONS BOTLIB)

if(BUILD_STANDALONE)
    list(APPEND SERVER_DEFINITIONS STANDALONE)
endif()

if(USE_VOIP)
    list(APPEND SERVER_DEFINITIONS USE_VOIP)
endif()

list(APPEND SERVER_BINARY_SOURCES
    ${SERVER_SOURCES}
    ${NULL_SOURCES}
    ${COMMON_SOURCES}
    ${BOTLIB_SOURCES}
    ${SYSTEM_SOURCES}
    ${ASM_SOURCES}
    ${SERVER_PLATFORM_SOURCES}
    ${SERVER_LIBRARY_SOURCES})

add_executable(${SERVER_BINARY} ${SERVER_EXECUTABLE_OPTIONS} ${SERVER_BINARY_SOURCES})

target_include_directories(     ${SERVER_BINARY} PRIVATE ${SERVER_INCLUDE_DIRS})
target_compile_definitions(     ${SERVER_BINARY} PRIVATE ${SERVER_DEFINITIONS})
target_compile_options(         ${SERVER_BINARY} PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_libraries(          ${SERVER_BINARY} PRIVATE ${COMMON_LIBRARIES} ${SERVER_LIBRARIES})
target_link_options(            ${SERVER_BINARY} PRIVATE ${SERVER_LINK_OPTIONS})

set_output_dirs(${SERVER_BINARY})

if(POST_SERVER_CONFIGURE_FUNCTION)
    cmake_language(CALL ${POST_SERVER_CONFIGURE_FUNCTION})
endif()
