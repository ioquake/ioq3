#ifndef VK_INSTANCE_H_
#define VK_INSTANCE_H_


#include "VKimpl.h"
#include "ref_import.h" 


extern PFN_vkGetInstanceProcAddr						qvkGetInstanceProcAddr;

extern PFN_vkCreateInstance							    qvkCreateInstance;
extern PFN_vkEnumerateInstanceExtensionProperties		qvkEnumerateInstanceExtensionProperties;

extern PFN_vkCreateDevice								qvkCreateDevice;
extern PFN_vkDestroyInstance							qvkDestroyInstance;
extern PFN_vkEnumerateDeviceExtensionProperties	    	qvkEnumerateDeviceExtensionProperties;
extern PFN_vkEnumeratePhysicalDevices					qvkEnumeratePhysicalDevices;
extern PFN_vkGetDeviceProcAddr							qvkGetDeviceProcAddr;
extern PFN_vkGetPhysicalDeviceFeatures					qvkGetPhysicalDeviceFeatures;
extern PFN_vkGetPhysicalDeviceFormatProperties			qvkGetPhysicalDeviceFormatProperties;
extern PFN_vkGetPhysicalDeviceMemoryProperties			qvkGetPhysicalDeviceMemoryProperties;
extern PFN_vkGetPhysicalDeviceProperties				qvkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties	    qvkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkDestroySurfaceKHR							qvkDestroySurfaceKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		    qvkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	qvkGetPhysicalDeviceSurfacePresentModesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR		    qvkGetPhysicalDeviceSurfaceSupportKHR;

#ifndef NDEBUG
extern PFN_vkCreateDebugReportCallbackEXT				qvkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT				qvkDestroyDebugReportCallbackEXT;
#endif

extern PFN_vkAllocateCommandBuffers					    qvkAllocateCommandBuffers;
extern PFN_vkAllocateDescriptorSets					    qvkAllocateDescriptorSets;
extern PFN_vkAllocateMemory							    qvkAllocateMemory;
extern PFN_vkBeginCommandBuffer					    	qvkBeginCommandBuffer;
extern PFN_vkBindBufferMemory							qvkBindBufferMemory;
extern PFN_vkBindImageMemory							qvkBindImageMemory;
extern PFN_vkCmdBeginRenderPass					    	qvkCmdBeginRenderPass;
extern PFN_vkCmdBindDescriptorSets						qvkCmdBindDescriptorSets;
extern PFN_vkCmdBindIndexBuffer					    	qvkCmdBindIndexBuffer;
extern PFN_vkCmdBindPipeline							qvkCmdBindPipeline;
extern PFN_vkCmdBindVertexBuffers						qvkCmdBindVertexBuffers;
extern PFN_vkCmdBlitImage								qvkCmdBlitImage;
extern PFN_vkCmdClearAttachments						qvkCmdClearAttachments;
extern PFN_vkCmdCopyBufferToImage						qvkCmdCopyBufferToImage;
extern PFN_vkCmdCopyImage								qvkCmdCopyImage;
extern PFN_vkCmdCopyImageToBuffer                       qvkCmdCopyImageToBuffer;
extern PFN_vkCmdDraw									qvkCmdDraw;
extern PFN_vkCmdDrawIndexed						    	qvkCmdDrawIndexed;
extern PFN_vkCmdEndRenderPass							qvkCmdEndRenderPass;
extern PFN_vkCmdPipelineBarrier					    	qvkCmdPipelineBarrier;
extern PFN_vkCmdPushConstants							qvkCmdPushConstants;
extern PFN_vkCmdSetDepthBias							qvkCmdSetDepthBias;
extern PFN_vkCmdSetScissor								qvkCmdSetScissor;
extern PFN_vkCmdSetViewport						    	qvkCmdSetViewport;
extern PFN_vkCreateBuffer								qvkCreateBuffer;
extern PFN_vkCreateCommandPool							qvkCreateCommandPool;
extern PFN_vkCreateDescriptorPool						qvkCreateDescriptorPool;
extern PFN_vkCreateDescriptorSetLayout					qvkCreateDescriptorSetLayout;
extern PFN_vkCreateFence								qvkCreateFence;
extern PFN_vkCreateFramebuffer							qvkCreateFramebuffer;
extern PFN_vkCreateGraphicsPipelines					qvkCreateGraphicsPipelines;
extern PFN_vkCreateImage								qvkCreateImage;
extern PFN_vkCreateImageView							qvkCreateImageView;
extern PFN_vkCreatePipelineLayout						qvkCreatePipelineLayout;
extern PFN_vkCreateRenderPass							qvkCreateRenderPass;
extern PFN_vkCreateSampler								qvkCreateSampler;
extern PFN_vkCreateSemaphore							qvkCreateSemaphore;
extern PFN_vkCreateShaderModule				    		qvkCreateShaderModule;
extern PFN_vkDestroyBuffer								qvkDestroyBuffer;
extern PFN_vkDestroyCommandPool					    	qvkDestroyCommandPool;
extern PFN_vkDestroyDescriptorPool						qvkDestroyDescriptorPool;
extern PFN_vkDestroyDescriptorSetLayout			    	qvkDestroyDescriptorSetLayout;
extern PFN_vkDestroyDevice								qvkDestroyDevice;
extern PFN_vkDestroyFence								qvkDestroyFence;
extern PFN_vkDestroyFramebuffer					    	qvkDestroyFramebuffer;
extern PFN_vkDestroyImage								qvkDestroyImage;
extern PFN_vkDestroyImageView							qvkDestroyImageView;
extern PFN_vkDestroyPipeline							qvkDestroyPipeline;
extern PFN_vkDestroyPipelineLayout						qvkDestroyPipelineLayout;
extern PFN_vkDestroyRenderPass							qvkDestroyRenderPass;
extern PFN_vkDestroySampler						    	qvkDestroySampler;
extern PFN_vkDestroySemaphore							qvkDestroySemaphore;
extern PFN_vkDestroyShaderModule						qvkDestroyShaderModule;
extern PFN_vkDeviceWaitIdle						    	qvkDeviceWaitIdle;
extern PFN_vkEndCommandBuffer							qvkEndCommandBuffer;
extern PFN_vkFreeCommandBuffers					    	qvkFreeCommandBuffers;
extern PFN_vkFreeDescriptorSets					    	qvkFreeDescriptorSets;
extern PFN_vkFreeMemory							    	qvkFreeMemory;
extern PFN_vkGetBufferMemoryRequirements				qvkGetBufferMemoryRequirements;
extern PFN_vkGetDeviceQueue						    	qvkGetDeviceQueue;
extern PFN_vkGetImageMemoryRequirements			    	qvkGetImageMemoryRequirements;
extern PFN_vkGetImageSubresourceLayout					qvkGetImageSubresourceLayout;
extern PFN_vkMapMemory									qvkMapMemory;
extern PFN_vkUnmapMemory                                qvkUnmapMemory;
extern PFN_vkQueueSubmit								qvkQueueSubmit;
extern PFN_vkQueueWaitIdle								qvkQueueWaitIdle;
extern PFN_vkResetDescriptorPool						qvkResetDescriptorPool;
extern PFN_vkResetFences								qvkResetFences;
extern PFN_vkUpdateDescriptorSets						qvkUpdateDescriptorSets;
extern PFN_vkWaitForFences								qvkWaitForFences;
extern PFN_vkAcquireNextImageKHR						qvkAcquireNextImageKHR;
extern PFN_vkCreateSwapchainKHR				    		qvkCreateSwapchainKHR;
extern PFN_vkDestroySwapchainKHR						qvkDestroySwapchainKHR;
extern PFN_vkGetSwapchainImagesKHR						qvkGetSwapchainImagesKHR;
extern PFN_vkQueuePresentKHR							qvkQueuePresentKHR;



