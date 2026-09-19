#include "VKimpl.h"
#include "vk_image_sampler.h"
#include "vk_instance.h"
#include "ref_import.h"

typedef struct {
	char *name;
	int	minimize, maximize;
} textureMode_t;

struct Vk_Sampler_Def
{
	VkBool32 repeat_texture; // clamp/repeat texture addressing mode
	int gl_mag_filter; // GL_XXX mag filter
	int gl_min_filter; // GL_XXX min filter
};

#ifndef GL_NEAREST
#define GL_NEAREST				0x2600
#endif

#ifndef GL_LINEAR
#define GL_LINEAR				0x2601
#endif

#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST		0x2700
#endif

#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR		0x2702
#endif

#ifndef GL_LINEAR_MIPMAP_NEAREST
#define GL_LINEAR_MIPMAP_NEAREST		0x2701
#endif

#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR			0x2703
#endif

const static textureMode_t texModes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};


/*
  The maximum number of sampler objects which can be simultaneously created on a device
  is implementation-dependent and specified by the maxSamplerAllocationCount member of
  the VkPhysicalDeviceLimits structure. If maxSamplerAllocationCount is exceeded, 
  vkCreateSampler will return VK_ERROR_TOO_MANY_OBJECTS.
*/


#define MAX_VK_SAMPLERS     32
static struct Vk_Sampler_Def s_SamplerDefs[MAX_VK_SAMPLERS] = {0};

static uint32_t s_NumSamplers = 0;
static VkSampler s_ImgSamplers[MAX_VK_SAMPLERS] = {0};



void vk_free_sampler(void)
{
    uint32_t i = 0;
    for (i = 0; i < s_NumSamplers; i++)
    {
        if(s_ImgSamplers[i] != VK_NULL_HANDLE)
        {
		    qvkDestroySampler(vk.device, s_ImgSamplers[i], NULL);
            s_ImgSamplers[i] = VK_NULL_HANDLE;
        }

        memset(&s_SamplerDefs[i], 0, sizeof(struct Vk_Sampler_Def));
    }

    s_NumSamplers = 0;
}


void vk_set_sampler(int m)
{
	if ( m >= 6 ) {
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

    ri.Cvar_Set( "r_textureMode", texModes[m].name);
}


VkSampler vk_find_sampler(VkBool32 mipmap, VkBool32 repeat_texture)
{
	struct Vk_Sampler_Def sampler_def;
    memset(&sampler_def, 0, sizeof(sampler_def));

	sampler_def.repeat_texture = repeat_texture;
	if (mipmap) {
		sampler_def.gl_mag_filter = GL_LINEAR;
		sampler_def.gl_min_filter = GL_LINEAR_MIPMAP_NEAREST;
	} else {
		sampler_def.gl_mag_filter = GL_LINEAR;
		sampler_def.gl_min_filter = GL_LINEAR;
	}

	// Look for sampler among existing samplers.
	uint32_t i;
    for (i = 0; i < s_NumSamplers; i++)
    {
		if (( s_SamplerDefs[i].repeat_texture == sampler_def.repeat_texture) &&
			( s_SamplerDefs[i].gl_mag_filter == sampler_def.gl_mag_filter) && 
			( s_SamplerDefs[i].gl_min_filter == sampler_def.gl_min_filter) )
		{
			return s_ImgSamplers[i];
		}
	}

	s_SamplerDefs[s_NumSamplers] = sampler_def;
	// Create new sampler.
	if (s_NumSamplers >= MAX_VK_SAMPLERS)
    {
		ri.Error(ERR_DROP, "vk_find_sampler: MAX_VK_SAMPLERS hit\n");
	}

	VkSamplerAddressMode address_mode = repeat_texture ?
        VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	VkFilter mag_filter;
	if (sampler_def.gl_mag_filter == GL_NEAREST)
    {
		mag_filter = VK_FILTER_NEAREST;
	}
    else if(sampler_def.gl_mag_filter == GL_LINEAR)
    {
		mag_filter = VK_FILTER_LINEAR;
	}
    else
    {
		ri.Error(ERR_FATAL, "vk_find_sampler: invalid gl_mag_filter");
	}

	VkFilter min_filter;
	VkSamplerMipmapMode mipmap_mode;
	qboolean max_lod_0_25 = qfalse; // used to emulate OpenGL's GL_LINEAR/GL_NEAREST minification filter
	if (sampler_def.gl_min_filter == GL_NEAREST) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		max_lod_0_25 = qtrue;
	}
    else if (sampler_def.gl_min_filter == GL_LINEAR)
    {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		max_lod_0_25 = qtrue;
	}
    else if (sampler_def.gl_min_filter == GL_NEAREST_MIPMAP_NEAREST)
    {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
    else if (sampler_def.gl_min_filter == GL_LINEAR_MIPMAP_NEAREST)
    {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
    else if (sampler_def.gl_min_filter == GL_NEAREST_MIPMAP_LINEAR)
    {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
    else if (sampler_def.gl_min_filter == GL_LINEAR_MIPMAP_LINEAR)
    {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
    else {
		ri.Error(ERR_FATAL, "vk_find_sampler: invalid gl_min_filter");
	}

	VkSamplerCreateInfo desc;
	desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.magFilter = mag_filter;
	desc.minFilter = min_filter;
	desc.mipmapMode = mipmap_mode;
	desc.addressModeU = address_mode;
	desc.addressModeV = address_mode;
	desc.addressModeW = address_mode;
	desc.mipLodBias = 0.0f;
	desc.anisotropyEnable = VK_FALSE;
	desc.maxAnisotropy = 1;
	desc.compareEnable = VK_FALSE;
	desc.compareOp = VK_COMPARE_OP_ALWAYS;
	desc.minLod = 0.0f;
	desc.maxLod = max_lod_0_25 ? 0.25f : 12.0f;
	desc.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	desc.unnormalizedCoordinates = VK_FALSE;

	VkSampler sampler;
	VK_CHECK(qvkCreateSampler(vk.device, &desc, NULL, &sampler));


	s_ImgSamplers[s_NumSamplers++] = sampler;
	if (s_NumSamplers >= MAX_VK_SAMPLERS)
    {
		ri.Error(ERR_DROP, "MAX_VK_SAMPLERS hit\n");
	}
	return sampler;
}

