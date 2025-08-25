if(NOT BUILD_CLIENT OR NOT BUILD_RENDERER_GL2)
    return()
endif()

include(utils/set_output_dirs)
include(renderer_common)

set(RENDERER_GL2_SOURCES
    ${SOURCE_DIR}/renderergl2/tr_animation.c
    ${SOURCE_DIR}/renderergl2/tr_backend.c
    ${SOURCE_DIR}/renderergl2/tr_bsp.c
    ${SOURCE_DIR}/renderergl2/tr_cmds.c
    ${SOURCE_DIR}/renderergl2/tr_curve.c
    ${SOURCE_DIR}/renderergl2/tr_dsa.c
    ${SOURCE_DIR}/renderergl2/tr_extramath.c
    ${SOURCE_DIR}/renderergl2/tr_extensions.c
    ${SOURCE_DIR}/renderergl2/tr_fbo.c
    ${SOURCE_DIR}/renderergl2/tr_flares.c
    ${SOURCE_DIR}/renderergl2/tr_glsl.c
    ${SOURCE_DIR}/renderergl2/tr_image.c
    ${SOURCE_DIR}/renderergl2/tr_image_dds.c
    ${SOURCE_DIR}/renderergl2/tr_init.c
    ${SOURCE_DIR}/renderergl2/tr_light.c
    ${SOURCE_DIR}/renderergl2/tr_main.c
    ${SOURCE_DIR}/renderergl2/tr_marks.c
    ${SOURCE_DIR}/renderergl2/tr_mesh.c
    ${SOURCE_DIR}/renderergl2/tr_model.c
    ${SOURCE_DIR}/renderergl2/tr_model_iqm.c
    ${SOURCE_DIR}/renderergl2/tr_postprocess.c
    ${SOURCE_DIR}/renderergl2/tr_scene.c
    ${SOURCE_DIR}/renderergl2/tr_shade.c
    ${SOURCE_DIR}/renderergl2/tr_shade_calc.c
    ${SOURCE_DIR}/renderergl2/tr_shader.c
    ${SOURCE_DIR}/renderergl2/tr_shadows.c
    ${SOURCE_DIR}/renderergl2/tr_sky.c
    ${SOURCE_DIR}/renderergl2/tr_surface.c
    ${SOURCE_DIR}/renderergl2/tr_vbo.c
    ${SOURCE_DIR}/renderergl2/tr_world.c
)

file(GLOB RENDERER_GL2_SHADER_SOURCES ${SOURCE_DIR}/renderergl2/glsl/*.glsl)

set(SHADERS_DIR ${CMAKE_BINARY_DIR}/shaders.dir)
file(MAKE_DIRECTORY ${SHADERS_DIR})

foreach(SHADER_FILE IN LISTS RENDERER_GL2_SHADER_SOURCES)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_C_FILE ${SHADERS_DIR}/${SHADER_NAME}.c)

    string(REPLACE "${CMAKE_BINARY_DIR}/" "" SHADER_C_FILE_COMMENT ${SHADER_C_FILE})

    add_custom_command(
        OUTPUT ${SHADER_C_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${SHADER_FILE}
            -DOUTPUT_FILE=${SHADER_C_FILE}
            -DSHADER_NAME=${SHADER_NAME}
            -P ${CMAKE_SOURCE_DIR}/cmake/utils/stringify_shader.cmake
        DEPENDS ${SHADER_FILE}
        COMMENT "Stringify shader ${SHADER_C_FILE_COMMENT}")

    list(APPEND RENDERER_GL2_SHADER_C_SOURCES ${SHADER_C_FILE})
endforeach()

set(RENDERER_GL2_BASENAME renderer_opengl2)
set(RENDERER_GL2_BINARY ${RENDERER_GL2_BASENAME})

list(APPEND RENDERER_GL2_BINARY_SOURCES
    ${RENDERER_COMMON_SOURCES}
    ${RENDERER_GL2_SOURCES}
    ${RENDERER_GL2_SHADER_C_SOURCES}
    ${SDL_RENDERER_SOURCES}
    ${RENDERER_LIBRARY_SOURCES})

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_GL2_BINARY_SOURCES ${DYNAMIC_RENDERER_SOURCES})

    add_library(${RENDERER_GL2_BINARY} SHARED ${RENDERER_GL2_BINARY_SOURCES})

    target_link_libraries(      ${RENDERER_GL2_BINARY} PRIVATE ${RENDERER_LIBRARIES})
    target_include_directories( ${RENDERER_GL2_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS})
    target_compile_definitions( ${RENDERER_GL2_BINARY} PRIVATE ${RENDERER_DEFINITIONS})
    target_compile_options(     ${RENDERER_GL2_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_link_options(        ${RENDERER_GL2_BINARY} PRIVATE ${RENDERER_LINK_OPTIONS})

    set_output_dirs(${RENDERER_GL2_BINARY})
    install(TARGETS ${RENDERER_GL2_BINARY} LIBRARY DESTINATION .)
endif()

