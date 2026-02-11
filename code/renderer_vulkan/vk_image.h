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

#ifndef VK_IMAGE_H_
#define VK_IMAGE_H_

#include "tr_image.h"

// work around, will be removed in the future

#ifndef GL_REPEAT
#define GL_REPEAT				0x2901
#endif

#ifndef GL_CLAMP
#define GL_CLAMP				0x2900
#endif

uint32_t find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags properties);


void vk_destroyImageRes(void);

image_t* R_FindImageFile(const char *name, VkBool32 mipmap,	VkBool32 allowPicmip, int glWrapClampMode);

image_t* R_CreateImage( const char *name, unsigned char* pic, uint32_t width, uint32_t height,
						VkBool32 mipmap, VkBool32 allowPicmip, int glWrapClampMode);


void R_LoadImage(const char *name, unsigned char **pic, uint32_t* width, uint32_t* height );
void R_LoadImage2(const char *name, unsigned char **pic, uint32_t* width, uint32_t* height );



void gpuMemUsageInfo_f(void);


#endif
