include("${CMAKE_SOURCE_DIR}/cmake/SDL2.cmake")

find_package(OpenGL)

set(PARENT_DIR ${CMAKE_SOURCE_DIR}/src)
include_directories(
    ${PARENT_DIR}/jpeg-8c
    ${PARENT_DIR}/renderercommon 
    ${PARENT_DIR}/nettle-3.3
    ${SDL2_INCLUDE_DIRS} 
    )

set(GLSL_SHADERS
    glsl/bokeh_fp.glsl
    glsl/bokeh_vp.glsl
    glsl/calclevels4x_fp.glsl
    glsl/calclevels4x_vp.glsl
    glsl/depthblur_fp.glsl
    glsl/depthblur_vp.glsl
    glsl/dlight_fp.glsl
    glsl/dlight_vp.glsl
    glsl/down4x_fp.glsl
    glsl/down4x_vp.glsl
    glsl/fogpass_fp.glsl
    glsl/fogpass_vp.glsl
    glsl/generic_fp.glsl
    glsl/generic_vp.glsl
    glsl/lightall_fp.glsl
    glsl/lightall_vp.glsl
    glsl/pshadow_fp.glsl
    glsl/pshadow_vp.glsl
    glsl/shadowfill_fp.glsl
    glsl/shadowfill_vp.glsl
    glsl/shadowmask_fp.glsl
    glsl/shadowmask_vp.glsl
    glsl/ssao_fp.glsl
    glsl/ssao_vp.glsl
    glsl/texturecolor_fp.glsl
    glsl/texturecolor_vp.glsl
    glsl/tonemap_fp.glsl
    glsl/tonemap_vp.glsl
    )

# Converts ^^^GSLS shaders^^^ into const char* C-strings.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/glsl/)
foreach(shader ${GLSL_SHADERS})
    set(in ${CMAKE_CURRENT_SOURCE_DIR}/${shader})

    get_filename_component(out ${in} NAME_WE)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/glsl/${out}.c)

    get_filename_component(shader ${shader} NAME_WE)
    message(STATUS "-- Generating embeded GLSL ${shader}")

    file(READ ${in} contents HEX)
    file(WRITE ${out} "const char *fallbackShader_${shader} = \"")

    string(LENGTH "${contents}" contents_length)
    math(EXPR contents_length "${contents_length} - 1")

    foreach(iter RANGE 0 ${contents_length} 2)
      string(SUBSTRING ${contents} ${iter} 2 line)
      file(APPEND "${out}" "\\x${line}")
    endforeach()
    file(APPEND "${out}" "\";\n")

    list(APPEND renderergl2_SHADERS "${out}")
    set_source_files_properties(${out} PROPERTIES GENERATED TRUE)
endforeach(shader)

set(renderergl2_SRCS
    ${renderergl2_SHADERS}
    tr_local.h
    tr_dsa.h
    tr_extramath.h
    tr_extratypes.h
    tr_fbo.h
    tr_postprocess.h
    tr_animation.c
    tr_backend.c
    tr_bsp.c
    tr_cmds.c
    tr_curve.c
    tr_dsa.c
    tr_extensions.c
    tr_extramath.c
    tr_fbo.c
    tr_flares.c
    tr_glsl.c
    tr_image.c
    tr_image_dds.c
    tr_init.c
    tr_light.c
    tr_main.c
    tr_marks.c
    tr_mesh.c
    tr_model.c
    tr_model_iqm.c
    tr_postprocess.c
    tr_scene.c
    tr_shade.c
    tr_shade_calc.c
    tr_shader.c
    tr_shadows.c
    tr_sky.c
    tr_subs.c
    tr_surface.c
    tr_vbo.c
    tr_world.c
    ${CMAKE_SOURCE_DIR}/code/qcommon/q_shared.c
    ${CMAKE_SOURCE_DIR}/code/qcommon/puff.c
    ${CMAKE_SOURCE_DIR}/code/qcommon/q_math.c
    )

if(NOT USE_RENDERER_DLOPEN)

    add_library(
        renderergl2 STATIC
        ${renderergl2_SRCS})

    target_link_libraries(
        renderergl2 renderercommon
        ${SDL2_LIBRARIES})

else(NOT USE_RENDERER_DLOPEN)

    add_library(
        renderergl2 SHARED
        ${renderergl2_SRCS}
        )

        target_link_libraries(
        renderergl2 
        renderercommon
        ${FRAMEWORKS}
        ${OPENGL_LIBRARIES}
        ${SDL2_LIBRARIES}
        )
    add_custom_command(
        TARGET renderergl2
        POST_BUILD COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/librenderergl2${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/renderer_opengl2${CMAKE_SHARED_LIBRARY_SUFFIX}
        )

endif(NOT USE_RENDERER_DLOPEN)

