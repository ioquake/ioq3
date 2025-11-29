/*
===========================================================================
Metal renderer entry points. This file mirrors the GL back-end naming so
higher level engine code can treat renderers consistently.
===========================================================================
*/

#include "tr_backend.h"
#include "tr_scene.h"
#include "tr_local.h"

extern "C" {
#include "../qcommon/qcommon.h"
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
}

#include <cstring>

namespace {
refexport_t g_refExport;
}

extern "C" {
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t* QDECL GetRefAPI(int apiVersion, refimport_t* rimp)
#else
refexport_t* GetRefAPI(int apiVersion, refimport_t* rimp)
#endif
{
	if (apiVersion != REF_API_VERSION || !rimp) {
		return nullptr;
	}

	if (!MetalBackend_Initialize(*rimp)) {
		return nullptr;
	}

	std::memset(&g_refExport, 0, sizeof(g_refExport));

	g_refExport.Shutdown = MetalBackend_Shutdown;
	g_refExport.BeginRegistration = MetalBackend_BeginRegistration;
	g_refExport.RegisterModel = MetalBackend_RegisterModel;
	g_refExport.RegisterSkin = MetalBackend_RegisterSkin;
	g_refExport.RegisterShader = [](const char* name) {
		return MetalBackend_RegisterShader(name, true);
	};
	g_refExport.RegisterShaderNoMip = [](const char* name) {
		return MetalBackend_RegisterShader(name, false);
	};
	g_refExport.LoadWorld = MetalBackend_LoadWorld;
	g_refExport.SetWorldVisData = MetalBackend_SetWorldVisData;
	g_refExport.EndRegistration = MetalBackend_EndRegistration;

	g_refExport.BeginFrame = MetalBackend_BeginFrame;
	g_refExport.EndFrame = MetalBackend_EndFrame;

	g_refExport.MarkFragments = [](int, const vec3_t*, const vec3_t, int, vec3_t, int, markFragment_t*) {
		Metal_LogRendererCall("re.MarkFragments");
		return 0;
	};
	g_refExport.LerpTag = MetalBackend_LerpTag;
	g_refExport.ModelBounds = MetalBackend_ModelBounds;

	g_refExport.ClearScene = RE_ClearScene;
	g_refExport.AddRefEntityToScene = RE_AddRefEntityToScene;
	g_refExport.AddPolyToScene = RE_AddPolyToScene;
	g_refExport.LightForPoint = R_LightForPoint;
	g_refExport.AddLightToScene = RE_AddLightToScene;
	g_refExport.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	g_refExport.RenderScene = RE_RenderScene;

	g_refExport.SetColor = MetalBackend_SetColor;
	g_refExport.DrawStretchPic = MetalBackend_DrawStretchPic;
	g_refExport.DrawStretchRaw = MetalBackend_DrawStretchRaw;
	g_refExport.UploadCinematic = MetalBackend_UploadCinematic;

	g_refExport.RegisterFont = [](const char*, int, fontInfo_t* font) {
		Metal_LogRendererCall("re.RegisterFont");
		if (font) {
			std::memset(font, 0, sizeof(*font));
		}
	};
	g_refExport.RemapShader = [](const char*, const char*, const char*) {
		Metal_LogRendererCall("re.RemapShader");
	};
	g_refExport.GetEntityToken = [](char* buffer, int size) {
		Metal_LogRendererCall("re.GetEntityToken");
		if (buffer && size > 0) {
			buffer[0] = '\0';
		}
		return qfalse;
	};
	g_refExport.inPVS = [](const vec3_t, const vec3_t) {
		Metal_LogRendererCall("re.inPVS");
		return qfalse;
	};

	g_refExport.TakeVideoFrame = [](int, int, byte*, byte*, qboolean) {
		Metal_LogRendererCall("re.TakeVideoFrame");
	};

	return &g_refExport;
}
}
