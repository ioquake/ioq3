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

// Forward declaration: implemented in tr_shader.cpp.
void MetalRemapShader(const char *shaderName, const char *newShaderName,
                      const char *timeOffset);

// ---------------------------------------------------------------------------
// Dependencies required by RE_RegisterFont (from renderercommon/tr_font.c).
// ---------------------------------------------------------------------------
extern "C" {

// Declared in renderercommon/tr_common.h; defined here for the Metal renderer.
cvar_t *r_saveFontData = nullptr;

// RE_RegisterFont flushes pending render commands before doing CPU-side font
// work.  Metal does not use a GL-style command queue, so this is a no-op.
void R_IssuePendingRenderCommands(void) {}

// RE_RegisterFont registers each glyph image as a "no-mip" shader so it is
// sampled without mipmapping.  Forward to the Metal shader registration path.
qhandle_t RE_RegisterShaderNoMip(const char *name) {
    return MetalBackend_RegisterShader(name, /*mipmap=*/false);
}

// Forward declaration for RE_RegisterFont implemented in tr_font.c.
void RE_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);

} // extern "C"

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

	// Initialize cvars that renderercommon/tr_font.c references via tr_common.h.
	r_saveFontData = rimp->Cvar_Get("r_saveFontData", "0", CVAR_ARCHIVE);

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

	g_refExport.MarkFragments = MetalBackend_MarkFragments;
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
	g_refExport.DrawRotatePic = MetalBackend_DrawRotatePic;
	g_refExport.DrawRotatePic2 = MetalBackend_DrawRotatePic2;
	g_refExport.DrawStretchRaw = MetalBackend_DrawStretchRaw;
	g_refExport.UploadCinematic = MetalBackend_UploadCinematic;

	g_refExport.RegisterFont = RE_RegisterFont;
	g_refExport.RemapShader = MetalRemapShader;
	g_refExport.GetEntityToken = MetalBackend_GetEntityToken;
	g_refExport.inPVS = MetalBackend_inPVS;

	g_refExport.TakeVideoFrame = [](int, int, byte*, byte*, qboolean) {
		Metal_LogRendererCall("re.TakeVideoFrame");
	};

	return &g_refExport;
}
}
