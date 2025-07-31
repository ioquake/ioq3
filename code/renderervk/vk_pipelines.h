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
