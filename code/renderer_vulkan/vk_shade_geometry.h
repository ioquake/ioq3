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

#ifndef VK_SHADE_GEOMETRY
#define VK_SHADE_GEOMETRY

#include "vk_instance.h"

enum Vk_Depth_Range {
	DEPTH_RANGE_NORMAL, // [0..1]
	DEPTH_RANGE_ZERO, // [0..0]
	DEPTH_RANGE_ONE, // [1..1]
	DEPTH_RANGE_WEAPON // [0..0.3]
};


const float * getptr_modelview_matrix(void);

void set_modelview_matrix(const float mv[16]);

void vk_shade_geometry(VkPipeline pipeline, VkBool32 multitexture, enum Vk_Depth_Range depth_range, VkBool32 indexed);
void vk_UploadXYZI(float (*pXYZ)[4], uint32_t nVertex, uint32_t* pIdx, uint32_t nIndex);

void updateMVP(VkBool32 isPortal, VkBool32 is2D, const float mvMat4x4[16]);
void vk_resetGeometryBuffer(void);

void vk_createVertexBuffer(void);
void vk_createIndexBuffer(void);

VkBuffer vk_getIndexBuffer(void);
void vk_destroy_shading_data(void);

void updateCurDescriptor( VkDescriptorSet curDesSet, uint32_t tmu);

VkRect2D get_scissor_rect(void);


void vk_clearColorAttachments(const float* color);
void vk_clearDepthStencilAttachments(void);

#endif