// Initializes VK_Instance structure.
void vk_getProcAddress(void);
void vk_clearProcAddress(void);

const char * cvtResToStr(VkResult result);

#ifndef NDEDBG
#define VK_CHECK(function_call) { \
	VkResult result = function_call; \
	if (result != VK_SUCCESS) \
		ri.Printf(PRINT_ALL, \
        "Vulkan: error %s returned by %s \n", cvtResToStr(result), #function_call); \
}
#else
#define VK_CHECK(function_call)	\
	function_call;
#endif


#define MAX_SWAPCHAIN_IMAGES    8

// Vk_Instance contains engine-specific vulkan resources that persist entire renderer lifetime.
// This structure is initialized/deinitialized by vk_initialize/vk_shutdown functions correspondingly.
struct Vk_Instance {
	VkInstance instance ;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceFeatures features;

    // Native platform surface or window objects are abstracted by surface objects,
    // which are represented by VkSurfaceKHR handles. The VK_KHR_surface extension
    // declares the VkSurfaceKHR object, and provides a function for destroying
    // VkSurfaceKHR objects. Separate platform-specific extensions each provide a
    // function for creating a VkSurfaceKHR object for the respective platform.
	VkSurfaceKHR surface;
	VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_caps;

// Depth/stencil formats are considered opaque and need not be stored
// in the exact number of bits pertexel or component ordering indicated
// by the format enum. However, implementations must not substitute a
// different depth or stencil precision than that described in the
// format (e.g. D16 must not be implemented as D24 or D32).

// The features for the set of formats (VkFormat) supported by the
// implementation are queried individually using the 
// vkGetPhysicalDeviceFormatProperties command.
// To determine the set of valid usage bits for a given format, 
// call vkGetPhysicalDeviceFormatProperties.

// depth and stencil aspects of a given image subresource must always be in the same layout.

    VkFormat fmt_DepthStencil;
	VkPhysicalDeviceMemoryProperties devMemProperties;
	
    uint32_t queue_family_index;
	VkDevice device;
	VkQueue queue;

	VkSwapchainKHR swapchain;
	uint32_t swapchain_image_count ;
	VkImage swapchain_images_array[MAX_SWAPCHAIN_IMAGES];
	VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
	uint32_t idx_swapchain_image;


	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	VkRenderPass render_pass;
	VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGES];

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout set_layout;

    // Pipeline layout: the uniform and push values referenced by 
    // the shader that can be updated at draw time
	VkPipelineLayout pipeline_layout;

    VkBool32 isBlitSupported;

    VkBool32 isInitialized;

#ifndef NDEBUG
    VkDebugReportCallbackEXT h_debugCB;
#endif
};



extern struct Vk_Instance vk;



#endif
