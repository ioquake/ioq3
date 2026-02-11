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

#ifndef GL_CONFIGURE_H_
#define GL_CONFIGURE_H_


void R_SetWinMode(int mode, unsigned int w, unsigned int h, unsigned int hz);
void R_glConfigInit(void);
void R_glConfigClear(void);
void R_GetWinResolution(int* w, int* h);
void R_GetWinResolutionF(float* w, float* h);

void R_GetGlConfig(glconfig_t * const pOut);
void R_DisplayResolutionList_f(void);
void R_InitDisplayResolution( void );

void vulkanInfo_f( void );
#endif
