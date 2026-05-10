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
// PER-OBJECT SHADOW MAPS (PSHADOW)
//=============================================================================

#define MAX_DRAWN_PSHADOWS  4
#define MAX_CALC_PSHADOWS   32
#define PSHADOW_MAP_SIZE    512

typedef struct {
	float      lightRadius;      // World-space extent of shadow projection
	float      viewRadius;       // Bounding sphere radius of all caster entities
	vec3_t     lightOrigin;      // World-space light position
	vec3_t     viewOrigin;       // World-space centre of caster bounding sphere
	vec3_t     lightViewAxis[3]; // [0]=forward(-lightDir), [1]=right, [2]=up
	cplane_t   cullPlane;        // Cull receivers facing away from the light
	int        numEntities;      // Number of caster entities (max 8)
	int        entityNums[8];    // Indices into drawPackets_ (set during computePshadows)
	vec3_t     entityOrigins[8]; // Entity world-space origins
	float      entityRadiuses[8];// Entity bounding radii
	float      sort;             // Sort key: smaller = higher priority
} pshadow_t;

#ifdef __cplusplus
#include <cmath>
static inline bool SpheresIntersect(const float* a, float ra, const float* b, float rb) {
	float dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
	float r  = ra + rb;
	return (dx*dx + dy*dy + dz*dz) <= (r*r);
}
static inline void BoundingSphereOfSpheres(const float* a, float ra, const float* b, float rb,
                                            float* center, float* radius) {
	float dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
	float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
	if (dist < 1e-4f) {
		center[0] = a[0]; center[1] = a[1]; center[2] = a[2];
		*radius = (ra > rb) ? ra : rb;
	} else {
		float t = (dist + rb - ra) / (2.0f * dist);
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		center[0] = a[0] + t*dx;
		center[1] = a[1] + t*dy;
		center[2] = a[2] + t*dz;
		*radius = (dist + ra + rb) * 0.5f;
	}
}
#endif

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

// Set identity light value based on overbright bits (call after cvar init)
// identityLight = 1.0 / (1 << overbrightBits) for overbright rendering
void R_SetIdentityLight(int overbrightBits);

// Get current identity light value
float R_GetIdentityLight(void);

// Get current sun direction (world-space unit vector, used by post-processing)
void R_GetSunDirection(vec3_t out);

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

// -------------------------------------------------------------------------
// Cubemap probe system (mirrors GL2 tr_light.c / tr_bsp.c)
// -------------------------------------------------------------------------

// Clear any loaded probe origins (called on world load/unload).
void R_InitCubemapProbes(void);
void R_ShutdownCubemapProbes(void);

// Parse the BSP entity string and collect probe origins from
// 'misc_cubemap' entities (falls back to 'info_player_deathmatch').
// Returns the number of probes found.
int  R_LoadCubemapProbeOrigins(const char* entitiesData, int entitiesLen);

// Return the 1-based index of the nearest probe to 'point', or 0 if none.
// Matches GL2's R_CubemapForPoint return convention.
int  R_CubemapForPoint(const vec3_t point);

// Accessors for probe data (used by the Metal backend to build GPU textures).
int  R_GetNumCubemapProbes(void);
void R_GetCubemapProbeOrigin(int idx, vec3_t out);

#ifdef __cplusplus
// Forward declarations for C++ brush model dlight support
struct MetalBrushModel;
struct MetalSceneLight;

/*
 * R_DlightBmodel - port of renderergl2/tr_light.c:R_DlightBmodel
 *
 * Transforms each scene light into the brush model's local coordinate space
 * and tests it against the model's AABB.  Returns a bitmask (bit i set means
 * light i overlaps the model and should contribute a dlight pass).
 */
uint32_t R_DlightBmodel(const MetalBrushModel& bmodel, const refEntity_t& ent,
                         const MetalSceneLight* lights, int numLights);
#endif

#endif // TR_LOCAL_H
