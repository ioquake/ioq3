set(SRCS
	tr_animation.c
	tr_backend.c
	tr_bsp.c
	tr_cmds.c
	tr_curve.c
	tr_dsa.c tr_dsa.h
	tr_extramath.c tr_extramath.h
	tr_extensions.c
	tr_extratypes.h
	tr_fbo.c tr_fbo.h
	tr_flares.c
	tr_glsl.c
	tr_image.c
	tr_image_dds.c
	tr_init.c
	tr_light.c
	tr_local.h
	tr_main.c
	tr_marks.c
	tr_mesh.c
	tr_model.c
	tr_model_iqm.c
	tr_postprocess.c tr_postprocess.h
	tr_scene.c
	tr_shade.c
	tr_shade_calc.c
	tr_shader.c
	tr_shadows.c
	tr_sky.c
	tr_surface.c
	tr_vbo.c
	tr_world.c

	../renderercommon/iqm.h
	../renderercommon/qgl.h
	../renderercommon/tr_common.h
	../renderercommon/tr_font.c
	../renderercommon/tr_image_bmp.c
	../renderercommon/tr_image_jpg.c
	../renderercommon/tr_image_pcx.c
	../renderercommon/tr_image_png.c
	../renderercommon/tr_image_tga.c
	../renderercommon/tr_noise.c
	../renderercommon/tr_public.h
	../renderercommon/tr_types.h

	../sdl/sdl_gamma.c
	../sdl/sdl_glimp.c
)

set(SHADER_SRCS
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
list(APPEND SRCS ${SHADER_SRCS})

foreach (shader ${SHADER_SRCS})
	get_filename_component(shaderfile ${shader} NAME_WE)
	list(APPEND SRCS ${CMAKE_CURRENT_BINARY_DIR}/${shaderfile}.c)
	set_source_files_properties(${shaderfile}.c PROPERTIES GENERATED TRUE)
	add_custom_command(OUTPUT ${shaderfile}.c
		DEPENDS stringify ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
		COMMAND stringify ${CMAKE_CURRENT_SOURCE_DIR}/${shader} ${CMAKE_CURRENT_BINARY_DIR}/${shaderfile}.c
		COMMENT "Generate c source for ${shader}: ${shaderfile}.c"
	)
endforeach()

if (USE_RENDERER_DLOPEN)
	list(APPEND SRCS
		../qcommon/q_shared.c
		../qcommon/puff.c
		../qcommon/q_math.c
		tr_subs.c
	)
endif()

project(renderer_opengl2)
q3_add_library(TARGET ${PROJECT_NAME} SRCS ${SRCS})
set(LIBS jpeg ${SDL2_LIBRARIES})
if (MSVC)
	list(APPEND LIBS ws2_32 winmm psapi gdi32 ole32)
elseif (APPLE)
	list(APPEND LIBS "-framework OpenGL")
endif()
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})

set(RENDERER_DEFINES)
if (USE_RENDERER_DLOPEN)
	list(APPEND RENDERER_DEFINES -DUSE_RENDERER_DLOPEN)
endif()
if (RENDERER_DEFINES)
	target_compile_definitions(${PROJECT_NAME} PRIVATE ${RENDERER_DEFINES})
endif()
