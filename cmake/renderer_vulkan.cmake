if(NOT BUILD_CLIENT OR NOT BUILD_RENDERER_VULKAN)
    return()
endif()

include(utils/set_output_dirs)
include(renderer_common)

set(RENDERER_VULKAN_LIBRARY_SOURCES ${RENDERER_LIBRARY_SOURCES})
set(RENDERER_VULKAN_DYNAMIC_SOURCES ${DYNAMIC_RENDERER_SOURCES})

list(FILTER RENDERER_VULKAN_DYNAMIC_SOURCES EXCLUDE REGEX "tr_subs\\.c$")
list(FILTER RENDERER_VULKAN_LIBRARY_SOURCES EXCLUDE REGEX "tr_subs\\.c$")

set(RENDERER_VULKAN_SOURCES
    ${SOURCE_DIR}/renderer/vulkan/matrix_multiplication.c
    ${SOURCE_DIR}/renderer/vulkan/tr_globals.c
    ${SOURCE_DIR}/renderer/vulkan/tr_cvar.c
    ${SOURCE_DIR}/renderer/vulkan/tr_animation.c
    ${SOURCE_DIR}/renderer/vulkan/tr_bsp.c
    ${SOURCE_DIR}/renderer/vulkan/tr_cmds.c
    ${SOURCE_DIR}/renderer/vulkan/tr_curve.c
    ${SOURCE_DIR}/renderer/vulkan/tr_fonts.c
    ${SOURCE_DIR}/renderer/vulkan/tr_image.c
    ${SOURCE_DIR}/renderer/vulkan/R_FindShader.c
    ${SOURCE_DIR}/renderer/vulkan/R_ListShader.c
    ${SOURCE_DIR}/renderer/vulkan/R_ImageProcess.c
    ${SOURCE_DIR}/renderer/vulkan/tr_init.c
    ${SOURCE_DIR}/renderer/vulkan/tr_light.c
    ${SOURCE_DIR}/renderer/vulkan/tr_main.c
    ${SOURCE_DIR}/renderer/vulkan/tr_marks.c
    ${SOURCE_DIR}/renderer/vulkan/tr_mesh.c
    ${SOURCE_DIR}/renderer/vulkan/tr_model.c
    ${SOURCE_DIR}/renderer/vulkan/tr_model_iqm.c
    ${SOURCE_DIR}/renderer/vulkan/RE_RegisterModel.c
    ${SOURCE_DIR}/renderer/vulkan/R_ModelBounds.c
    ${SOURCE_DIR}/renderer/vulkan/R_LoadMD3.c
    ${SOURCE_DIR}/renderer/vulkan/R_LoadMDR.c
    ${SOURCE_DIR}/renderer/vulkan/R_LerpTag.c
    ${SOURCE_DIR}/renderer/vulkan/tr_noise.c
    ${SOURCE_DIR}/renderer/vulkan/tr_scene.c
    ${SOURCE_DIR}/renderer/vulkan/tr_shade.c
    ${SOURCE_DIR}/renderer/vulkan/tr_shade_calc.c
    ${SOURCE_DIR}/renderer/vulkan/tr_shader.c
    ${SOURCE_DIR}/renderer/vulkan/tr_shadows.c
    ${SOURCE_DIR}/renderer/vulkan/tr_sky.c
    ${SOURCE_DIR}/renderer/vulkan/tr_surface.c
    ${SOURCE_DIR}/renderer/vulkan/tr_flares.c
    ${SOURCE_DIR}/renderer/vulkan/tr_fog.c
    ${SOURCE_DIR}/renderer/vulkan/tr_world.c

    ${SOURCE_DIR}/renderer/vulkan/vk_instance.c
    ${SOURCE_DIR}/renderer/vulkan/vk_init.c
    ${SOURCE_DIR}/renderer/vulkan/vk_cmd.c
    ${SOURCE_DIR}/renderer/vulkan/vk_image.c
    ${SOURCE_DIR}/renderer/vulkan/vk_image_sampler2.c
    ${SOURCE_DIR}/renderer/vulkan/vk_pipelines.c
    ${SOURCE_DIR}/renderer/vulkan/vk_frame.c
    ${SOURCE_DIR}/renderer/vulkan/vk_swapchain.c
    ${SOURCE_DIR}/renderer/vulkan/vk_screenshot.c
    ${SOURCE_DIR}/renderer/vulkan/vk_shade_geometry.c
    ${SOURCE_DIR}/renderer/vulkan/vk_depth_attachment.c
    ${SOURCE_DIR}/renderer/vulkan/vk_shaders.c

    ${SOURCE_DIR}/renderer/vulkan/R_StretchRaw.c
    ${SOURCE_DIR}/renderer/vulkan/R_DebugGraphics.c
    ${SOURCE_DIR}/renderer/vulkan/RB_ShowImages.c
    ${SOURCE_DIR}/renderer/vulkan/RB_DrawNormals.c
    ${SOURCE_DIR}/renderer/vulkan/RB_DrawTris.c
    ${SOURCE_DIR}/renderer/vulkan/RB_SurfaceAnim.c
    ${SOURCE_DIR}/renderer/vulkan/tr_backend.c
    ${SOURCE_DIR}/renderer/vulkan/tr_Cull.c
    ${SOURCE_DIR}/renderer/vulkan/glConfig.c
    ${SOURCE_DIR}/renderer/vulkan/R_Parser.c
    ${SOURCE_DIR}/renderer/vulkan/R_PortalPlane.c
    ${SOURCE_DIR}/renderer/vulkan/R_PrintMat.c

    ${SOURCE_DIR}/renderer/vulkan/R_LoadImage2.c
    ${SOURCE_DIR}/renderer/vulkan/R_LoadImage.c
    ${SOURCE_DIR}/renderer/vulkan/R_ImageJPG.c
    ${SOURCE_DIR}/renderer/vulkan/R_ImageTGA.c
    ${SOURCE_DIR}/renderer/vulkan/R_ImagePNG.c
    ${SOURCE_DIR}/renderer/vulkan/R_ImageBMP.c
    ${SOURCE_DIR}/renderer/vulkan/R_ImagePCX.c

    ${SOURCE_DIR}/renderer/vulkan/ref_import.c
    ${SOURCE_DIR}/renderer/vulkan/render_export.c

    ${SOURCE_DIR}/renderer/vulkan/vk_create_window_SDL.c
    ${SOURCE_DIR}/qcommon/puff.c
)

