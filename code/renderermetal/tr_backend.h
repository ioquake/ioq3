#pragma once

extern "C" {
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
}

bool MetalBackend_Initialize(refimport_t imports);
void MetalBackend_Shutdown(qboolean destroyWindow);
void MetalBackend_BeginRegistration(glconfig_t* configOut);
void MetalBackend_BeginFrame(stereoFrame_t stereoFrame);
void MetalBackend_EndFrame(int* frontEndMsec, int* backEndMsec);
void MetalBackend_UploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);
void MetalBackend_DrawStretchRaw(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);
qhandle_t MetalBackend_RegisterShader(const char* name, bool mipmap);
qhandle_t MetalBackend_RegisterModel(const char* name);
qhandle_t MetalBackend_RegisterSkin(const char* name);
void MetalBackend_SetColor(const float* rgba);
void MetalBackend_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader);
void MetalBackend_DrawRotatePic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float degrees, qhandle_t shader);
void MetalBackend_DrawRotatePic2(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float degrees, qhandle_t shader);
void MetalBackend_LoadWorld(const char* name);
void MetalBackend_SetWorldVisData(const byte* vis);
void MetalBackend_EndRegistration();
qboolean MetalBackend_GetEntityToken(char* buffer, int size);
qboolean MetalBackend_inPVS(const vec3_t p1, const vec3_t p2);
int MetalBackend_LerpTag(orientation_t* tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char* tagName);
void MetalBackend_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);
int MetalBackend_MarkFragments(int numPoints, const vec3_t* points, const vec3_t projection,
                               int maxPoints, vec3_t pointBuffer,
                               int maxFragments, markFragment_t* fragmentBuffer);

// Debug / instrumentation helpers
void Metal_LogRendererCall(const char* name);

// -------------------------------------------------------------------------
// Flare system — implemented in tr_flares.cpp, called by tr_backend.cpp
// -------------------------------------------------------------------------
// Initialise cvars and clear the flare pool. Call once from initialize().
void Metal_InitFlares();
// Reset active/inactive flare lists (called from Metal_InitFlares).
void R_ClearFlares();
// Add a flare for every dynamic light in the current scene state.
void RB_AddDlightFlares();
// Test visibility (simplified) and render all active flares.
void RB_RenderFlares();

// Helpers called from tr_flares.cpp back into the Metal backend:
// Retrieve an additive-blended pipeline handle for the flare shader.
qhandle_t MetalBackend_GetFlareShader();
// Draw a screen-space quad with additive blending (One, One).
// x, y, w, h are in screen pixels (y = 0 at window top).
// r, g, b are pre-multiplied linear colour values in [0, 1].
void MetalBackend_DrawFlareQuad(float x, float y, float w, float h,
                                float r, float g, float b, qhandle_t shader);
// Expose current scene camera matrices and viewport bounds.
// viewMat[16] and projMat[16] are column-major floats.
// vpX/vpY/vpW/vpH are in screen pixels (y = 0 at window top).
void MetalBackend_GetSceneCameraForFlares(float viewMat[16], float projMat[16],
                                          float viewOrg[3],
                                          int* vpX, int* vpY, int* vpW, int* vpH);
