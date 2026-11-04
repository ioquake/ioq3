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

#include "tr_local.h"
#include "ref_import.h"
#include "tr_cvar.h"

#include "../renderercommon/tr_types.h"

#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_image.h"
// outside of this file shouldn't modify glConfig
// I want keep it locally, as it belong to OpenGL, not VulKan
// have to use this keep backward Compatibility
static glconfig_t glConfig;


static cvar_t* r_customwidth;
static cvar_t* r_customheight;
static cvar_t* r_customaspect;

typedef struct vidmode_s
{
    const char *description;
    int         width, height;
	float		pixelAspect;		// pixel width / height
} vidmode_t;


static const vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480 (480p)",	640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480",		856,	480,	1 }, // Q3 MODES END HERE AND EXTENDED MODES BEGIN
	{ "Mode 12: 1280x720 (720p)",	1280,	720,	1 },
	{ "Mode 13: 1280x768",		1280,	768,	1 },
	{ "Mode 14: 1280x800",		1280,	800,	1 },
	{ "Mode 15: 1280x960",		1280,	960,	1 },
	{ "Mode 16: 1360x768",		1360,	768,	1 },
	{ "Mode 17: 1366x768",		1366,	768,	1 }, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024,	1 },
	{ "Mode 19: 1400x1050",		1400,	1050,	1 },
	{ "Mode 20: 1400x900",		1400,	900,	1 },
	{ "Mode 21: 1600x900",		1600,	900,	1 },
	{ "Mode 22: 1680x1050",		1680,	1050,	1 },
	{ "Mode 23: 1920x1080 (1080p)",	1920,	1080,	1 },
	{ "Mode 24: 1920x1200",		1920,	1200,	1 },
	{ "Mode 25: 1920x1440",		1920,	1440,	1 },
    { "Mode 26: 2560x1080",		2560,	1080,	1 },
    { "Mode 27: 2560x1600",		2560,	1600,	1 },
	{ "Mode 28: 3840x2160 (4K)",	3840,	2160,	1 }
};
static const int s_numVidModes = 29;


void R_DisplayResolutionList_f( void )
{
	int i;

	ri.Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf( PRINT_ALL, "\n" );
}


void R_SetWinMode(int mode, unsigned int width, unsigned int height, unsigned int hz)
{
	
    if ( mode < -2 || mode >= s_numVidModes) {
         mode = 3;
	}

	if (mode == -2)
	{
        // use desktop video resolution
        glConfig.vidWidth = width;
        glConfig.vidHeight = height;
        glConfig.windowAspect = (float)width / (float)height;
        glConfig.displayFrequency = hz;
        glConfig.isFullscreen = 1;
    }
	else if ( mode == -1 )
    {
		glConfig.vidWidth = r_customwidth->integer;
		glConfig.vidHeight = r_customheight->integer;
		glConfig.windowAspect = r_customaspect->value;
        glConfig.displayFrequency = 60;
        glConfig.isFullscreen = 0;
	} 
    else
    {
        glConfig.vidWidth = r_vidModes[mode].width;
        glConfig.vidHeight = r_vidModes[mode].height;
        glConfig.windowAspect = (float)r_vidModes[mode].width / ( r_vidModes[mode].height * r_vidModes[mode].pixelAspect );
        glConfig.displayFrequency = 60;
        glConfig.isFullscreen = 0;

    }
    
	ri.Printf(PRINT_ALL,  " MODE: %d, %d x %d, refresh rate: %dhz\n",
        mode, glConfig.vidWidth, glConfig.vidHeight, glConfig.displayFrequency);
}

void R_GetWinResolution(int* w, int* h)
{
    *w = glConfig.vidWidth;
    *h = glConfig.vidHeight;
}

void R_GetWinResolutionF(float* w, float* h)
{
    *w = glConfig.vidWidth;
    *h = glConfig.vidHeight;
}

void R_InitDisplayResolution( void )
{
    // leilei - -2 is so convenient for modern day PCs
    r_mode = ri.Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );
    r_customwidth = ri.Cvar_Get( "r_customwidth", "960", CVAR_ARCHIVE | CVAR_LATCH );
    r_customheight = ri.Cvar_Get( "r_customheight", "540", CVAR_ARCHIVE | CVAR_LATCH );
    r_customaspect = ri.Cvar_Get( "r_customaspect", "1.78", CVAR_ARCHIVE | CVAR_LATCH );
}

void R_glConfigClear(void)
{
    memset(&glConfig, 0, sizeof(glConfig));
}


//IN: a pointer to the glConfig struct
void R_GetGlConfig(glconfig_t * const pCfg)
{
	*pCfg = glConfig;
}


void R_glConfigInit(void)
{
    ri.Printf(PRINT_DEVELOPER,  "--- R_glConfigInit() ---\n");

    // These values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

    // Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
    glConfig.deviceSupportsGamma = qtrue;

    glConfig.textureEnvAddAvailable = 0; // not used
    glConfig.textureCompression = TC_NONE; // not used
	// init command buffers and SMP
	glConfig.stereoEnabled = 0;
	glConfig.smpActive = qfalse; // not used

    // hardcode it
    glConfig.colorBits = 32;
    glConfig.depthBits = 24;
    glConfig.stencilBits = 8;
}

// ==================================================
// ==================================================

