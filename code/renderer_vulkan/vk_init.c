/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2017-2025 Artem Kharytoniuk
Copyright (C) 2019-2024 Sui Jingfeng and the vkQuake3 contributors

This file is part of the Vulkan renderer of ioquake3, ported from vkQuake3
(https://github.com/suijingfeng/vkQuake3), whose Vulkan code is a C
conversion of the Vulkan backend of Quake-III-Arena-Kenny-Edition
(https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_frame.h"
#include "vk_cmd.h"
#include "vk_depth_attachment.h"
#include "vk_pipelines.h"
#include "vk_shade_geometry.h"
#include "vk_shaders.h"
#include "glConfig.h"
#include "tr_backend.h"

// vk_init have nothing to do with tr_init
// vk_instance should be small
// 
// After calling this function we get fully functional vulkan subsystem.
void vk_initialize(void)
{
    // This function is responsible for initializing a valid Vulkan subsystem.
    vk_createWindow();

    vk_getProcAddress(); 
 
	// Swapchain. vk.physical_device required to be init. 
	vk_createSwapChain(vk.device, vk.surface, vk.surface_format);

	// Sync primitives.
    vk_create_sync_primitives();

	// we have to create a command pool before we can create command buffers
    // command pools manage the memory that is used to store the buffers and
    // command buffers are allocated from them.
    ri.Printf(PRINT_DEVELOPER, " Create command pool: vk.command_pool \n");
    vk_create_command_pool(&vk.command_pool);
    
    ri.Printf(PRINT_DEVELOPER, " Create command buffer: vk.command_buffer \n");
    vk_create_command_buffer(vk.command_pool, &vk.command_buffer);


    int width;
    int height;

    R_GetWinResolution(&width, &height);

    backEnd.viewParms.viewportWidth = width;
    backEnd.viewParms.viewportHeight = height;

    // Depth attachment image.
    vk_createDepthAttachment(width, height);

    vk_createFrameBuffers(width, height);

	// Pipeline layout.
	// You can use uniform values in shaders, which are globals similar to
    // dynamic state variables that can be changes at the drawing time to
    // alter the behavior of your shaders without having to recreate them.
    // They are commonly used to create texture samplers in the fragment 
    // shader. The uniform values need to be specified during pipeline
    // creation by creating a VkPipelineLayout object.
    
    vk_createPipelineLayout();

	//
	vk_createVertexBuffer();
    vk_createIndexBuffer();;
	//
	// Shader modules.
	//
	vk_loadShaderModules();

	//
	// Standard pipelines.
	//
    create_standard_pipelines();

    vk.isInitialized = VK_TRUE;
}


VkBool32 isVKinitialied(void)
{
    return vk.isInitialized;
}

// Shutdown vulkan subsystem by releasing resources acquired by Vk_Instance.
void vk_shutdown(void)
{
    ri.Printf( PRINT_DEVELOPER, "vk_shutdown()\n" );

    // After a fatal error part-way through vk_initialize() only some
    // of these objects exist and the device functions that destroy them
    // may not be loaded; the process is exiting, so release just the
    // device and instance (vk_clearProcAddress) then.
    if (vk.isInitialized)
    {
        vk_destroyDepthAttachment();

        vk_destroyFrameBuffers();

        vk_destroy_shading_data();

        vk_destroy_sync_primitives();

        vk_destroyShaderModules();

    //
        vk_destroyGlobalStagePipeline();
    //
        vk_destroy_commands();
    }

	vk_clearProcAddress();

    ri.Printf( PRINT_DEVELOPER, " clear vk struct: vk \n" );
	memset(&vk, 0, sizeof(vk));


    vk.isInitialized = VK_FALSE;
}

