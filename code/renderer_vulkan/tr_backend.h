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

#ifndef TR_BACKEND_H_
#define TR_BACKEND_H_

#include "tr_local.h"

typedef struct {
	int	c_surfaces;
    int c_shaders;
    int c_vertexes;
    int c_indexes;
    int c_totalIndexes;
	int	c_dlightVertexes;
	int	c_dlightIndexes;
	int	msec;			// total msec for backend run
} backEndCounters_t;


// all state modified by the back end is seperated
// from the front end state
typedef struct {
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	or;
	backEndCounters_t	pc;
	trRefEntity_t   entity2D;	// currentEntity will point at this when doing 2D rendering
	trRefEntity_t*  currentEntity;

    unsigned char	Color2D[4];
    qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	qboolean	isHyperspace;

} backEndState_t;

extern backEndState_t backEnd;

void R_ClearBackendState(void);
void R_PrintBackEnd_OR_f(void);

#endif
