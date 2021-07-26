set(SRCS
	tr_altivec.c
	tr_animation.c
	tr_backend.c
	tr_bsp.c
	tr_cmds.c
	tr_curve.c
	tr_flares.c
	tr_image.c
	tr_init.c
	tr_light.c
	tr_local.h
	tr_main.c
	tr_marks.c
	tr_mesh.c
	tr_model.c
	tr_model_iqm.c
	tr_scene.c
	tr_shade.c
	tr_shade_calc.c
	tr_shader.c
	tr_shadows.c
	tr_sky.c
	tr_surface.c
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

if (USE_RENDERER_DLOPEN)
	list(APPEND SRCS
		../qcommon/q_shared.c
		../qcommon/puff.c
		../qcommon/q_math.c
		tr_subs.c
	)
endif()

project(renderer_opengl1)
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
	list(APPEND RENDERER_DEFINES USE_RENDERER_DLOPEN)
endif()
if (RENDERER_DEFINES)
	target_compile_definitions(${PROJECT_NAME} PRIVATE ${RENDERER_DEFINES})
endif()
