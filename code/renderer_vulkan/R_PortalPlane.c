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

#include "R_PortalPlane.h"

static struct rplane_s g_portalPlane;		// clip anything behind this if mirroring

inline static float DotProduct( const float v1[3], const float v2[3] )
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}


void R_SetupPortalPlane(const float axis[3][3], const float origin[3])
{
	// VectorSubtract( vec3_origin, pCamera->axis[0], g_portalPlane.normal );
    // g_portalPlane.dist = DotProduct( pCamera->origin, g_portalPlane.normal );

    g_portalPlane.normal[0] = - axis[0][0];
    g_portalPlane.normal[1] = - axis[0][1];
    g_portalPlane.normal[2] = - axis[0][2];
    g_portalPlane.dist = - origin[0] * axis[0][0]
                         - origin[1] * axis[0][1]
                         - origin[2] * axis[0][2];
}


void R_TransformPlane(const float R[3][3], const float T[3], struct rplane_s* pDstPlane)
{
    pDstPlane->normal[0] = DotProduct (R[0], g_portalPlane.normal);
    pDstPlane->normal[1] = DotProduct (R[1], g_portalPlane.normal);
    pDstPlane->normal[2] = DotProduct (R[2], g_portalPlane.normal);
    pDstPlane->dist = DotProduct (T, g_portalPlane.normal) - g_portalPlane.dist;
}
