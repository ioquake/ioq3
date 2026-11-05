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

#include "tr_backend.h"
#include "vk_shade_geometry.h"
#include "tr_globals.h"
#include "vk_pipelines.h"

/*
================
Draws vertex normals for debugging
================
*/
void RB_DrawNormals (shaderCommands_t* pTess, int numVertexes )
{
	// VULKAN
    // int numVertexes = tess.numVertexes;
    vec4_t xyz[SHADER_MAX_VERTEXES];
    memcpy(xyz, pTess->xyz, numVertexes * sizeof(vec4_t));
   
    memset(pTess->svars.colors, tr.identityLightByte, SHADER_MAX_VERTEXES * sizeof(color4ub_t));

    int i = 0;
    while (i < numVertexes)
    {
        int count = numVertexes - i;
        if (count >= SHADER_MAX_VERTEXES/2 - 1)
            count = SHADER_MAX_VERTEXES/2 - 1;

        int k;
        for (k = 0; k < count; k++)
        {
            VectorCopy(xyz[i + k], pTess->xyz[2*k]);
            VectorMA(xyz[i + k], 2, pTess->normal[i + k], pTess->xyz[2*k + 1]);
        }
        pTess->numVertexes = 2 * count;
        pTess->numIndexes = 0;

        vk_UploadXYZI(pTess->xyz, pTess->numVertexes, NULL, 0);
        
        updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, getptr_modelview_matrix());
        vk_shade_geometry(g_stdPipelines.normals_debug_pipeline, VK_FALSE, DEPTH_RANGE_ZERO, VK_FALSE);

        i += count;
    }
}
