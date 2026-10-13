#include "tr_local.h"
#include "vk_instance.h"
#include "vk_image.h"
#include "vk_cmd.h"
#include "vk_depth_attachment.h"


void vk_createDepthAttachment(int Width, int Height)
{
    // A depth attachment is based on an image, just like the color attachment
    // The difference is that the swap chain will not automatically create
    // depth image for us. We need only s single depth image, because only
    // one draw operation is running at once.
    ri.Printf(PRINT_ALL, " Create depth image: vk.depth_image, %d x %d. \n", Width, Height);
    {
        VkImageCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        desc.imageType = VK_IMAGE_TYPE_2D;
        desc.format = vk.fmt_DepthStencil;
        desc.extent.width = Width;
        desc.extent.height = Height;
        desc.extent.depth = 1;
        desc.mipLevels = 1;
        desc.arrayLayers = 1;
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.tiling = VK_IMAGE_TILING_OPTIMAL;
        desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        desc.queueFamilyIndexCount = 0;
        desc.pQueueFamilyIndices = NULL;
        desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VK_CHECK(qvkCreateImage(vk.device, &desc, NULL, &vk.depth_image));
    }

    ri.Printf(PRINT_ALL, " Allocate device local memory for depth image: vk.depth_image_memory. \n");
    {
        VkMemoryRequirements memory_requirements;
        qvkGetImageMemoryRequirements(vk.device, vk.depth_image, &memory_requirements);

        VkMemoryAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.allocationSize = memory_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type( memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        // = vk.idx_depthImgMem;
        VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &vk.depth_image_memory));
        VK_CHECK(qvkBindImageMemory(vk.device, vk.depth_image, vk.depth_image_memory, 0));
    }


    ri.Printf(PRINT_ALL, " Create image view for depth image: vk.depth_image_view. \n");
    {
        VkImageViewCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        desc.image = vk.depth_image;
        desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
        desc.format = vk.fmt_DepthStencil;
        desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        desc.subresourceRange.baseMipLevel = 0;
        desc.subresourceRange.levelCount = 1;
        desc.subresourceRange.baseArrayLayer = 0;
        desc.subresourceRange.layerCount = 1;
        VK_CHECK(qvkCreateImageView(vk.device, &desc, NULL, &vk.depth_image_view));
    }

    VkImageAspectFlags image_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;


    VkCommandBufferAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.commandPool = vk.command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer pCB;
    VK_CHECK(qvkAllocateCommandBuffers(vk.device, &alloc_info, &pCB));

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    VK_CHECK(qvkBeginCommandBuffer(pCB, &begin_info));

    record_image_layout_transition(pCB, vk.depth_image, 
            image_aspect_flags, 0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | 
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


    VK_CHECK(qvkEndCommandBuffer(pCB));

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &pCB;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    VK_CHECK(qvkQueueSubmit(vk.queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK(qvkQueueWaitIdle(vk.queue));
    qvkFreeCommandBuffers(vk.device, vk.command_pool, 1, &pCB);
}



void vk_destroyDepthAttachment(void)
{
    qvkDestroyImageView(vk.device, vk.depth_image_view, NULL);
	qvkDestroyImage(vk.device, vk.depth_image, NULL);
	qvkFreeMemory(vk.device, vk.depth_image_memory, NULL);
}
