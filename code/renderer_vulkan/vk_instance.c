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

#include <string.h>
#include <stdlib.h>

#include "VKimpl.h"
#include "vk_instance.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "vk_image.h"
#include "vk_instance.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "vk_frame.h"
#include "vk_shaders.h"
#include "vk_depth_attachment.h"


struct Vk_Instance vk;

//
// Vulkan API functions used by the renderer.
//
PFN_vkGetInstanceProcAddr						qvkGetInstanceProcAddr;

PFN_vkCreateInstance							qvkCreateInstance;
PFN_vkEnumerateInstanceExtensionProperties		qvkEnumerateInstanceExtensionProperties;

PFN_vkCreateDevice								qvkCreateDevice;
PFN_vkDestroyInstance							qvkDestroyInstance;
PFN_vkEnumerateDeviceExtensionProperties		qvkEnumerateDeviceExtensionProperties;
PFN_vkEnumeratePhysicalDevices					qvkEnumeratePhysicalDevices;
PFN_vkGetDeviceProcAddr							qvkGetDeviceProcAddr;
PFN_vkGetPhysicalDeviceFeatures					qvkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties			qvkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceMemoryProperties			qvkGetPhysicalDeviceMemoryProperties;
PFN_vkGetPhysicalDeviceProperties				qvkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties	qvkGetPhysicalDeviceQueueFamilyProperties;


PFN_vkDestroySurfaceKHR							qvkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		qvkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	qvkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR		qvkGetPhysicalDeviceSurfaceSupportKHR;

PFN_vkEnumerateInstanceLayerProperties			qvkEnumerateInstanceLayerProperties;
PFN_vkCreateDebugReportCallbackEXT				qvkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT				qvkDestroyDebugReportCallbackEXT;

PFN_vkAllocateCommandBuffers					qvkAllocateCommandBuffers;
PFN_vkAllocateDescriptorSets					qvkAllocateDescriptorSets;
PFN_vkAllocateMemory							qvkAllocateMemory;
PFN_vkBeginCommandBuffer						qvkBeginCommandBuffer;
PFN_vkBindBufferMemory							qvkBindBufferMemory;
PFN_vkBindImageMemory							qvkBindImageMemory;
PFN_vkCmdBeginRenderPass						qvkCmdBeginRenderPass;
PFN_vkCmdBindDescriptorSets						qvkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer						qvkCmdBindIndexBuffer;
PFN_vkCmdBindPipeline							qvkCmdBindPipeline;
PFN_vkCmdBindVertexBuffers						qvkCmdBindVertexBuffers;
PFN_vkCmdBlitImage								qvkCmdBlitImage;
PFN_vkCmdClearAttachments						qvkCmdClearAttachments;
PFN_vkCmdCopyBufferToImage						qvkCmdCopyBufferToImage;
PFN_vkCmdCopyImage								qvkCmdCopyImage;
PFN_vkCmdCopyImageToBuffer                      qvkCmdCopyImageToBuffer;
PFN_vkCmdDraw									qvkCmdDraw;
PFN_vkCmdDrawIndexed							qvkCmdDrawIndexed;
PFN_vkCmdEndRenderPass							qvkCmdEndRenderPass;
PFN_vkCmdPipelineBarrier						qvkCmdPipelineBarrier;
PFN_vkCmdPushConstants							qvkCmdPushConstants;
PFN_vkCmdSetDepthBias							qvkCmdSetDepthBias;
PFN_vkCmdSetScissor								qvkCmdSetScissor;
PFN_vkCmdSetViewport							qvkCmdSetViewport;
PFN_vkCreateBuffer								qvkCreateBuffer;
PFN_vkCreateCommandPool							qvkCreateCommandPool;
PFN_vkCreateDescriptorPool						qvkCreateDescriptorPool;
PFN_vkCreateDescriptorSetLayout					qvkCreateDescriptorSetLayout;
PFN_vkCreateFence								qvkCreateFence;
PFN_vkCreateFramebuffer							qvkCreateFramebuffer;
PFN_vkCreateGraphicsPipelines					qvkCreateGraphicsPipelines;
PFN_vkCreateImage								qvkCreateImage;
PFN_vkCreateImageView							qvkCreateImageView;
PFN_vkCreatePipelineLayout						qvkCreatePipelineLayout;
PFN_vkCreateRenderPass							qvkCreateRenderPass;
PFN_vkCreateSampler								qvkCreateSampler;
PFN_vkCreateSemaphore							qvkCreateSemaphore;
PFN_vkCreateShaderModule						qvkCreateShaderModule;
PFN_vkDestroyBuffer								qvkDestroyBuffer;
PFN_vkDestroyCommandPool						qvkDestroyCommandPool;
PFN_vkDestroyDescriptorPool						qvkDestroyDescriptorPool;
PFN_vkDestroyDescriptorSetLayout				qvkDestroyDescriptorSetLayout;
PFN_vkDestroyDevice								qvkDestroyDevice;
PFN_vkDestroyFence								qvkDestroyFence;
PFN_vkDestroyFramebuffer						qvkDestroyFramebuffer;
PFN_vkDestroyImage								qvkDestroyImage;
PFN_vkDestroyImageView							qvkDestroyImageView;
PFN_vkDestroyPipeline							qvkDestroyPipeline;
PFN_vkDestroyPipelineLayout						qvkDestroyPipelineLayout;
PFN_vkDestroyRenderPass							qvkDestroyRenderPass;
PFN_vkDestroySampler							qvkDestroySampler;
PFN_vkDestroySemaphore							qvkDestroySemaphore;
PFN_vkDestroyShaderModule						qvkDestroyShaderModule;
PFN_vkDeviceWaitIdle							qvkDeviceWaitIdle;
PFN_vkEndCommandBuffer							qvkEndCommandBuffer;
PFN_vkFreeCommandBuffers						qvkFreeCommandBuffers;
PFN_vkFreeDescriptorSets						qvkFreeDescriptorSets;
PFN_vkFreeMemory								qvkFreeMemory;
PFN_vkGetBufferMemoryRequirements				qvkGetBufferMemoryRequirements;
PFN_vkGetDeviceQueue							qvkGetDeviceQueue;
PFN_vkGetImageMemoryRequirements				qvkGetImageMemoryRequirements;
PFN_vkGetImageSubresourceLayout					qvkGetImageSubresourceLayout;
PFN_vkMapMemory									qvkMapMemory;
PFN_vkUnmapMemory                               qvkUnmapMemory;
PFN_vkQueueSubmit								qvkQueueSubmit;
PFN_vkQueueWaitIdle								qvkQueueWaitIdle;
PFN_vkResetDescriptorPool						qvkResetDescriptorPool;
PFN_vkResetFences								qvkResetFences;
PFN_vkUpdateDescriptorSets						qvkUpdateDescriptorSets;
PFN_vkWaitForFences								qvkWaitForFences;
PFN_vkAcquireNextImageKHR						qvkAcquireNextImageKHR;
PFN_vkCreateSwapchainKHR						qvkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR						qvkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR						qvkGetSwapchainImagesKHR;
PFN_vkQueuePresentKHR							qvkQueuePresentKHR;



