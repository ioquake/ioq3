#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "../qcommon/q_shared.h"
#ifdef __cplusplus
}
#endif

typedef enum {
    MI_NONE,
    MI_NVX,
    MI_ATI
} memInfo_t;

typedef enum {
    TCR_NONE = 0x0000,
    TCR_RGTC = 0x0001,
    TCR_BPTC = 0x0002,
} textureCompressionRef_t;

typedef struct {
    qboolean intelGraphics;

    qboolean occlusionQuery;
    int occlusionQueryTarget;

    int glslMajorVersion;
    int glslMinorVersion;
    int glslMaxAnimatedBones;

    memInfo_t memInfo;

    qboolean framebufferObject;
    int maxRenderbufferSize;
    int maxColorAttachments;

    qboolean textureFloat;
    textureCompressionRef_t textureCompression;
    qboolean swizzleNormalmap;

    qboolean framebufferMultisample;
    qboolean framebufferBlit;

    qboolean depthClamp;
    qboolean seamlessCubeMap;

    qboolean vertexArrayObject;
    qboolean directStateAccess;

    int maxVertexAttribs;
    qboolean gpuVertexAnimation;

    int vaoCacheGlIndexType;
    size_t vaoCacheGlIndexSize;

    qboolean readDepth;
    qboolean readStencil;
    qboolean shadowSamplers;
    qboolean standardDerivatives;
} glRefConfig_t;

#ifdef __cplusplus
extern "C" {
#endif
extern glRefConfig_t glRefConfig;
#ifdef __cplusplus
}
#endif
