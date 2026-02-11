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

#ifndef TR_IMAGE_H_
#define TR_IMAGE_H_

#include "VKimpl.h"


typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
	uint32_t	width, height;				// source image
	uint32_t	uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE

    uint32_t    index;

	VkImage handle;
    // To use any VkImage, including those in the swap chain, int the render pipeline
    // we have to create a VkImageView object. An image view is quite literally a
    // view into image. It describe how to access the image and witch part of the
    // image to access, if it should be treated as a 2D texture depth texture without
    // any mipmapping levels.
    
    VkImageView view;

    // Descriptor set that contains single descriptor used to access the given image.
	// It is updated only once during image initialization.
	VkDescriptorSet descriptor_set;

    int			wrapClampMode;		// GL_CLAMP or GL_REPEAT, for vulkan
    VkBool32    mipmap;             // for vulkan
    uint32_t    mipLevels;			// gl texture binding
    VkBool32    allowPicmip;        // for vulkan
    VkBool32    isLightmap;

	struct image_s*	next;
} image_t;


#endif
