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

#ifndef TR_CVAR_H_
#define TR_CVAR_H_

#include "../qcommon/q_shared.h"


extern cvar_t	*r_railWidth;
extern cvar_t	*r_railCoreWidth;
extern cvar_t	*r_railSegmentLength;

extern cvar_t	*r_verbose;				// used for verbose debug spew

extern cvar_t	*r_znear;				// near Z clip plane


extern cvar_t	*r_depthbits;			// number of desired depth bits




extern cvar_t	*r_inGameVideo;			// controls whether in game video should be draw
extern cvar_t	*r_dynamiclight;		// dynamic lights enabled/disabled

extern cvar_t	*r_norefresh;			// bypasses the ref rendering
extern cvar_t	*r_drawentities;		// disable/enable entity rendering
extern cvar_t	*r_drawworld;			// disable/enable world rendering
extern cvar_t	*r_speeds;				// various levels of information display

extern cvar_t	*r_novis;				// disable/enable usage of PVS
extern cvar_t	*r_nocull;
extern cvar_t	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern cvar_t	*r_nocurves;
extern cvar_t	*r_showcluster;

extern cvar_t	*r_mode;				// video mode
extern cvar_t	*r_fullscreen;
extern cvar_t	*r_gamma;


extern cvar_t	*r_singleShader;				// make most world faces use default shader
extern cvar_t	*r_colorMipLevels;				// development aid to see texture mip usage
extern cvar_t	*r_picmip;						// controls picmip values
extern cvar_t	*r_offsetFactor;
extern cvar_t	*r_offsetUnits;

extern cvar_t	*r_fullbright;					// avoid lightmap pass
extern cvar_t	*r_lightmap;					// render lightmaps only
extern cvar_t	*r_vertexLight;					// vertex lighting mode for better performance
extern cvar_t	*r_uiFullScreen;				// ui is running fullscreen

extern cvar_t	*r_showtris;					// enables wireframe rendering of the world
extern cvar_t	*r_showsky;						// forces sky in front of all surfaces
extern cvar_t	*r_shownormals;					// draws wireframe normals
extern cvar_t	*r_clear;						// force screen clear every frame

extern cvar_t	*r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection

extern cvar_t	*r_intensity;

extern cvar_t	*r_lockpvs;
extern cvar_t	*r_noportals;
extern cvar_t	*r_portalOnly;

extern cvar_t	*r_subdivisions;
extern cvar_t	*r_lodCurveError;

//extern	cvar_t	*r_overBrightBits;
extern	cvar_t	*r_mapOverBrightBits;

extern	cvar_t	*r_debugSurface;
extern	cvar_t	*r_simpleMipMaps;

extern	cvar_t	*r_showImages;
extern	cvar_t	*r_debugSort;

extern	cvar_t	*r_printShaders;
extern	cvar_t	*r_saveFontData;


extern cvar_t	*r_maxpolys;
extern cvar_t	*r_maxpolyverts;


extern	cvar_t	*r_ambientScale;
extern	cvar_t	*r_directedScale;
extern	cvar_t	*r_debugLight;

extern cvar_t* r_allowResize; // make window resizable
extern cvar_t* r_mode;
extern cvar_t* r_fullscreen;
extern cvar_t* r_displayRefresh;
extern cvar_t* r_vkValidation;

void R_Register( void );



#endif