static const char * const validation_layer_name = "VK_LAYER_KHRONOS_validation";

static qboolean vk_isInstanceLayerAvailable(const char *name)
{
    uint32_t count = 0, i;
    VkLayerProperties *layers;
    qboolean found = qfalse;

    if (qvkEnumerateInstanceLayerProperties(&count, NULL) != VK_SUCCESS || count == 0)
        return qfalse;

    layers = malloc(sizeof(*layers) * count);
    if (layers == NULL)
        return qfalse;

    if (qvkEnumerateInstanceLayerProperties(&count, layers) == VK_SUCCESS)
        for (i = 0; i < count; i++)
            if (!strcmp(layers[i].layerName, name))
                found = qtrue;

    free(layers);
    return found;
}


VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
        uint64_t object, size_t location, int32_t message_code, 
        const char* layer_prefix, const char* message, void* user_data )
{
    ri.Printf(PRINT_WARNING, "%s\n", message);
	return VK_FALSE;
}


static void vk_createDebugCallback( PFN_vkDebugReportCallbackEXT qvkDebugCB)
{
    if (qvkCreateDebugReportCallbackEXT == NULL)
    {
        ri.Printf( PRINT_WARNING, "r_vkValidation: VK_EXT_debug_report is not available, no debug callback\n" );
        return;
    }

    ri.Printf( PRINT_DEVELOPER, " vk_createDebugCallback() \n" ); 
    
    VkDebugReportCallbackCreateInfoEXT desc;
    desc.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    desc.pNext = NULL;
    desc.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT;
    desc.pfnCallback = qvkDebugCB;
    desc.pUserData = NULL;

    VK_CHECK(qvkCreateDebugReportCallbackEXT(vk.instance, &desc, NULL, &vk.h_debugCB));
}