# Auto-include the generated SPIR-V C stubs
file(GLOB RENDERER_VULKAN_SHADER_C_SOURCES
    ${SOURCE_DIR}/renderer/vulkan/shaders/Compiled/*.c
)

set(RENDERER_VULKAN_BASENAME renderer_vulkan)
set(RENDERER_VULKAN_BINARY  ${RENDERER_VULKAN_BASENAME})

# IMPORTANT: Do *not* pull in RENDERER_COMMON_SOURCES or SDL_RENDERER_SOURCES here.
list(APPEND RENDERER_VULKAN_BINARY_SOURCES
    ${RENDERER_VULKAN_SOURCES}
    ${RENDERER_VULKAN_SHADER_C_SOURCES}
    ${RENDERER_VULKAN_LIBRARY_SOURCES}
)

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_VULKAN_BINARY_SOURCES ${RENDERER_VULKAN_DYNAMIC_SOURCES})

    add_library(${RENDERER_VULKAN_BINARY} SHARED ${RENDERER_VULKAN_BINARY_SOURCES})

    target_link_libraries(      ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_LIBRARIES})
    target_include_directories( ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS})
    target_compile_definitions( ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_DEFINITIONS})
    target_compile_options(     ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_link_options(        ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_LINK_OPTIONS})

    set_output_dirs(${RENDERER_VULKAN_BINARY})
endif()

