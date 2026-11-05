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

#include "ref_import.h"
#include "tr_backend.h"
#include "R_PrintMat.h"
#include "glConfig.h"
backEndState_t backEnd;


void R_ClearBackendState(void)
{
    ri.Printf(PRINT_DEVELOPER, " backend state cleared. \n");
	// clear all our internal state
	memset( &backEnd, 0, sizeof( backEnd ) );

    int width, height;
    R_GetWinResolution(&width, &height);
    backEnd.viewParms.viewportWidth = width;
    backEnd.viewParms.viewportHeight = height;
}


void R_PrintBackEnd_OR_f(void)
{
    // in world coordinates
    printMat1x3f("backEnd.or.origin", backEnd.or.origin);
    // orientation in world
    printMat3x3f("backEnd.or.axis", backEnd.or.axis);
    // viewParms->or.origin in local coordinates
    printMat1x3f("backEnd.or.viewOrigin", backEnd.or.viewOrigin);
    printMat4x4f("backEnd.or.modelMatrix", backEnd.or.modelMatrix);	
}
