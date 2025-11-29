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
void MetalBackend_LoadWorld(const char* name);
void MetalBackend_SetWorldVisData(const byte* vis);
void MetalBackend_EndRegistration();
int MetalBackend_LerpTag(orientation_t* tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char* tagName);
void MetalBackend_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);

// Debug / instrumentation helpers
void Metal_LogRendererCall(const char* name);
