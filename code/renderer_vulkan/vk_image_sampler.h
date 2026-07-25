#ifndef IMAGE_SAMPLER_H_
#define IMAGE_SAMPLER_H_

void vk_free_sampler(void);
VkSampler vk_find_sampler( VkBool32 isMipmap, VkBool32 isRepeatTexture );

//void vk_set_sampler(int m);

#endif
