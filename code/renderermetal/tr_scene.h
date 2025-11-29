#pragma once

#include <array>

extern "C" {
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
}

// Conservative limits aligned with the GL2 renderer defaults.
constexpr int METAL_MAX_SCENE_POLYS = 600;
constexpr int METAL_MAX_SCENE_POLYVERTS = 3000;

struct MetalScenePolyRange {
    qhandle_t shader = 0;
    int numVerts = 0;
    int firstVertex = 0;
};

struct MetalSceneLight {
    vec3_t origin{};
    vec3_t color{};
    float intensity = 0.0f;
    qboolean additive = qfalse;
};

struct MetalSceneState {
    refdef_t refdef{};
    qboolean refdefValid = qfalse;
    int frameId = 0;
    int sceneCount = 0;

    int numEntities = 0;
    int numPolys = 0;
    int numPolyVerts = 0;
    int numLights = 0;

    // Scene boundary markers (like OpenGL2's r_firstSceneEntity, etc.)
    // These mark where the current scene starts within the accumulated entities
    int firstSceneEntity = 0;
    int firstScenePoly = 0;
    int firstScenePolyVert = 0;
    int firstSceneLight = 0;

    // Saved range for the world scene (when RDF_NOWORLDMODEL is NOT set)
    // processEntities will use these to know which entities to render
    int worldSceneFirstEntity = 0;
    int worldSceneNumEntities = 0;

    std::array<refEntity_t, MAX_REFENTITIES> entities{};
    std::array<MetalScenePolyRange, METAL_MAX_SCENE_POLYS> polys{};
    std::array<polyVert_t, METAL_MAX_SCENE_POLYVERTS> polyVerts{};
    std::array<MetalSceneLight, MAX_DLIGHTS> lights{};
};

// Frame management -----------------------------------------------------------
void MetalScene_BeginFrame();
void MetalScene_EndFrame();
MetalSceneState& MetalScene_MutableState();
const MetalSceneState& MetalScene_GetState();

// Exported render-system entry points ---------------------------------------
extern "C" {
void RE_ClearScene(void);
void RE_AddRefEntityToScene(const refEntity_t* re);
void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys);
void RE_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void RE_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void RE_RenderScene(const refdef_t* fd);
}