static void vk_createInstance(void)
{
    // There is no global state in Vulkan and all per-application state
    // is stored in a VkInstance object. Creating a VkInstance object 
    // initializes the Vulkan library and allows the application to pass
    // information about itself to the implementation.
    ri.Printf(PRINT_DEVELOPER, " Creating instance: vk.instance\n");
	
    // The version of Vulkan that is supported by an instance may be 
    // different than the version of Vulkan supported by a device or
    // physical device. Because Vulkan 1.0 implementations may fail
    // with VK_ERROR_INCOMPATIBLE_DRIVER, applications should determine
    // the version of Vulkan available before calling vkCreateInstance.
    // If the vkGetInstanceProcAddr returns NULL for vkEnumerateInstanceVersion,
    // it is a Vulkan 1.0 implementation. Otherwise, the application can
    // call vkEnumerateInstanceVersion to determine the version of Vulkan.

    VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
	appInfo.pApplicationName = PRODUCT_NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 36, 0);
	appInfo.pEngineName = CLIENT_WINDOW_TITLE;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 36, 0);
    // apiVersion must be the highest version of Vulkan that the
    // application is designed to use, encoded as described in the
    // API Version Numbers and Semantics section. The patch version
    // number specified in apiVersion is ignored when creating an 
    // instance object. Only the major and minor versions of the 
    // instance must match those requested in apiVersion.
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);


	VkInstanceCreateInfo instanceCreateInfo;
	
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // pNext is NULL or a pointer to an extension-specific structure.
	instanceCreateInfo.pNext = NULL;
    // flags is set below after scanning instance extensions for
    // VK_KHR_portability_enumeration (needed for MoltenVK on macOS).
    // pApplicationInfo is NULL or a pointer to an instance of
    // VkApplicationInfo. If not NULL, this information helps 
    // implementations recognize behavior inherent to classes
    // of applications.
    instanceCreateInfo.pApplicationInfo = &appInfo;

    
	// check extensions availability
    //
    // Extensions may define new Vulkan commands, structures, and enumerants.
    // For compilation purposes, the interfaces defined by registered extensions,
    // including new structures and enumerants as well as function pointer types
    // for new commands, are defined in the Khronos-supplied vulkan_core.h 
    // together with the core API. However, commands defined by extensions may
    // not be available for static linking - in which case function pointers to
    // these commands should be queried at runtime as described in Command Function
    // Pointers.
    // Extensions may be provided by layers as well as by a Vulkan implementation.
    //
    // check extensions availability
	uint32_t nInsExt = 0, nRequired, nEnabled, i, j;
	VkExtensionProperties *pInsExt;
	const char **ppInstanceExt;
	VkInstanceCreateFlags flags = 0;

	// Enable only what is needed: the surface extensions SDL asks for, plus
	// VK_KHR_portability_enumeration (MoltenVK) and VK_EXT_debug_report
	// (r_vkValidation) when they exist.  Enabling every extension the loader
	// reports is fragile: some depend on others, some conflict.
	VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, NULL) );
	pInsExt = malloc(sizeof(VkExtensionProperties) * (nInsExt ? nInsExt : 1));
	VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, pInsExt) );
	ri.Printf(PRINT_DEVELOPER, "--- Total %d instance extensions. --- \n", nInsExt);

	nRequired = vk_getRequiredInstanceExtensions(NULL, 0);
	ppInstanceExt = malloc(sizeof(char *) * (nRequired + 3));
	vk_getRequiredInstanceExtensions(ppInstanceExt, nRequired);
	for (i = 0; i < nRequired; i++)
	{
		for (j = 0; j < nInsExt && strcmp(ppInstanceExt[i], pInsExt[j].extensionName); j++)
			;
		if (j == nInsExt)
			ri.Error(ERR_FATAL, "Vulkan: instance extension %s, needed for the window surface, is not available", ppInstanceExt[i]);
	}
	nEnabled = nRequired;

	for (j = 0; j < nInsExt; j++)
	{
#ifdef VK_KHR_portability_enumeration
		// MoltenVK (macOS/iOS) enumerates portability-subset devices only
		// with this extension and flag
		if (!strcmp(pInsExt[j].extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
		{
			ppInstanceExt[nEnabled++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
			flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		}
		// MoltenVK's devices need VK_KHR_portability_subset, which requires this
		if (!strcmp(pInsExt[j].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			for (i = 0; i < nEnabled && strcmp(ppInstanceExt[i], pInsExt[j].extensionName); i++)
				;
			if (i == nEnabled)
				ppInstanceExt[nEnabled++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
		}
#endif
		if (r_vkValidation->integer && !strcmp(pInsExt[j].extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
			ppInstanceExt[nEnabled++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	}

	for (i = 0; i < nEnabled; i++)
		ri.Printf(PRINT_DEVELOPER, " Enabling instance extension %s\n", ppInstanceExt[i]);

	instanceCreateInfo.flags = flags;
	instanceCreateInfo.enabledExtensionCount = nEnabled;
	instanceCreateInfo.ppEnabledExtensionNames = ppInstanceExt;

    instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = NULL;

    if (r_vkValidation->integer)
    {
        if (vk_isInstanceLayerAvailable(validation_layer_name))
        {
            ri.Printf(PRINT_ALL, "Using %s\n", validation_layer_name);
            instanceCreateInfo.enabledLayerCount = 1;
            instanceCreateInfo.ppEnabledLayerNames = &validation_layer_name;
        }
        else
        {
            ri.Printf(PRINT_WARNING, "r_vkValidation is set, but %s is not installed\n", validation_layer_name);
        }
    }


    VkResult e = qvkCreateInstance(&instanceCreateInfo, NULL, &vk.instance);
    if(e == VK_SUCCESS)
    {
        ri.Printf(PRINT_DEVELOPER, "--- Vulkan create instance success! ---\n\n");
    }
    else if (e == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		// The requested version of Vulkan is not supported by the driver 
		// or is otherwise incompatible for implementation-specific reasons.
        ri.Error(ERR_FATAL, 
            "The requested version of Vulkan is not supported by the driver." );
    }
    else if (e == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        ri.Error(ERR_FATAL, "Cannot find a specified extension library.");
    }
    else 
    {
        ri.Error(ERR_FATAL, "Vulkan: vkCreateInstance returned %s", cvtResToStr(e));
    }
   
    free(ppInstanceExt);

    free(pInsExt);
}



static void vk_loadGlobalFunctions(void)
{
    ri.Printf(PRINT_DEVELOPER, " Loading vulkan instance functions \n");

    vk_getInstanceProcAddrImpl();

    #define INIT_INSTANCE_FUNCTION(func)                                \
    q##func = (PFN_ ## func)qvkGetInstanceProcAddr(vk.instance, #func); \
    if (q##func == NULL) {                                              \
        ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func);     \
    }

	INIT_INSTANCE_FUNCTION(vkCreateInstance)
	INIT_INSTANCE_FUNCTION(vkEnumerateInstanceExtensionProperties)
	INIT_INSTANCE_FUNCTION(vkEnumerateInstanceLayerProperties)

    //
	// Get instance level functions.
	//
	vk_createInstance();


	INIT_INSTANCE_FUNCTION(vkCreateDevice)
	INIT_INSTANCE_FUNCTION(vkDestroyInstance)
    INIT_INSTANCE_FUNCTION(vkDestroySurfaceKHR)
	INIT_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)
	INIT_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)
	INIT_INSTANCE_FUNCTION(vkGetDeviceProcAddr)

    INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)
    INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)

    INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)
   

    // optional: only present with VK_EXT_debug_report, used by r_vkValidation
    qvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
        qvkGetInstanceProcAddr(vk.instance, "vkCreateDebugReportCallbackEXT");
    qvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
        qvkGetInstanceProcAddr(vk.instance, "vkDestroyDebugReportCallbackEXT");

    #undef INIT_INSTANCE_FUNCTION

    ri.Printf(PRINT_DEVELOPER, " Init global functions done. \n");
}

////////////////////////////////


static void vk_selectPhysicalDevice(void)
{
    // After initializing the Vulkan library through a VkInstance
    // we need to look for and select a graphics card in the system
    // that supports the features we need. In fact we can select any
    // number of graphics cards and use them simultaneously.
	uint32_t gpu_count = 0;

    // Initial call to query gpu_count, then second call for gpu info.
	qvkEnumeratePhysicalDevices(vk.instance, &gpu_count, NULL);

	if (gpu_count <= 0)
		ri.Error(ERR_FATAL, "Vulkan: no physical device found");

    VkPhysicalDevice *pPhyDev = (VkPhysicalDevice *) malloc (sizeof(VkPhysicalDevice) * gpu_count);
    
    // TODO: multi graphic cards selection support
    VK_CHECK(qvkEnumeratePhysicalDevices(vk.instance, &gpu_count, pPhyDev));
    // For demo app we just grab the first physical device
    vk.physical_device = pPhyDev[0];
	
    free(pPhyDev);

    ri.Printf(PRINT_DEVELOPER, " Total %d graphics card, the first one is choosed. \n", gpu_count);

    ri.Printf(PRINT_DEVELOPER, " Get physical device memory properties: vk.devMemProperties \n");
    qvkGetPhysicalDeviceMemoryProperties(vk.physical_device, &vk.devMemProperties);
}



static void vk_selectSurfaceFormat(void)
{
    uint32_t nSurfmt;
    
    ri.Printf(PRINT_DEVELOPER, "\n -------- vk_selectSurfaceFormat() -------- \n");


    // Get the numbers of VkFormat's that are supported
    // "vk.surface" is the surface that will be associated with the swapchain.
    // "vk.surface" must be a valid VkSurfaceKHR handle
    VK_CHECK(qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &nSurfmt, NULL));
    assert(nSurfmt > 0);

    VkSurfaceFormatKHR *pSurfFmts = 
        (VkSurfaceFormatKHR *) malloc ( nSurfmt * sizeof(VkSurfaceFormatKHR) );

    // To query the supported swapchain format-color space pairs for a surface
    VK_CHECK(qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &nSurfmt, pSurfFmts));

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface
    // has no preferred format. Otherwise, at least one supported format will be returned.
    if ( (nSurfmt == 1) && (pSurfFmts[0].format == VK_FORMAT_UNDEFINED) )
    {
        // special case that means we can choose any format
        vk.surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        vk.surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        ri.Printf(PRINT_DEVELOPER, "VK_FORMAT_R8G8B8A8_UNORM\n");
        ri.Printf(PRINT_DEVELOPER, "VK_COLORSPACE_SRGB_NONLINEAR_KHR\n");
    }
    else
    {
        uint32_t i;
        ri.Printf(PRINT_DEVELOPER, " Total %d surface formats supported, we choose: \n", nSurfmt);

        for( i = 0; i < nSurfmt; i++)
        {
            if( ( pSurfFmts[i].format == VK_FORMAT_B8G8R8A8_UNORM) &&
                ( pSurfFmts[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) )
            {

                ri.Printf(PRINT_DEVELOPER, " format = VK_FORMAT_B8G8R8A8_UNORM \n");
                ri.Printf(PRINT_DEVELOPER, " colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR \n");
                
                vk.surface_format = pSurfFmts[i];
                break;
            }
        }

        if (i == nSurfmt)
            vk.surface_format = pSurfFmts[0];
    }

    free(pSurfFmts);


    // To query the basic capabilities of a surface, needed in order to create a swapchain
	VK_CHECK(qvkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, vk.surface, &vk.surface_caps));

    // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required by image clear operations.
	if ((vk.surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
		ri.Error(ERR_FATAL, "VK_IMAGE_USAGE_TRANSFER_DST_BIT is not supported by you GPU.");

	// VK_IMAGE_USAGE_TRANSFER_SRC_BIT is required in order to take screenshots.
	if ((vk.surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0)
		ri.Error(ERR_FATAL, "VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not supported by you GPU.");


    // To query supported format features which are properties of the physical device
	
    VkFormatProperties props;


    // To determine the set of valid usage bits for a given format,
    // call vkGetPhysicalDeviceFormatProperties.

    // ========================= color ================
    qvkGetPhysicalDeviceFormatProperties(vk.physical_device, vk.surface_format.format, &props);
    
    // Check if the device supports blitting to linear images 
    if ( props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
        ri.Printf(PRINT_DEVELOPER, "--- Linear TilingFeatures supported. ---\n");

    if ( props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) 
    {
        ri.Printf(PRINT_DEVELOPER, "--- Blitting from linear tiled images supported. ---\n");
    }

    if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT )
    {
        ri.Printf(PRINT_DEVELOPER, "--- Blitting from optimal tiled images supported. ---\n");
        vk.isBlitSupported = VK_TRUE;
    }


    //=========================== depth =====================================
    qvkGetPhysicalDeviceFormatProperties(vk.physical_device, VK_FORMAT_D24_UNORM_S8_UINT, &props);
    if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
    {
        ri.Printf(PRINT_DEVELOPER, " VK_FORMAT_D24_UNORM_S8_UINT optimal Tiling feature supported.\n");
        vk.fmt_DepthStencil = VK_FORMAT_D24_UNORM_S8_UINT;
    }
    else
    {
        qvkGetPhysicalDeviceFormatProperties(vk.physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);

        if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
        {
            ri.Printf(PRINT_DEVELOPER, " VK_FORMAT_D32_SFLOAT_S8_UINT optimal Tiling feature supported.\n");
            vk.fmt_DepthStencil = VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        else
        {
            //formats[0] = VK_FORMAT_X8_D24_UNORM_PACK32;
		    //formats[1] = VK_FORMAT_D32_SFLOAT;
            // never get here.
	        ri.Error(ERR_FATAL, " Failed to find depth attachment format.");
        }
    }

    ri.Printf(PRINT_DEVELOPER, " -------- --------------------------- --------\n");
}


static void vk_selectQueueFamilyForPresentation(void)
{
    // Almosty every operation in Vulkan, anything from drawing textures,
    // requires commands to be submitted to a queue. There are different
    // types of queues that originate from differnet queue families and
    // each family of queues allows only a subset of commands. 
    // For example, there could be a queue family allows processing of 
    // compute commands or one that only allows memory thansfer related
    // commands. We need to check which queue families are supported by
    // the device and which one of these supports the commands that we use.


    uint32_t nSurfmt;
    qvkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &nSurfmt, NULL);
    
    assert(nSurfmt > 0);

    VkQueueFamilyProperties* pQueueFamilies = (VkQueueFamilyProperties *) malloc (
            nSurfmt * sizeof(VkQueueFamilyProperties) );

    // To query properties of queues available on a physical device
    qvkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &nSurfmt, pQueueFamilies);

    // Select queue family with presentation and graphics support
    // Iterate over each queue to learn whether it supports presenting:
    vk.queue_family_index = -1;
    
    uint32_t i;
    for (i = 0; i < nSurfmt; ++i)
    {
        // To look for a queue family that has the capability of presenting
        // to our window surface
        
        VkBool32 presentation_supported = VK_FALSE;
        VK_CHECK(qvkGetPhysicalDeviceSurfaceSupportKHR(
                    vk.physical_device, i, vk.surface, &presentation_supported));

        if (presentation_supported && 
                (pQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            vk.queue_family_index = i;
            
            ri.Printf(PRINT_DEVELOPER, " Queue family for presentation selected: %d\n",
                    vk.queue_family_index);

            break;
        }
    }

    free(pQueueFamilies);

    if (vk.queue_family_index == -1)
        ri.Error(ERR_FATAL, "Vulkan: failed to find queue family");
}


static void vk_createLogicalDevice(void)
{
    const char* device_extensions[2];
    uint32_t device_extension_count = 0;

    device_extensions[device_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    //  Not all graphics cards are capble of presenting images directly
    //  to a screen for various reasons, for example because they are 
    //  designed for servers and don't have any display outputs. 
    //  Secondly, since image presentation is heavily tied into the 
    //  window system and the surfaces associated with windows, it is
    //  not actually part of the vulkan core. You have to enable the
    //  VK_KHR_swapchain device extension after querying for its support.
    uint32_t nDevExts = 0;
    VkBool32 swapchainExtFound = 0;

    // To query the numbers of extensions available to a given physical device
    ri.Printf( PRINT_DEVELOPER, " Check for VK_KHR_swapchain extension. \n" );

    qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, NULL);

    VkExtensionProperties* pDeviceExt = 
        (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nDevExts);

    qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, pDeviceExt);


    uint32_t j;
    for (j = 0; j < nDevExts; j++)
    {
        if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, pDeviceExt[j].extensionName))
        {
            swapchainExtFound = VK_TRUE;
        }
        // VK_KHR_portability_subset must be enabled on devices that
        // report it (required by the Vulkan spec). This is the case
        // for MoltenVK on macOS/iOS.
        if (!strcmp("VK_KHR_portability_subset", pDeviceExt[j].extensionName))
        {
            device_extensions[device_extension_count++] = "VK_KHR_portability_subset";
            ri.Printf( PRINT_DEVELOPER, " Enabling %s device extension.\n", "VK_KHR_portability_subset" );
        }
    }
    if (VK_FALSE == swapchainExtFound)
        ri.Error(ERR_FATAL, "VK_KHR_SWAPCHAIN_EXTENSION_NAME is not available");

    free(pDeviceExt);


    const float priority = 1.0;
    VkDeviceQueueCreateInfo queue_desc;
    queue_desc.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_desc.pNext = NULL;
    queue_desc.flags = 0;
    queue_desc.queueFamilyIndex = vk.queue_family_index;
    queue_desc.queueCount = 1;
    queue_desc.pQueuePriorities = &priority;


    // Query fine-grained feature support for this device. If APP 
    // has specific feature requirements it should check supported
    // features based on this query.
	qvkGetPhysicalDeviceFeatures(vk.physical_device, &vk.features);

    VkDeviceCreateInfo device_desc;
    device_desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_desc.pNext = NULL;
    device_desc.flags = 0;
    device_desc.queueCreateInfoCount = 1;
    device_desc.pQueueCreateInfos = &queue_desc;
    device_desc.enabledLayerCount = 0;
    device_desc.ppEnabledLayerNames = NULL;
    device_desc.enabledExtensionCount = device_extension_count;
    device_desc.ppEnabledExtensionNames = device_extensions;
    device_desc.pEnabledFeatures = &vk.features;
    

    // After selecting a physical device to use we need to set up a
    // logical device to interface with it. The logical device 
    // creation process id similar to the instance creation process
    // and describes the features we want to use. we also need to 
    // specify which queues to create now that we've queried which
    // queue families are available. You can create multiple logical
    // devices from the same physical device if you have varying requirements.
    ri.Printf( PRINT_DEVELOPER, " Create logical device: vk.device \n" );
    VK_CHECK(qvkCreateDevice(vk.physical_device, &device_desc, NULL, &vk.device));

}


