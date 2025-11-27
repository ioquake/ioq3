#include "tr_extensions.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "tr_utils.h"

extern "C" {
glRefConfig_t glRefConfig = {};
}

namespace
{
constexpr int kDefaultTextureUnits = 16;
constexpr int kDefaultTextureSize = 8192;
constexpr int kMaxTextureSize = 16384;

bool SupportsHighTierFeatures(MTL::Device* device)
{
    if (!device)
    {
        return false;
    }

    return device->supportsFamily(MTL::GPUFamilyMac2) ||
           device->supportsFamily(MTL::GPUFamilyApple7) ||
           device->supportsFamily(MTL::GPUFamilyMacCatalyst2);
}

bool IsIntelDevice(MTL::Device* device)
{
    if (!device || !device->name())
    {
        return false;
    }

    const char* name = device->name()->utf8String();
    return name && std::strstr(name, "Intel");
}

int DetermineMaxTextureSize(MTL::Device* device)
{
    if (!device)
    {
        return kDefaultTextureSize;
    }

    if (SupportsHighTierFeatures(device))
    {
        return kMaxTextureSize;
    }

    return kDefaultTextureSize;
}

int DetermineTextureUnits(MTL::Device* device)
{
    (void)device;
    return kDefaultTextureUnits;
}

void PopulateCapabilityFlags(MTL::Device* device)
{
    glRefConfig = {};
    glRefConfig.memInfo = MI_NONE;
    glRefConfig.textureCompression = TCR_NONE;

    const qboolean highTier = SupportsHighTierFeatures(device) ? qtrue : qfalse;

    glRefConfig.intelGraphics = IsIntelDevice(device) ? qtrue : qfalse;
    glRefConfig.occlusionQuery = qfalse;
    glRefConfig.occlusionQueryTarget = 0;

    glRefConfig.glslMajorVersion = highTier ? 3 : 2;
    glRefConfig.glslMinorVersion = highTier ? 2 : 1;
    glRefConfig.glslMaxAnimatedBones = highTier ? 256 : 128;

    glRefConfig.framebufferObject = qtrue;
    glRefConfig.framebufferMultisample = qtrue;
    glRefConfig.framebufferBlit = qtrue;
    glRefConfig.maxRenderbufferSize = DetermineMaxTextureSize(device);
    glRefConfig.maxColorAttachments = 8;

    glRefConfig.textureFloat = qtrue;
    glRefConfig.swizzleNormalmap = qfalse;

    glRefConfig.depthClamp = qtrue;
    glRefConfig.seamlessCubeMap = qtrue;
    glRefConfig.vertexArrayObject = qtrue;
    glRefConfig.directStateAccess = qtrue;
    glRefConfig.maxVertexAttribs = highTier ? 31 : 16;
    glRefConfig.gpuVertexAnimation = qtrue;
    glRefConfig.vaoCacheGlIndexType = sizeof(uint32_t);
    glRefConfig.vaoCacheGlIndexSize = sizeof(uint32_t);

    glRefConfig.readDepth = qtrue;
    glRefConfig.readStencil = qtrue;
    glRefConfig.shadowSamplers = qtrue;
    glRefConfig.standardDerivatives = qtrue;
}

void PrintDeviceBanner(MTL::Device* device, refimport_t* ri)
{
    if (!ri || !ri->Printf)
    {
        return;
    }

    const char* deviceName = device && device->name() ? device->name()->utf8String() : "Unknown";
    ri->Printf(PRINT_ALL, "----- Metal Device Info -----\n");
    ri->Printf(PRINT_ALL, " Renderer: %s\n", deviceName);
    ri->Printf(PRINT_ALL, " GPU Families: Mac2=%d Apple7=%d MacCatalyst2=%d\n",
               device && device->supportsFamily(MTL::GPUFamilyMac2),
               device && device->supportsFamily(MTL::GPUFamilyApple7),
               device && device->supportsFamily(MTL::GPUFamilyMacCatalyst2));
    ri->Printf(PRINT_ALL, " Max texture size: %d\n", DetermineMaxTextureSize(device));
    ri->Printf(PRINT_ALL, " Texture units: %d\n", DetermineTextureUnits(device));
    ri->Printf(PRINT_ALL, "-----------------------------\n");
}

void PrintCapabilitySummary(refimport_t* ri)
{
    if (!ri || !ri->Printf)
    {
        return;
    }

    ri->Printf(PRINT_ALL, "----- Metal Capability Flags -----\n");
    ri->Printf(PRINT_ALL, " intelGraphics: %d  bones: %d\n",
               glRefConfig.intelGraphics, glRefConfig.glslMaxAnimatedBones);
    ri->Printf(PRINT_ALL, " framebuffer: FBO=%d MSAA=%d Blit=%d attachments=%d\n",
               glRefConfig.framebufferObject,
               glRefConfig.framebufferMultisample,
               glRefConfig.framebufferBlit,
               glRefConfig.maxColorAttachments);
    ri->Printf(PRINT_ALL, " vertexArrayObject=%d directStateAccess=%d attribs=%d\n",
               glRefConfig.vertexArrayObject,
               glRefConfig.directStateAccess,
               glRefConfig.maxVertexAttribs);
    ri->Printf(PRINT_ALL, " depthClamp=%d seamlessCubeMap=%d readDepth=%d readStencil=%d\n",
               glRefConfig.depthClamp,
               glRefConfig.seamlessCubeMap,
               glRefConfig.readDepth,
               glRefConfig.readStencil);
    ri->Printf(PRINT_ALL, " shadowSamplers=%d standardDerivatives=%d\n",
               glRefConfig.shadowSamplers,
               glRefConfig.standardDerivatives);
    ri->Printf(PRINT_ALL, "----------------------------------\n");
}
}

void Metal_InitExtensions(MTL::Device* device, glconfig_t* config, refimport_t* ri)
{
    if (!config)
    {
        return;
    }

    PrintDeviceBanner(device, ri);
    PopulateCapabilityFlags(device);
    PrintCapabilitySummary(ri);

    const char* deviceName = device && device->name() ? device->name()->utf8String() : "Metal";
    std::snprintf(config->vendor_string, sizeof(config->vendor_string), "Apple");
    std::snprintf(config->renderer_string, sizeof(config->renderer_string), "%s", deviceName);
    std::snprintf(config->version_string, sizeof(config->version_string), "Metal API");
    config->extensions_string[0] = '\0';

    config->maxTextureSize = DetermineMaxTextureSize(device);
    config->numTextureUnits = DetermineTextureUnits(device);

    config->textureCompression = TC_NONE;
    config->textureEnvAddAvailable = qtrue;
    config->deviceSupportsGamma = qfalse;
    config->driverType = GLDRV_ICD;
    config->hardwareType = GLHW_GENERIC;
    config->stereoEnabled = qfalse;
    config->smpActive = qfalse;
}
