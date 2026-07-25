if(NOT BUILD_CLIENT OR NOT BUILD_RENDERER_VULKAN)
    return()
endif()

include(utils/set_output_dirs)
include(renderer_common)

set(RENDERER_VULKAN_SOURCES
    ${SOURCE_DIR}/renderer_vulkan/glConfig.c
    ${SOURCE_DIR}/renderer_vulkan/matrix_multiplication.c
    ${SOURCE_DIR}/renderer_vulkan/RB_DrawNormals.c
    ${SOURCE_DIR}/renderer_vulkan/RB_DrawTris.c
    ${SOURCE_DIR}/renderer_vulkan/RB_ShowImages.c
    ${SOURCE_DIR}/renderer_vulkan/RB_SurfaceAnim.c
    ${SOURCE_DIR}/renderer_vulkan/R_DebugGraphics.c
    ${SOURCE_DIR}/renderer_vulkan/RE_RegisterModel.c
    ${SOURCE_DIR}/renderer_vulkan/R_FindShader.c
    ${SOURCE_DIR}/renderer_vulkan/R_ImageBMP.c
    ${SOURCE_DIR}/renderer_vulkan/R_ImageJPG.c
    ${SOURCE_DIR}/renderer_vulkan/R_ImagePCX.c
    ${SOURCE_DIR}/renderer_vulkan/R_ImagePNG.c
    ${SOURCE_DIR}/renderer_vulkan/R_ImageProcess.c
    ${SOURCE_DIR}/renderer_vulkan/R_ImageTGA.c
    ${SOURCE_DIR}/renderer_vulkan/R_LerpTag.c
    ${SOURCE_DIR}/renderer_vulkan/R_ListShader.c
    ${SOURCE_DIR}/renderer_vulkan/R_LoadImage.c
    ${SOURCE_DIR}/renderer_vulkan/R_LoadImage2.c
    ${SOURCE_DIR}/renderer_vulkan/R_LoadMD3.c
    ${SOURCE_DIR}/renderer_vulkan/R_LoadMDR.c
    ${SOURCE_DIR}/renderer_vulkan/R_ModelBounds.c
    ${SOURCE_DIR}/renderer_vulkan/R_Parser.c
    ${SOURCE_DIR}/renderer_vulkan/R_PortalPlane.c
    ${SOURCE_DIR}/renderer_vulkan/R_PrintMat.c
    ${SOURCE_DIR}/renderer_vulkan/R_StretchRaw.c
    ${SOURCE_DIR}/renderer_vulkan/ref_import.c
    ${SOURCE_DIR}/renderer_vulkan/render_export.c
    ${SOURCE_DIR}/renderer_vulkan/tr_animation.c
    ${SOURCE_DIR}/renderer_vulkan/tr_backend.c
    ${SOURCE_DIR}/renderer_vulkan/tr_bsp.c
    ${SOURCE_DIR}/renderer_vulkan/tr_cmds.c
    ${SOURCE_DIR}/renderer_vulkan/tr_Cull.c
    ${SOURCE_DIR}/renderer_vulkan/tr_curve.c
    ${SOURCE_DIR}/renderer_vulkan/tr_cvar.c
    ${SOURCE_DIR}/renderer_vulkan/tr_flares.c
    ${SOURCE_DIR}/renderer_vulkan/tr_fog.c
    ${SOURCE_DIR}/renderer_vulkan/tr_fonts.c
    ${SOURCE_DIR}/renderer_vulkan/tr_globals.c
    ${SOURCE_DIR}/renderer_vulkan/tr_image.c
    ${SOURCE_DIR}/renderer_vulkan/tr_init.c
    ${SOURCE_DIR}/renderer_vulkan/tr_light.c
    ${SOURCE_DIR}/renderer_vulkan/tr_main.c
    ${SOURCE_DIR}/renderer_vulkan/tr_marks.c
    ${SOURCE_DIR}/renderer_vulkan/tr_mesh.c
    ${SOURCE_DIR}/renderer_vulkan/tr_model.c
    ${SOURCE_DIR}/renderer_vulkan/tr_model_iqm.c
    ${SOURCE_DIR}/renderer_vulkan/tr_noise.c
    ${SOURCE_DIR}/renderer_vulkan/tr_scene.c
    ${SOURCE_DIR}/renderer_vulkan/tr_shade.c
    ${SOURCE_DIR}/renderer_vulkan/tr_shade_calc.c
    ${SOURCE_DIR}/renderer_vulkan/tr_shader.c
    ${SOURCE_DIR}/renderer_vulkan/tr_shadows.c
    ${SOURCE_DIR}/renderer_vulkan/tr_sky.c
    ${SOURCE_DIR}/renderer_vulkan/tr_surface.c
    ${SOURCE_DIR}/renderer_vulkan/tr_world.c
    ${SOURCE_DIR}/renderer_vulkan/vk_cmd.c
    ${SOURCE_DIR}/renderer_vulkan/vk_create_window_SDL.c
    ${SOURCE_DIR}/renderer_vulkan/vk_depth_attachment.c
    ${SOURCE_DIR}/renderer_vulkan/vk_frame.c
    ${SOURCE_DIR}/renderer_vulkan/vk_image.c
    ${SOURCE_DIR}/renderer_vulkan/vk_image_sampler2.c
    ${SOURCE_DIR}/renderer_vulkan/vk_init.c
    ${SOURCE_DIR}/renderer_vulkan/vk_instance.c
    ${SOURCE_DIR}/renderer_vulkan/vk_pipelines.c
    ${SOURCE_DIR}/renderer_vulkan/vk_screenshot.c
    ${SOURCE_DIR}/renderer_vulkan/vk_shade_geometry.c
    ${SOURCE_DIR}/renderer_vulkan/vk_shaders.c
    ${SOURCE_DIR}/renderer_vulkan/vk_swapchain.c
)

