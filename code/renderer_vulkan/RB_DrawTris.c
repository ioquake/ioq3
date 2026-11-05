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

#include "RB_DrawTris.h"
#include "tr_globals.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "tr_backend.h"
/*
================
Draws triangle outlines for debugging
================
*/
void RB_DrawTris (shaderCommands_t * pInput)
{
	if (vk.features.fillModeNonSolid == VK_FALSE) {
		static qboolean printed = qfalse;
		if (!printed) {
			ri.Printf(PRINT_WARNING, "RB_ShowTris: fillModeNonSolid not supported.\n");
			printed = qtrue;
		}
		return;
	}


	updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

	// VULKAN

    memset(pInput->svars.colors, 255, pInput->numVertexes * 4 );
    VkPipeline pipeline = backEnd.viewParms.isMirror ? g_stdPipelines.tris_mirror_debug_pipeline : g_stdPipelines.tris_debug_pipeline;
    vk_shade_geometry(pipeline, VK_FALSE, DEPTH_RANGE_ZERO, VK_TRUE);
}