static void vk_loadDeviceFunctions(void)
{
    ri.Printf( PRINT_DEVELOPER, " Loading device level function. \n" );

    #define INIT_DEVICE_FUNCTION(func)                              \
    q##func = (PFN_ ## func)qvkGetDeviceProcAddr(vk.device, #func); \
    if (q##func == NULL) {                                          \
        ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func); \
    }     

	INIT_DEVICE_FUNCTION(vkAllocateCommandBuffers)
	INIT_DEVICE_FUNCTION(vkAllocateDescriptorSets)
	INIT_DEVICE_FUNCTION(vkAllocateMemory)
	INIT_DEVICE_FUNCTION(vkBeginCommandBuffer)
	INIT_DEVICE_FUNCTION(vkBindBufferMemory)
	INIT_DEVICE_FUNCTION(vkBindImageMemory)
	INIT_DEVICE_FUNCTION(vkCmdBeginRenderPass)
	INIT_DEVICE_FUNCTION(vkCmdBindDescriptorSets)
	INIT_DEVICE_FUNCTION(vkCmdBindIndexBuffer)
	INIT_DEVICE_FUNCTION(vkCmdBindPipeline)
	INIT_DEVICE_FUNCTION(vkCmdBindVertexBuffers)
	INIT_DEVICE_FUNCTION(vkCmdBlitImage)
	INIT_DEVICE_FUNCTION(vkCmdClearAttachments)
	INIT_DEVICE_FUNCTION(vkCmdCopyBufferToImage)
	INIT_DEVICE_FUNCTION(vkCmdCopyImage)
    INIT_DEVICE_FUNCTION(vkCmdCopyImageToBuffer)
	INIT_DEVICE_FUNCTION(vkCmdDraw)
	INIT_DEVICE_FUNCTION(vkCmdDrawIndexed)
	INIT_DEVICE_FUNCTION(vkCmdEndRenderPass)
	INIT_DEVICE_FUNCTION(vkCmdPipelineBarrier)
	INIT_DEVICE_FUNCTION(vkCmdPushConstants)
	INIT_DEVICE_FUNCTION(vkCmdSetDepthBias)
	INIT_DEVICE_FUNCTION(vkCmdSetScissor)
	INIT_DEVICE_FUNCTION(vkCmdSetViewport)
	INIT_DEVICE_FUNCTION(vkCreateBuffer)
	INIT_DEVICE_FUNCTION(vkCreateCommandPool)
	INIT_DEVICE_FUNCTION(vkCreateDescriptorPool)
	INIT_DEVICE_FUNCTION(vkCreateDescriptorSetLayout)
	INIT_DEVICE_FUNCTION(vkCreateFence)
	INIT_DEVICE_FUNCTION(vkCreateFramebuffer)
	INIT_DEVICE_FUNCTION(vkCreateGraphicsPipelines)
	INIT_DEVICE_FUNCTION(vkCreateImage)
	INIT_DEVICE_FUNCTION(vkCreateImageView)
	INIT_DEVICE_FUNCTION(vkCreatePipelineLayout)
	INIT_DEVICE_FUNCTION(vkCreateRenderPass)
	INIT_DEVICE_FUNCTION(vkCreateSampler)
	INIT_DEVICE_FUNCTION(vkCreateSemaphore)
	INIT_DEVICE_FUNCTION(vkCreateShaderModule)
	INIT_DEVICE_FUNCTION(vkDestroyBuffer)
	INIT_DEVICE_FUNCTION(vkDestroyCommandPool)
	INIT_DEVICE_FUNCTION(vkDestroyDescriptorPool)
	INIT_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout)
	INIT_DEVICE_FUNCTION(vkDestroyDevice)
	INIT_DEVICE_FUNCTION(vkDestroyFence)
	INIT_DEVICE_FUNCTION(vkDestroyFramebuffer)
	INIT_DEVICE_FUNCTION(vkDestroyImage)
	INIT_DEVICE_FUNCTION(vkDestroyImageView)
	INIT_DEVICE_FUNCTION(vkDestroyPipeline)
	INIT_DEVICE_FUNCTION(vkDestroyPipelineLayout)
	INIT_DEVICE_FUNCTION(vkDestroyRenderPass)
	INIT_DEVICE_FUNCTION(vkDestroySampler)
	INIT_DEVICE_FUNCTION(vkDestroySemaphore)
	INIT_DEVICE_FUNCTION(vkDestroyShaderModule)
	INIT_DEVICE_FUNCTION(vkDeviceWaitIdle)
	INIT_DEVICE_FUNCTION(vkEndCommandBuffer)
	INIT_DEVICE_FUNCTION(vkFreeCommandBuffers)
	INIT_DEVICE_FUNCTION(vkFreeDescriptorSets)
	INIT_DEVICE_FUNCTION(vkFreeMemory)
	INIT_DEVICE_FUNCTION(vkGetBufferMemoryRequirements)
	INIT_DEVICE_FUNCTION(vkGetDeviceQueue)
	INIT_DEVICE_FUNCTION(vkGetImageMemoryRequirements)
	INIT_DEVICE_FUNCTION(vkGetImageSubresourceLayout)
	INIT_DEVICE_FUNCTION(vkMapMemory)
	INIT_DEVICE_FUNCTION(vkUnmapMemory)
	INIT_DEVICE_FUNCTION(vkQueueSubmit)
	INIT_DEVICE_FUNCTION(vkQueueWaitIdle)
	INIT_DEVICE_FUNCTION(vkResetDescriptorPool)
	INIT_DEVICE_FUNCTION(vkResetFences)
	INIT_DEVICE_FUNCTION(vkUpdateDescriptorSets)
	INIT_DEVICE_FUNCTION(vkWaitForFences)
    
	INIT_DEVICE_FUNCTION(vkCreateSwapchainKHR)
	INIT_DEVICE_FUNCTION(vkDestroySwapchainKHR)
	INIT_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)
    INIT_DEVICE_FUNCTION(vkAcquireNextImageKHR)
	INIT_DEVICE_FUNCTION(vkQueuePresentKHR)

    #undef INIT_DEVICE_FUNCTION
}



