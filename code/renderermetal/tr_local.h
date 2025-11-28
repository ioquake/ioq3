/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

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
// tr_local.h - Metal renderer internal definitions

#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"
#include "../renderercommon/tr_types.h"

#ifdef __cplusplus
}
#endif

// Function table size for sine/cosine lookups
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)

//=============================================================================
// DYNAMIC LIGHTS
//=============================================================================

// Dynamic light structure - matches renderergl2/tr_local.h:80-87
typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost when the lightmap is dark
} dlight_t;

//=============================================================================
// ENTITIES
//=============================================================================

// Internal entity structure with lighting data
// Matches renderergl2/tr_local.h:92-105
typedef struct {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	qboolean	mirrored;		// mirrored matrix, needs reversed culling
	vec3_t		lightDir;		// normalized direction towards light, in world space
	vec3_t		modelLightDir;	// normalized direction towards light, in model space
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;// 32 bit rgba packed
	vec3_t		directedLight;	// color normalized to 0-255
} trRefEntity_t;

//=============================================================================
// RENDERER IMPORTS
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

extern	refimport_t		ri;

// Console variables
extern cvar_t	*r_ambientScale;
extern cvar_t	*r_directedScale;
extern cvar_t	*r_debugLight;
extern cvar_t	*r_dlightMode;

#ifdef __cplusplus
}
#endif

//=============================================================================
// LIGHTING FUNCTIONS
//=============================================================================

// Initialize/shutdown lighting system
void R_InitLightingSystem(void);
void R_ShutdownLightingSystem(void);

// Load light grid data from BSP
void R_LoadLightGrid(const byte* gridData, int gridDataSize,
                     const uint16_t* grid16Data, int grid16DataSize,
                     const vec3_t gridOrigin, const vec3_t gridSize,
                     const int gridBounds[3]);

// Setup entity lighting from light grid + dynamic lights
void R_SetupEntityLighting(const refdef_t* refdef, trRefEntity_t* ent,
                           const dlight_t* dlights, int numDlights);

// Sample light grid at a specific point
int R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);

#endif // TR_LOCAL_H
