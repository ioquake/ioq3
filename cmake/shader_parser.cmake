if(NOT BUILD_SHADER_PARSER)
    return()
endif()

include(utils/set_output_dirs)

set(SHADER_PARSER_SOURCES
    ${SOURCE_DIR}/tools/shader_parser/shader_parser_main.c
    ${SOURCE_DIR}/tools/shader_parser/shader_parser_env.c
    ${SOURCE_DIR}/tools/shader_parser/shader_parser_metal.cpp
    ${SOURCE_DIR}/renderergl2/tr_shader.c
    ${SOURCE_DIR}/renderermetal/tr_shader.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
)

add_executable(shader_parser ${SHADER_PARSER_SOURCES})

target_include_directories(shader_parser PRIVATE
    ${SOURCE_DIR}
    ${SOURCE_DIR}/thirdparty/SDL2-2.32.8/include)
target_compile_definitions(shader_parser PRIVATE
    SHADER_PARSER_TOOL
    USE_INTERNAL_SDL_HEADERS)

set_output_dirs(shader_parser)

install(TARGETS shader_parser
    RUNTIME DESTINATION .)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_custom_target(shader_parity_tests
        COMMAND ${Python3_EXECUTABLE}
                ${SOURCE_DIR}/tools/shader_parser/run_parity.py
                --shader-parser $<TARGET_FILE:shader_parser>
                --scripts ${SOURCE_DIR}/shaders/scripts
                --filters textures/test/*
        DEPENDS shader_parser
        WORKING_DIRECTORY ${SOURCE_DIR}
        COMMENT "Comparing Metal shader parser output against GL2")
else()
    message(STATUS "Python3 not found; shader_parity_tests target disabled")
endif()