static const char *vendorName(uint32_t vendorID)
{
	switch (vendorID)
	{
	case 0x1002: return "Advanced Micro Devices, Inc.";
	case 0x1010: return "Imagination Technologies";
	case 0x106B: return "Apple Inc.";
	case 0x10DE: return "NVIDIA";
	case 0x13B5: return "ARM";
	case 0x5143: return "Qualcomm";
	case 0x8086: return "Intel Corporation";
	case 0x10005: return "Mesa";	// VK_VENDOR_ID_MESA, e.g. lavapipe
	default: return "unknown";
	}
}


static const char *deviceTypeName(VkPhysicalDeviceType type)
{
	switch (type)
	{
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "integrated GPU";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "discrete GPU";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "virtual GPU";
	case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
	default: return "other";
	}
}


// driverVersion is vendor-specific, most drivers use the VK_MAKE_VERSION encoding
static void driverVersionString(const VkPhysicalDeviceProperties *props, char *buf, size_t size)
{
	uint32_t v = props->driverVersion;

	if (props->vendorID == 0x10DE)
	{
		Com_sprintf(buf, size, "%u.%u.%u.%u",
			(v >> 22) & 0x3ff, (v >> 14) & 0xff, (v >> 6) & 0xff, v & 0x3f);
	}
#ifdef _WIN32
	else if (props->vendorID == 0x8086)
	{
		Com_sprintf(buf, size, "%u.%u", v >> 14, v & 0x3fff);
	}
#endif
	else
	{
		Com_sprintf(buf, size, "%u.%u.%u",
			VK_VERSION_MAJOR(v), VK_VERSION_MINOR(v), VK_VERSION_PATCH(v));
	}
}


// Print "label: ext1 ext2 ...", like GL_EXTENSIONS in the GL renderers.
// Returns the list, to be freed by the caller, or NULL.
static VkExtensionProperties *printExtensions(const char *label, qboolean device, uint32_t *count)
{
	VkExtensionProperties *ext = NULL;
	uint32_t i;
	VkResult result;

	*count = 0;
	if (device)
		result = qvkEnumerateDeviceExtensionProperties(vk.physical_device, NULL, count, NULL);
	else
		result = qvkEnumerateInstanceExtensionProperties(NULL, count, NULL);

	if (result == VK_SUCCESS && *count > 0)
	{
		ext = malloc(sizeof(VkExtensionProperties) * *count);
		if (ext != NULL)
		{
			if (device)
				result = qvkEnumerateDeviceExtensionProperties(vk.physical_device, NULL, count, ext);
			else
				result = qvkEnumerateInstanceExtensionProperties(NULL, count, ext);
		}
		if (ext == NULL || result != VK_SUCCESS)
		{
			free(ext);
			ext = NULL;
			*count = 0;
		}
	}

	ri.Printf(PRINT_ALL, "%s: ", label);
	for (i = 0; i < *count; i++)
		ri.Printf(PRINT_ALL, "%s ", ext[i].extensionName);
	ri.Printf(PRINT_ALL, "\n");

	return ext;
}


void vulkanInfo_f( void )
{
	const char *fsstrings[] = { "windowed", "fullscreen" };
	VkPhysicalDeviceProperties props;
	VkExtensionProperties *ext;
	uint32_t numExt, i;
	size_t len = 0, nameLen;
	char driverVersion[32];

	// To query general properties of physical devices once enumerated
	qvkGetPhysicalDeviceProperties(vk.physical_device, &props);
	driverVersionString(&props, driverVersion, sizeof(driverVersion));

	ri.Printf(PRINT_ALL, "\nVK_VENDOR: %s (0x%X)\n", vendorName(props.vendorID), props.vendorID);
	ri.Printf(PRINT_ALL, "VK_RENDERER: %s (%s, device 0x%X)\n",
		props.deviceName, deviceTypeName(props.deviceType), props.deviceID);
	ri.Printf(PRINT_ALL, "VK_VERSION: %u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
		VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
	ri.Printf(PRINT_ALL, "VK_DRIVER_VERSION: %s\n", driverVersion);

	Q_strncpyz(glConfig.vendor_string, vendorName(props.vendorID), sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string, props.deviceName, sizeof(glConfig.renderer_string));
	Com_sprintf(glConfig.version_string, sizeof(glConfig.version_string), "Vulkan %u.%u.%u, driver %s",
		VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
		VK_VERSION_PATCH(props.apiVersion), driverVersion);

	// The instance extensions go to glConfig.extensions_string, for the
	// driver info menu (all of them are enabled); there are too many device
	// extensions for it
	ext = printExtensions("VK_INSTANCE_EXTENSIONS", qfalse, &numExt);
	glConfig.extensions_string[0] = '\0';
	for (i = 0; i < numExt; i++)
	{
		nameLen = strlen(ext[i].extensionName);
		if (len + nameLen + 2 > sizeof(glConfig.extensions_string))
			break;
		memcpy(glConfig.extensions_string + len, ext[i].extensionName, nameLen);
		len += nameLen;
		glConfig.extensions_string[len++] = ' ';
		glConfig.extensions_string[len] = '\0';
	}
	free(ext);

	free(printExtensions("VK_DEVICE_EXTENSIONS", qtrue, &numExt));

	ri.Printf(PRINT_ALL, "VK_MAX_IMAGE_DIMENSION_2D: %u\n", props.limits.maxImageDimension2D);
	ri.Printf(PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n",
		glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	ri.Printf(PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer,
		glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1]);
	if (glConfig.displayFrequency)
		ri.Printf(PRINT_ALL, "%d\n", glConfig.displayFrequency);
	else
		ri.Printf(PRINT_ALL, "N/A\n");

	gpuMemUsageInfo_f();
}
