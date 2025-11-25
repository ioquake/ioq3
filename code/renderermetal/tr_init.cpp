#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#ifdef USE_INTERNAL_SDL_HEADERS
#	include "SDL.h"
#	include "SDL_metal.h"
#else
#	include <SDL.h>
#	include <SDL_metal.h>
#endif

#if !defined(__APPLE__)
#error "Metal renderer requires an Apple platform"
#endif

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <cstdio>
#include <cstring>

static refimport_t ri;

namespace
{
	SDL_Window *g_window = nullptr;
	SDL_MetalView g_view = nullptr;
	MTL::Device *g_device = nullptr;
	MTL::CommandQueue *g_commandQueue = nullptr;
	CA::MetalLayer *g_layer = nullptr;
	glconfig_t g_glConfig{};
	bool g_inputInitialized = false;

	void Metal_UpdateDrawableSize()
	{
		if (!g_window)
		{
			return;
		}

		int w = 0;
		int h = 0;
		SDL_Metal_GetDrawableSize(g_window, &w, &h);

		if (w > 0 && h > 0)
		{
			g_glConfig.vidWidth = w;
			g_glConfig.vidHeight = h;
			g_glConfig.windowAspect = static_cast<float>(w) / static_cast<float>(h);
		}
	}

	void Metal_FillDefaultConfig()
	{
		std::snprintf(g_glConfig.renderer_string, sizeof(g_glConfig.renderer_string), "Metal");
		std::snprintf(g_glConfig.vendor_string, sizeof(g_glConfig.vendor_string), "Apple");
		std::snprintf(g_glConfig.version_string, sizeof(g_glConfig.version_string), "Metal stub");
		g_glConfig.extensions_string[0] = '\0';

		g_glConfig.maxTextureSize = 0;
		g_glConfig.numTextureUnits = 0;

		g_glConfig.colorBits = 32;
		g_glConfig.depthBits = 24;
		g_glConfig.stencilBits = 8;

		g_glConfig.driverType = GLDRV_ICD;
		g_glConfig.hardwareType = GLHW_GENERIC;

		g_glConfig.deviceSupportsGamma = qfalse;
		g_glConfig.textureCompression = TC_NONE;
		g_glConfig.textureEnvAddAvailable = qfalse;

		g_glConfig.displayFrequency = 0;
		g_glConfig.isFullscreen = qfalse;
		g_glConfig.stereoEnabled = qfalse;
		g_glConfig.smpActive = qfalse;
	}

	void Metal_Destroy(qboolean destroyWindow)
	{
		if (g_inputInitialized)
		{
			ri.IN_Shutdown();
			g_inputInitialized = false;
		}

		if (g_commandQueue)
		{
			g_commandQueue->release();
			g_commandQueue = nullptr;
		}

		if (g_device)
		{
			g_device->release();
			g_device = nullptr;
		}

		if (g_view)
		{
			SDL_Metal_DestroyView(g_view);
			g_view = nullptr;
		}

		if (destroyWindow && g_window)
		{
			SDL_DestroyWindow(g_window);
			g_window = nullptr;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		}

		g_layer = nullptr;
	}

	qboolean Metal_CreateWindowIfNeeded()
	{
		if (g_window)
		{
			Metal_UpdateDrawableSize();
			return qtrue;
		}

		if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
		{
			if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
			{
				ri.Printf(PRINT_ALL, "SDL video init failed: %s\n", SDL_GetError());
				return qfalse;
			}
		}

		const int windowWidth = 1280;
		const int windowHeight = 720;

		g_window = SDL_CreateWindow("ioquake3 (Metal)",
		                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                            windowWidth, windowHeight,
		                            SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
		if (!g_window)
		{
			ri.Printf(PRINT_ALL, "SDL_CreateWindow failed: %s\n", SDL_GetError());
			return qfalse;
		}

		g_view = SDL_Metal_CreateView(g_window);
		if (!g_view)
		{
			ri.Printf(PRINT_ALL, "SDL_Metal_CreateView failed: %s\n", SDL_GetError());
			Metal_Destroy(qtrue);
			return qfalse;
		}

		g_device = MTL::CreateSystemDefaultDevice();
		if (!g_device)
		{
			ri.Printf(PRINT_ALL, "Metal device creation failed\n");
			Metal_Destroy(qtrue);
			return qfalse;
		}

		g_commandQueue = g_device->newCommandQueue();

		g_layer = reinterpret_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(g_view));
		if (g_layer)
		{
			g_layer->setDevice(g_device);
			g_layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
			g_layer->setFramebufferOnly(false);
		}

		Metal_FillDefaultConfig();

		g_glConfig.vidWidth = windowWidth;
		g_glConfig.vidHeight = windowHeight;
		g_glConfig.windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
		Metal_UpdateDrawableSize();

		ri.IN_Init(g_window);
		g_inputInitialized = true;

		return qtrue;
	}

	void Metal_BeginFrame(stereoFrame_t)
	{
		Metal_UpdateDrawableSize();
	}

	void Metal_EndFrame(int *frontEndMsec, int *backEndMsec)
	{
		if (frontEndMsec)
		{
			*frontEndMsec = 0;
		}

		if (backEndMsec)
		{
			*backEndMsec = 0;
		}
	}

	void Metal_ClearScene()
	{
	}

	void Metal_AddRefEntityToScene(const refEntity_t *)
	{
	}

