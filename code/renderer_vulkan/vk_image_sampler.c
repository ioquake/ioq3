#include "VKimpl.h"
#include "vk_image_sampler.h"
#include "vk_instance.h"
#include "ref_import.h"


//static cvar_t* r_textureMode;

/*
    VkSampler objects represent the state of an image sampler 
    which is used by the implementation to read image data and
    apply filtering and other transformations for the shader.
*/


/*
  The maximum number of sampler objects which can be simultaneously
  created on a device is implementation-dependent and specified by the 
  maxSamplerAllocationCount member of the VkPhysicalDeviceLimits structure.
  If maxSamplerAllocationCount is exceeded, vkCreateSampler will return
  VK_ERROR_TOO_MANY_OBJECTS.

  4000
  
  Since VkSampler is a non-dispatchable handle type, implementations may 
  return the same handle for sampler state vectors that are identical. 
  In such cases, all such objects would only count once against the
  maxSamplerAllocationCount limit.
*/


#define MAX_VK_SAMPLERS     8

struct Vk_Sampler_Def
{
	VkBool32 repeat_texture; // clamp/repeat texture addressing mode
    VkBool32 mipmap;
    
    VkSampler ImgSampler;
};

static struct Vk_Sampler_Def s_SamplerDefs[MAX_VK_SAMPLERS] = {0};
static int s_NumSamplers = 0;



void vk_free_sampler(void)
{
    int i = 0;
    for (i = 0; i < s_NumSamplers; i++)
    {
        if(s_SamplerDefs[i].ImgSampler != VK_NULL_HANDLE)
        {
		    qvkDestroySampler(vk.device, s_SamplerDefs[i].ImgSampler, NULL);
        }

        memset(&s_SamplerDefs[i], 0, sizeof(struct Vk_Sampler_Def));
    }

    s_NumSamplers = 0;
}



VkSampler vk_find_sampler( VkBool32 isMipmap, VkBool32 isRepeatTexture )
{

	// Look for sampler among existing samplers.
	int i;
    for (i = 0; i < s_NumSamplers; i++)
    {
		if( ( s_SamplerDefs[i].repeat_texture == isRepeatTexture ) && 
            ( s_SamplerDefs[i].mipmap == isMipmap ) )
        {
			return s_SamplerDefs[i].ImgSampler;
		}
	}


    // Create it, if not exist!
    // VK_SAMPLER_ADDRESS_MODE_REPEAT specifies that the repeat wrap mode will be used.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE specifies that the clamp to edge wrap mode will be used.
    // specifying the behavior of sampling with coordinates outside 
    // the range [0,1] for the respective u, v, or w coordinate as defined
    // in the Wrapping Operation section
	VkSamplerAddressMode address_mode = isRepeatTexture ?
        VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    // magFilter is a VkFilter value specifying 
    // the magnification filter to apply to lookups.

	VkFilter mag_filter = VK_FILTER_LINEAR;
//  VkFilter mag_filter = VK_FILTER_NEAREST;

    // minFilter is a VkFilter value specifying
    // the minification filter to apply to lookups.
    VkFilter min_filter = VK_FILTER_LINEAR;
//  VkFilter min_filter = VK_FILTER_NEAREST;

    
    //used to emulate OpenGL's GL_LINEAR/GL_NEAREST minification filter    
    VkBool32 max_lod_0_25 = 0;

    // mipmapMode is a VkSamplerMipmapMode value specifying
    // the mipmap filter to apply to lookups.
    VkSamplerMipmapMode mipmap_mode;
    if (isMipmap)
    {
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        max_lod_0_25 = 0;
    }
    else
    {
        mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        max_lod_0_25 = 1;
    }
    //VK_SAMPLER_MIPMAP_MODE_LINEAR;


	VkSamplerCreateInfo desc;
	desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.magFilter = mag_filter;
	desc.minFilter = min_filter;

    // mipmapMode is a VkSamplerMipmapMode value specifying the mipmap filter
    // to apply to lookups
	desc.mipmapMode = mipmap_mode;

	desc.addressModeU = address_mode;
	desc.addressModeV = address_mode;
	desc.addressModeW = address_mode;

    // mipLodBias is the bias to be added to mipmap LOD calculation
    // and bias provided by image sampling functions in SPIR-V, 
    // as described in the Level-of-Detail Operation section.
	desc.mipLodBias = 0.0f;

    // anisotropyEnable is VK_TRUE to enable anisotropic filtering, 
    // or VK_FALSE otherwise.
	desc.anisotropyEnable = VK_TRUE;

    // maxAnisotropy is the anisotropy value clamp used by the sampler
    // when anisotropyEnable is VK_TRUE. If anisotropyEnable is VK_FALSE, 
    // maxAnisotropy is ignored.
	desc.maxAnisotropy = 16;

    // compareEnable is VK_TRUE to enable comparison against a reference value
    // during lookups, or VK_FALSE otherwise. 
    // compareOp is a VkCompareOp value specifying the comparison function 
    // to apply to fetched data before filtering as described in the
    // Depth Compare Operation section.
	desc.compareEnable = VK_FALSE;
	desc.compareOp = VK_COMPARE_OP_ALWAYS;

    // minLod and maxLod are the values used to clamp the computed LOD value,
    // as described in the Level-of-Detail Operation section.
	desc.minLod = 0.0f;
	desc.maxLod = max_lod_0_25 ? 0.25f : 12.0f;

    // borderColor is a VkBorderColor value specifying 
    // the predefined border color to use.
	desc.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	
    // unnormalizedCoordinates controls whether to use unnormalized or normalized
    // texel coordinates to address texels of the image.
    //
    // When set to VK_TRUE, the range of the image coordinates used 
    // to lookup the texel is in the range of zero to the image dimensions
    // for x, y and z.
    // 
    // When set to VK_FALSE the range of image coordinates is zero to one. 
    desc.unnormalizedCoordinates = VK_FALSE;


    // To create a sampler object
    VkSampler sampler;
	VK_CHECK(qvkCreateSampler(vk.device, &desc, NULL, &sampler));

	s_SamplerDefs[s_NumSamplers].repeat_texture = isRepeatTexture;
    s_SamplerDefs[s_NumSamplers].mipmap = isMipmap;
    s_SamplerDefs[s_NumSamplers].ImgSampler = sampler;
	s_NumSamplers++;
	if (s_NumSamplers >= MAX_VK_SAMPLERS)
    {
		ri.Error(ERR_DROP, "MAX_VK_SAMPLERS hit\n");
	}

	return sampler;
}