void vk_getProcAddress(void)
{
    vk_loadGlobalFunctions();

    if (r_vkValidation->integer)
        vk_createDebugCallback(vk_DebugCallback);

    // The window surface needs to be created right after the instance creation,
    // because it can actually influence the presentation mode selection.
	vk_createSurfaceImpl(); 
   
    // select physical device
    vk_selectPhysicalDevice();

    vk_selectSurfaceFormat(); 

    vk_selectQueueFamilyForPresentation();

    vk_createLogicalDevice();

    // Get device level functions.
    vk_loadDeviceFunctions();

    // a call to retrieve queue handle
	qvkGetDeviceQueue(vk.device, vk.queue_family_index, 0, &vk.queue);
}


void vk_clearProcAddress(void)
{

    ri.Printf( PRINT_DEVELOPER, " Destroy logical device: vk.device. \n" );
    // Device queues are implicitly cleaned up when the device is destroyed
    // so we don't need to do anything in clean up
    if (vk.device != VK_NULL_HANDLE && qvkDestroyDevice != NULL)
        qvkDestroyDevice(vk.device, NULL);
    
    ri.Printf( PRINT_DEVELOPER, " Destroy surface: vk.surface. \n" );
    // make sure that the surface is destroyed before the instance
    if (vk.surface != VK_NULL_HANDLE && qvkDestroySurfaceKHR != NULL)
        qvkDestroySurfaceKHR(vk.instance, vk.surface, NULL);

    if (vk.h_debugCB != VK_NULL_HANDLE && qvkDestroyDebugReportCallbackEXT != NULL)
    {
        ri.Printf( PRINT_DEVELOPER, " Destroy callback function: vk.h_debugCB. \n" );
        qvkDestroyDebugReportCallbackEXT(vk.instance, vk.h_debugCB, NULL);
        vk.h_debugCB = VK_NULL_HANDLE;
    }

    ri.Printf( PRINT_DEVELOPER, " Destroy instance: vk.instance. \n" );
    if (vk.instance != VK_NULL_HANDLE && qvkDestroyInstance != NULL)
        qvkDestroyInstance(vk.instance, NULL);

// ===========================================================
    ri.Printf( PRINT_DEVELOPER, " clear all proc address \n" );

	qvkCreateInstance                           = NULL;
	qvkEnumerateInstanceExtensionProperties		= NULL;

	qvkCreateDevice								= NULL;
	qvkDestroyInstance							= NULL;
	qvkEnumerateDeviceExtensionProperties		= NULL;
	qvkEnumeratePhysicalDevices					= NULL;
	qvkGetDeviceProcAddr						= NULL;
	qvkGetPhysicalDeviceFeatures				= NULL;
	qvkGetPhysicalDeviceFormatProperties		= NULL;
	qvkGetPhysicalDeviceMemoryProperties		= NULL;
	qvkGetPhysicalDeviceProperties				= NULL;
	qvkGetPhysicalDeviceQueueFamilyProperties	= NULL;

    qvkDestroySurfaceKHR						= NULL;
	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR	= NULL;
	qvkGetPhysicalDeviceSurfaceFormatsKHR		= NULL;
	qvkGetPhysicalDeviceSurfacePresentModesKHR	= NULL;
	qvkGetPhysicalDeviceSurfaceSupportKHR		= NULL;
	qvkEnumerateInstanceLayerProperties			= NULL;
	qvkCreateDebugReportCallbackEXT				= NULL;
	qvkDestroyDebugReportCallbackEXT			= NULL;

	qvkAllocateCommandBuffers					= NULL;
	qvkAllocateDescriptorSets					= NULL;
	qvkAllocateMemory							= NULL;
	qvkBeginCommandBuffer						= NULL;
	qvkBindBufferMemory							= NULL;
	qvkBindImageMemory							= NULL;
	qvkCmdBeginRenderPass						= NULL;
	qvkCmdBindDescriptorSets					= NULL;
	qvkCmdBindIndexBuffer						= NULL;
	qvkCmdBindPipeline							= NULL;
	qvkCmdBindVertexBuffers						= NULL;
	qvkCmdBlitImage								= NULL;
	qvkCmdClearAttachments						= NULL;
	qvkCmdCopyBufferToImage						= NULL;
	qvkCmdCopyImage								= NULL;
    qvkCmdCopyImageToBuffer                     = NULL;
	qvkCmdDraw									= NULL;
	qvkCmdDrawIndexed							= NULL;
	qvkCmdEndRenderPass							= NULL;
	qvkCmdPipelineBarrier						= NULL;
	qvkCmdPushConstants							= NULL;
	qvkCmdSetDepthBias							= NULL;
	qvkCmdSetScissor							= NULL;
	qvkCmdSetViewport							= NULL;
	qvkCreateBuffer								= NULL;
	qvkCreateCommandPool						= NULL;
	qvkCreateDescriptorPool						= NULL;
	qvkCreateDescriptorSetLayout				= NULL;
	qvkCreateFence								= NULL;
	qvkCreateFramebuffer						= NULL;
	qvkCreateGraphicsPipelines					= NULL;
	qvkCreateImage								= NULL;
	qvkCreateImageView							= NULL;
	qvkCreatePipelineLayout						= NULL;
	qvkCreateRenderPass							= NULL;
	qvkCreateSampler							= NULL;
	qvkCreateSemaphore							= NULL;
	qvkCreateShaderModule						= NULL;
	qvkDestroyBuffer							= NULL;
	qvkDestroyCommandPool						= NULL;
	qvkDestroyDescriptorPool					= NULL;
	qvkDestroyDescriptorSetLayout				= NULL;
	qvkDestroyDevice							= NULL;
	qvkDestroyFence								= NULL;
	qvkDestroyFramebuffer						= NULL;
	qvkDestroyImage								= NULL;
	qvkDestroyImageView							= NULL;
	qvkDestroyPipeline							= NULL;
	qvkDestroyPipelineLayout					= NULL;
	qvkDestroyRenderPass						= NULL;
	qvkDestroySampler							= NULL;
	qvkDestroySemaphore							= NULL;
	qvkDestroyShaderModule						= NULL;
	qvkDeviceWaitIdle							= NULL;
	qvkEndCommandBuffer							= NULL;
	qvkFreeCommandBuffers						= NULL;
	qvkFreeDescriptorSets						= NULL;
	qvkFreeMemory								= NULL;
	qvkGetBufferMemoryRequirements				= NULL;
	qvkGetDeviceQueue							= NULL;
	qvkGetImageMemoryRequirements				= NULL;
	qvkGetImageSubresourceLayout				= NULL;
	qvkMapMemory								= NULL;
    qvkUnmapMemory                              = NULL;
	qvkQueueSubmit								= NULL;
	qvkQueueWaitIdle							= NULL;
	qvkResetDescriptorPool						= NULL;
	qvkResetFences								= NULL;
	qvkUpdateDescriptorSets						= NULL;
	qvkWaitForFences							= NULL;
	qvkAcquireNextImageKHR						= NULL;
	qvkCreateSwapchainKHR						= NULL;
	qvkDestroySwapchainKHR						= NULL;
	qvkGetSwapchainImagesKHR					= NULL;
	qvkQueuePresentKHR							= NULL;
}


const char * cvtResToStr(VkResult result)
{
    switch(result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
            return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
//
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "VK_ERROR_NOT_PERMITTED_EXT";
//
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
        case VK_ERROR_FRAGMENTATION_EXT:
            return "VK_ERROR_FRAGMENTATION_EXT";
    }

    return "UNKNOWN_ERROR";
}