	void Metal_AddPolyToScene(qhandle_t, int, const polyVert_t *, int)
	{
	}

	int Metal_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir)
	{
		(void)point;
		if (ambientLight)
		{
			VectorClear(ambientLight);
		}

		if (directedLight)
		{
			VectorClear(directedLight);
		}

		if (lightDir)
		{
			VectorClear(lightDir);
		}

		return 0;
	}

	void Metal_AddLightToScene(const vec3_t, float, float, float, float)
	{
	}

	void Metal_AddAdditiveLightToScene(const vec3_t, float, float, float, float)
	{
	}

	void Metal_RenderScene(const refdef_t *)
	{
	}

	void Metal_SetColor(const float *)
	{
	}

	void Metal_DrawStretchPic(float, float, float, float, float, float, float, float, qhandle_t)
	{
	}

	void Metal_DrawStretchRaw(int, int, int, int, int, int, const byte *, int, qboolean)
	{
	}

	void Metal_UploadCinematic(int, int, int, int, const byte *, int, qboolean)
	{
	}

	void Metal_BeginRegistration(glconfig_t *config)
	{
		if (!Metal_CreateWindowIfNeeded())
		{
			ri.Error(ERR_FATAL, "Metal renderer failed to create SDL window");
			return;
		}

		if (config)
		{
			*config = g_glConfig;
		}
	}

	qhandle_t Metal_RegisterModel(const char *)
	{
		return 0;
	}

	qhandle_t Metal_RegisterSkin(const char *)
	{
		return 0;
	}

	qhandle_t Metal_RegisterShader(const char *)
	{
		return 0;
	}

	qhandle_t Metal_RegisterShaderNoMip(const char *)
	{
		return 0;
	}

	void Metal_LoadWorld(const char *)
	{
	}

	void Metal_SetWorldVisData(const byte *)
	{
	}

	void Metal_EndRegistration()
	{
	}

	int Metal_MarkFragments(int, const vec3_t *, const vec3_t, int, vec3_t, int, markFragment_t *)
	{
		return 0;
	}

	int Metal_LerpTag(orientation_t *tag, qhandle_t, int, int, float, const char *)
	{
		if (tag)
		{
			Com_Memset(tag, 0, sizeof(*tag));
		}
		return 0;
	}

	void Metal_ModelBounds(qhandle_t, vec3_t mins, vec3_t maxs)
	{
		if (mins)
		{
			VectorClear(mins);
		}

		if (maxs)
		{
			VectorClear(maxs);
		}
	}

	void Metal_RegisterFont(const char *, int, fontInfo_t *font)
	{
		if (font)
		{
			std::memset(font, 0, sizeof(*font));
		}
	}

	void Metal_RemapShader(const char *, const char *, const char *)
	{
	}

	qboolean Metal_GetEntityToken(char *buffer, int size)
	{
		if (buffer && size > 0)
		{
			buffer[0] = '\0';
		}
		return qfalse;
	}

	qboolean Metal_inPVS(const vec3_t, const vec3_t)
	{
		return qfalse;
	}

	void Metal_TakeVideoFrame(int, int, byte *, byte *, qboolean)
	{
	}

	void Metal_Shutdown(qboolean destroyWindow)
	{
		Metal_Destroy(destroyWindow);
	}
}

extern "C" {

#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t *QDECL GetRefAPI(int apiVersion, refimport_t *rimp)
#else
refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp)
#endif
{
	static refexport_t re;

	if (apiVersion != REF_API_VERSION)
	{
		return nullptr;
	}

	ri = *rimp;

	std::memset(&re, 0, sizeof(re));

	re.Shutdown = Metal_Shutdown;

	re.BeginRegistration = Metal_BeginRegistration;
	re.RegisterModel = Metal_RegisterModel;
	re.RegisterSkin = Metal_RegisterSkin;
	re.RegisterShader = Metal_RegisterShader;
	re.RegisterShaderNoMip = Metal_RegisterShaderNoMip;
	re.LoadWorld = Metal_LoadWorld;
	re.SetWorldVisData = Metal_SetWorldVisData;
	re.EndRegistration = Metal_EndRegistration;

	re.BeginFrame = Metal_BeginFrame;
	re.EndFrame = Metal_EndFrame;

	re.MarkFragments = Metal_MarkFragments;
	re.LerpTag = Metal_LerpTag;
	re.ModelBounds = Metal_ModelBounds;

	re.ClearScene = Metal_ClearScene;
	re.AddRefEntityToScene = Metal_AddRefEntityToScene;
	re.AddPolyToScene = Metal_AddPolyToScene;
	re.LightForPoint = Metal_LightForPoint;
	re.AddLightToScene = Metal_AddLightToScene;
	re.AddAdditiveLightToScene = Metal_AddAdditiveLightToScene;
	re.RenderScene = Metal_RenderScene;

	re.SetColor = Metal_SetColor;
	re.DrawStretchPic = Metal_DrawStretchPic;
	re.DrawStretchRaw = Metal_DrawStretchRaw;
	re.UploadCinematic = Metal_UploadCinematic;

	re.RegisterFont = Metal_RegisterFont;
	re.RemapShader = Metal_RemapShader;
	re.GetEntityToken = Metal_GetEntityToken;
	re.inPVS = Metal_inPVS;

	re.TakeVideoFrame = Metal_TakeVideoFrame;

	return &re;
}

}
