#ifndef VK_CMD_H_
#define VK_CMD_H_


void vk_create_command_pool(VkCommandPool* pPool);
void vk_create_command_buffer(VkCommandPool pool, VkCommandBuffer* pBuf);
void vk_destroy_commands(void);

void record_image_layout_transition(VkCommandBuffer command_buffer, VkImage image, VkImageAspectFlags image_aspect_flags,
	VkAccessFlags src_access_flags, VkImageLayout old_layout, VkAccessFlags dst_access_flags, VkImageLayout new_layout);


#endif