# Pre-compiled SPIR-V shaders embedded as C byte arrays
set(RENDERER_VULKAN_SHADER_SOURCES
    ${SOURCE_DIR}/renderer_vulkan/shaders/Compiled/multi_texture_clipping_plane_vert.c
    ${SOURCE_DIR}/renderer_vulkan/shaders/Compiled/multi_texture_frag.c
    ${SOURCE_DIR}/renderer_vulkan/shaders/Compiled/multi_texture_vert.c
    ${SOURCE_DIR}/renderer_vulkan/shaders/Compiled/single_texture_clipping_plane_vert.c
    ${SOURCE_DIR}/renderer_vulkan/shaders/Compiled/single_texture_frag.c
    ${SOURCE_DIR}/renderer_vulkan/shaders/Compiled/single_texture_vert.c
)

set(RENDERER_VULKAN_BASENAME renderer_vulkan)
set(RENDERER_VULKAN_BINARY ${RENDERER_VULKAN_BASENAME})

# The Vulkan renderer is self-contained: it has its own image loaders, font
# rendering, noise functions, and Com_Printf/Com_Error wrappers (in ref_import.c)
# rather than using renderercommon.  It only needs puff.c for PNG decompression.
list(APPEND RENDERER_VULKAN_BINARY_SOURCES
    ${RENDERER_VULKAN_SOURCES}
    ${RENDERER_VULKAN_SHADER_SOURCES}
    ${SOURCE_DIR}/renderercommon/puff.c
    ${RENDERER_LIBRARY_SOURCES})

# Vulkan headers: use bundled copy or system-provided headers.
# The renderer loads Vulkan dynamically via SDL, so we only need the headers
# at compile time -- no link-time dependency on libvulkan.
if(USE_INTERNAL_VULKAN_HEADERS)
    set(VULKAN_INCLUDE_DIRS ${SOURCE_DIR}/renderer_vulkan)
else()
    find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h)
    if(NOT VULKAN_INCLUDE_DIR)
        message(FATAL_ERROR "System Vulkan headers not found. "
            "Install vulkan-headers (e.g. 'brew install vulkan-headers' on macOS, "
            "'sudo apt install libvulkan-dev' on Debian/Ubuntu, "
            "'sudo dnf install vulkan-headers' on Fedora) "
            "or set USE_INTERNAL_VULKAN_HEADERS=ON to use the bundled copy.")
    endif()
    set(VULKAN_INCLUDE_DIRS ${VULKAN_INCLUDE_DIR})
endif()

if(USE_RENDERER_DLOPEN)
    # The Vulkan renderer's ref_import.c already provides Com_Printf and
    # Com_Error, so we cannot include tr_subs.c from DYNAMIC_RENDERER_SOURCES.
    # Include only q_shared.c and q_math.c directly.
    list(APPEND RENDERER_VULKAN_BINARY_SOURCES
        ${SOURCE_DIR}/qcommon/q_shared.c
        ${SOURCE_DIR}/qcommon/q_math.c)

    add_library(${RENDERER_VULKAN_BINARY} SHARED ${RENDERER_VULKAN_BINARY_SOURCES})

    target_link_libraries(      ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_LIBRARIES})
    target_include_directories( ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS}
                                                                   ${VULKAN_INCLUDE_DIRS})
    target_compile_definitions( ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_DEFINITIONS})
    target_compile_options(     ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_link_options(        ${RENDERER_VULKAN_BINARY} PRIVATE ${RENDERER_LINK_OPTIONS})

    set_output_dirs(${RENDERER_VULKAN_BINARY})
endif()
