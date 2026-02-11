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

#ifndef VK_PIPELINES_H_
#define VK_PIPELINES_H_

#include "tr_local.h"

void create_standard_pipelines(void);
void create_pipelines_for_each_stage(shaderStage_t *pStage, shader_t* pShader);
void vk_createPipelineLayout(void);

void vk_destroyShaderStagePipeline(void);
void vk_destroyGlobalStagePipeline(void);

void R_PipelineList_f(void);

struct GlobalPipelineManager {
	//
	// Standard pipelines.
	//
	VkPipeline skybox_pipeline;

	// dim 0: 0 - front side, 1 - back size
	// dim 1: 0 - normal view, 1 - mirror view
	VkPipeline shadow_volume_pipelines[2][2];
	VkPipeline shadow_finish_pipeline;

	// dim 0 is based on fogPass_t: 0 - corresponds to FP_EQUAL, 1 - corresponds to FP_LE.
	// dim 1 is directly a cullType_t enum value.
	// dim 2 is a polygon offset value (0 - off, 1 - on).
	VkPipeline fog_pipelines[2][3][2];

	// dim 0 is based on dlight additive flag: 0 - not additive, 1 - additive
	// dim 1 is directly a cullType_t enum value.
	// dim 2 is a polygon offset value (0 - off, 1 - on).
	VkPipeline dlight_pipelines[2][3][2];

	// debug visualization pipelines
	VkPipeline tris_debug_pipeline;
	VkPipeline tris_mirror_debug_pipeline;
	VkPipeline normals_debug_pipeline;
	VkPipeline surface_debug_pipeline_solid;
	VkPipeline surface_debug_pipeline_outline;
	VkPipeline images_debug_pipeline;
};

extern struct GlobalPipelineManager g_stdPipelines;


#endif
