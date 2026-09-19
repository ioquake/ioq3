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
