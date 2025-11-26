#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#ifdef USE_INTERNAL_SDL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

// SDL Metal module functions
extern "C" {
	qboolean SDLMetal_Init(int width, int height, qboolean fullscreen);
	void SDLMetal_Shutdown(qboolean destroyWindow);
	SDL_Window *SDLMetal_GetWindow(void);
	void *SDLMetal_GetView(void);
	void *SDLMetal_GetLayer(void);
	void SDLMetal_GetDrawableSize(int *w, int *h);
}

#if !defined(__APPLE__)
#error "Metal renderer requires an Apple platform"
#endif

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <algorithm>
#include <array>
#include <cstring>

struct CinematicSlot
{
	MTL::Texture *texture = nullptr;
	int cols = 0;
	int rows = 0;
};

static refimport_t ri;
static MTL::Device *g_device = nullptr;
static MTL::CommandQueue *g_commandQueue = nullptr;
static CA::MetalLayer *g_layer = nullptr;
static MTL::Texture *g_frameTexture = nullptr;
static int g_frameCols = 0;
static int g_frameRows = 0;
static MTL::RenderPipelineState *g_pipelineState = nullptr;
static MTL::SamplerState *g_samplerState = nullptr;
static glconfig_t g_glConfig{};
static bool g_inputInitialized = false;
static std::array<CinematicSlot, 8> g_slots{};

static int g_pendingClient = 0;
static int g_pendingCols = 0;
static int g_pendingRows = 0;
static int g_pendingX = 0;
static int g_pendingY = 0;
static int g_pendingW = 0;
static int g_pendingH = 0;
static bool g_pendingDirty = false;
static std::vector<uint8_t> g_pendingData;

static void Metal_Destroy(qboolean destroyWindow)
{
	if (g_samplerState)
	{
		g_samplerState->release();
		g_samplerState = nullptr;
	}
	if (g_pipelineState)
	{
		g_pipelineState->release();
		g_pipelineState = nullptr;
	}
	for (auto &slot : g_slots)
	{
		if (slot.texture)
		{
			slot.texture->release();
			slot.texture = nullptr;
		}
		slot.cols = slot.rows = 0;
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
	
	// Shutdown input system before SDL cleanup (matches OpenGL behavior)
	if (g_inputInitialized)
	{
		ri.IN_Shutdown();
		g_inputInitialized = false;
	}
	
	SDLMetal_Shutdown(destroyWindow);
	g_layer = nullptr;
}

static void Metal_FillConfigDefaults(int width, int height, qboolean fullscreen)
{
	std::snprintf(g_glConfig.renderer_string, sizeof(g_glConfig.renderer_string), "Metal");
	std::snprintf(g_glConfig.vendor_string, sizeof(g_glConfig.vendor_string), "Apple");
	std::snprintf(g_glConfig.version_string, sizeof(g_glConfig.version_string), "Metal");
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

	g_glConfig.vidWidth = width;
	g_glConfig.vidHeight = height;
	g_glConfig.windowAspect = static_cast<float>(width) / static_cast<float>(height);
	g_glConfig.displayFrequency = 0;
	g_glConfig.isFullscreen = fullscreen;
	g_glConfig.stereoEnabled = qfalse;
	g_glConfig.smpActive = qfalse;
}

static bool Metal_InitWindow()
{
	int width = 1280;
	int height = 720;
	qboolean fullscreen = qfalse;

	// Get window size preferences from cvars
	int cw = ri.Cvar_VariableIntegerValue("r_customwidth");
	int ch = ri.Cvar_VariableIntegerValue("r_customheight");
	fullscreen = ri.Cvar_VariableIntegerValue("r_fullscreen") ? qtrue : qfalse;
	if (cw > 0 && ch > 0)
	{
		width = cw;
		height = ch;
	}
	else
	{
		SDL_DisplayMode dm;
		if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
		{
			width = dm.w;
			height = dm.h;
		}
	}

	if (!SDLMetal_Init(width, height, fullscreen))
	{
		return false;
	}

	g_device = MTL::CreateSystemDefaultDevice();
	if (!g_device)
	{
		ri.Printf(PRINT_ALL, "Metal device creation failed\n");
		SDLMetal_Shutdown(qtrue);
		return false;
	}

	g_commandQueue = g_device->newCommandQueue();
	g_layer = reinterpret_cast<CA::MetalLayer *>(SDLMetal_GetLayer());
	if (g_layer)
	{
		g_layer->setDevice(g_device);
		g_layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
		g_layer->setFramebufferOnly(false);
		g_layer->setDisplaySyncEnabled(true);
		g_layer->setMaximumDrawableCount(3);
		CGSize drawableSize;
		drawableSize.width = static_cast<CGFloat>(width);
		drawableSize.height = static_cast<CGFloat>(height);
		g_layer->setDrawableSize(drawableSize);
	}

	Metal_FillConfigDefaults(width, height, fullscreen);
	return true;
}

static void Metal_EnsurePipeline(MTL::Texture *drawableTexture)
{
	if (g_pipelineState && g_samplerState)
	{
		return;
	}
	if (!g_device || !drawableTexture)
	{
		return;
	}

	static const char *shaderSrc =
		R"(
		using namespace metal;
		struct VertexIn { float2 pos [[attribute(0)]]; float2 uv [[attribute(1)]]; };
		struct VertexOut { float4 position [[position]]; float2 uv; };
		vertex VertexOut vmain(uint vid [[vertex_id]], const device VertexIn* verts [[buffer(0)]])
		{
			VertexOut out;
			VertexIn v = verts[vid];
			out.position = float4(v.pos, 0.0, 1.0);
			out.uv = v.uv;
			return out;
		}
		fragment float4 fmain(VertexOut in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler samp [[sampler(0)]])
		{
			return tex.sample(samp, in.uv);
		}
		)";

	NS::Error *error = nullptr;
	NS::String *src = NS::String::string(shaderSrc, NS::ASCIIStringEncoding);
	MTL::Library *lib = g_device->newLibrary(src, nullptr, &error);
	src->release();
	if (!lib)
	{
		if (error)
		{
			ri.Printf(PRINT_WARNING, "Metal: shader compile failed: %s\n", error->localizedDescription()->utf8String());
			error->release();
		}
		return;
	}

	MTL::Function *vfn = lib->newFunction(NS::String::string("vmain", NS::ASCIIStringEncoding));
	MTL::Function *ffn = lib->newFunction(NS::String::string("fmain", NS::ASCIIStringEncoding));

	MTL::RenderPipelineDescriptor *pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexFunction(vfn);
	pd->setFragmentFunction(ffn);
	pd->colorAttachments()->object(0)->setPixelFormat(drawableTexture->pixelFormat());

	g_pipelineState = g_device->newRenderPipelineState(pd, &error);

	pd->release();
	vfn->release();
	ffn->release();
	lib->release();

	if (!g_pipelineState)
	{
		if (error)
		{
			ri.Printf(PRINT_WARNING, "Metal: pipeline creation failed: %s\n", error->localizedDescription()->utf8String());
			error->release();
		}
		return;
	}

	MTL::SamplerDescriptor *sd = MTL::SamplerDescriptor::alloc()->init();
	sd->setMinFilter(MTL::SamplerMinMagFilterLinear);
	sd->setMagFilter(MTL::SamplerMinMagFilterLinear);
	sd->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
	sd->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
	g_samplerState = g_device->newSamplerState(sd);
	sd->release();
}

static void Metal_BeginFrame(stereoFrame_t)
{
	if (!Metal_InitWindow())
	{
		ri.Error(ERR_FATAL, "Metal renderer failed to create window");
		return;
	}
	int w = 0, h = 0;
	SDLMetal_GetDrawableSize(&w, &h);
	if (w > 0 && h > 0)
	{
		g_glConfig.vidWidth = w;
		g_glConfig.vidHeight = h;
		g_glConfig.windowAspect = static_cast<float>(w) / static_cast<float>(h);
		if (g_layer)
		{
			CGSize drawableSize;
			drawableSize.width = static_cast<CGFloat>(w);
			drawableSize.height = static_cast<CGFloat>(h);
			g_layer->setDrawableSize(drawableSize);
		}
	}
}

static void Metal_EndFrame(int *frontEndMsec, int *backEndMsec)
{
	if (frontEndMsec)
	{
		*frontEndMsec = 0;
	}
	if (backEndMsec)
	{
		*backEndMsec = 0;
	}
	if (!g_layer || !g_commandQueue)
	{
		return;
	}

	CA::MetalDrawable *drawable = g_layer->nextDrawable();
	if (!drawable)
	{
		return;
	}

	Metal_EnsurePipeline(drawable->texture());
	if (!g_pipelineState || !g_samplerState)
	{
		drawable->release();
		return;
	}

	if (g_pendingDirty && g_device)
	{
		int clientIndex = std::clamp(g_pendingClient, 0, static_cast<int>(g_slots.size()) - 1);
		CinematicSlot &slot = g_slots[clientIndex];

		if (!slot.texture || slot.cols != g_pendingCols || slot.rows != g_pendingRows)
		{
			if (slot.texture)
			{
				slot.texture->release();
				slot.texture = nullptr;
			}
			MTL::TextureDescriptor *desc = MTL::TextureDescriptor::texture2DDescriptor(
			    MTL::PixelFormatRGBA8Unorm, g_pendingCols, g_pendingRows, false);
			desc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead);
			desc->setStorageMode(MTL::StorageModeShared);
			slot.texture = g_device->newTexture(desc);
			slot.cols = g_pendingCols;
			slot.rows = g_pendingRows;
			desc->release();
		}

		if (slot.texture)
		{
			MTL::Region region = MTL::Region::Make2D(0, 0, g_pendingCols, g_pendingRows);
			slot.texture->replaceRegion(region, 0, g_pendingData.data(), g_pendingCols * 4);
		}

		g_pendingDirty = false;
		g_frameTexture = slot.texture;
		g_frameCols = slot.cols;
		g_frameRows = slot.rows;
	}

	if (!g_frameTexture)
	{
		drawable->release();
		return;
	}

	struct Vertex
	{
		float pos[2];
		float uv[2];
	};

	const float fbWidth = static_cast<float>(g_glConfig.vidWidth);
	const float fbHeight = static_cast<float>(g_glConfig.vidHeight);

	const float ndcLeft = (g_pendingX * 2.0f / fbWidth) - 1.0f;
	const float ndcRight = ((g_pendingX + g_pendingW) * 2.0f / fbWidth) - 1.0f;
	const float ndcTop = 1.0f - (g_pendingY * 2.0f / fbHeight);
	const float ndcBottom = 1.0f - ((g_pendingY + g_pendingH) * 2.0f / fbHeight);

	const float u0 = 0.5f / static_cast<float>(g_frameCols);
	const float v0 = 0.5f / static_cast<float>(g_frameRows);
	const float u1 = (static_cast<float>(g_frameCols) - 0.5f) / static_cast<float>(g_frameCols);
	const float v1 = (static_cast<float>(g_frameRows) - 0.5f) / static_cast<float>(g_frameRows);

	Vertex verts[6] = {
	    {{ndcLeft, ndcTop}, {u0, v0}},
	    {{ndcRight, ndcTop}, {u1, v0}},
	    {{ndcRight, ndcBottom}, {u1, v1}},
	    {{ndcLeft, ndcTop}, {u0, v0}},
	    {{ndcRight, ndcBottom}, {u1, v1}},
	    {{ndcLeft, ndcBottom}, {u0, v1}},
	};

	MTL::RenderPassDescriptor *rp = MTL::RenderPassDescriptor::renderPassDescriptor();
	rp->colorAttachments()->object(0)->setTexture(drawable->texture());
	rp->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
	rp->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

	MTL::CommandBuffer *cb = g_commandQueue->commandBuffer();
	MTL::RenderCommandEncoder *enc = cb->renderCommandEncoder(rp);
	enc->setRenderPipelineState(g_pipelineState);
	MTL::Viewport vp;
	vp.originX = 0.0;
	vp.originY = 0.0;
	vp.width = static_cast<double>(drawable->texture()->width());
	vp.height = static_cast<double>(drawable->texture()->height());
	vp.znear = 0.0;
	vp.zfar = 1.0;
	enc->setViewport(vp);
	enc->setVertexBytes(verts, sizeof(verts), 0);
	enc->setFragmentTexture(g_frameTexture, 0);
	enc->setFragmentSamplerState(g_samplerState, 0);
	enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
	enc->endEncoding();

	cb->presentDrawable(drawable);
	cb->commit();

	enc->release();
	rp->release();
	drawable->release();
}

static void Metal_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	(void)w;
	(void)h;

	if (!data || cols <= 0 || rows <= 0)
	{
		return;
	}
	auto isPowerOfTwo = [](int v) { return v > 0 && (v & (v - 1)) == 0; };
	if (!isPowerOfTwo(cols) || !isPowerOfTwo(rows))
	{
		ri.Printf(PRINT_WARNING, "Metal: UploadCinematic got non-power-of-two %dx%d\n", cols, rows);
		return;
	}

	const size_t expected = static_cast<size_t>(cols) * static_cast<size_t>(rows) * 4;
	g_pendingData.assign(data, data + expected);
	g_pendingCols = cols;
	g_pendingRows = rows;
	g_pendingClient = std::clamp(client, 0, static_cast<int>(g_slots.size()) - 1);
	g_pendingDirty = dirty ? true : false;
}

static void Metal_DrawStretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	if (!data || cols <= 0 || rows <= 0 || w <= 0 || h <= 0)
	{
		return;
	}
	Metal_UploadCinematic(w, h, cols, rows, data, client, dirty);

	g_pendingX = x;
	g_pendingY = y;
	g_pendingW = w;
	g_pendingH = h;
}

static void Metal_BeginRegistration(glconfig_t *config)
{
	if (!Metal_InitWindow())
	{
		ri.Error(ERR_FATAL, "Metal renderer failed to create SDL window");
		return;
	}
	if (config)
	{
		*config = g_glConfig;
	}
	if (!g_inputInitialized)
	{
		ri.IN_Init(SDLMetal_GetWindow());
		g_inputInitialized = true;
	}
}

static void Metal_Shutdown(qboolean destroyWindow)
{
	Metal_Destroy(destroyWindow);
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
	re.RegisterModel = [](const char *) { return 0; };
	re.RegisterSkin = [](const char *) { return 0; };
	re.RegisterShader = [](const char *) { return 0; };
	re.RegisterShaderNoMip = [](const char *) { return 0; };
	re.LoadWorld = [](const char *) {};
	re.SetWorldVisData = [](const byte *) {};
	re.EndRegistration = []() {};

	re.BeginFrame = Metal_BeginFrame;
	re.EndFrame = Metal_EndFrame;

	re.MarkFragments = [](int, const vec3_t *, const vec3_t, int, vec3_t, int, markFragment_t *) { return 0; };
	re.LerpTag = [](orientation_t *tag, qhandle_t, int, int, float, const char *) {
		if (tag)
		{
			Com_Memset(tag, 0, sizeof(*tag));
		}
		return 0;
	};
	re.ModelBounds = [](qhandle_t, vec3_t mins, vec3_t maxs) {
		if (mins)
		{
			VectorClear(mins);
		}
		if (maxs)
		{
			VectorClear(maxs);
		}
	};

	re.ClearScene = []() {};
	re.AddRefEntityToScene = [](const refEntity_t *) {};
	re.AddPolyToScene = [](qhandle_t, int, const polyVert_t *, int) {};
	re.LightForPoint = [](vec3_t, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
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
	};
	re.AddLightToScene = [](const vec3_t, float, float, float, float) {};
	re.AddAdditiveLightToScene = [](const vec3_t, float, float, float, float) {};
	re.RenderScene = [](const refdef_t *) {};

	re.SetColor = [](const float *) {};
	re.DrawStretchPic = [](float, float, float, float, float, float, float, float, qhandle_t) {};
	re.DrawStretchRaw = Metal_DrawStretchRaw;
	re.UploadCinematic = Metal_UploadCinematic;

	re.RegisterFont = [](const char *, int, fontInfo_t *font) {
		if (font)
		{
			std::memset(font, 0, sizeof(*font));
		}
	};
	re.RemapShader = [](const char *, const char *, const char *) {};
	re.GetEntityToken = [](char *buffer, int size) {
		if (buffer && size > 0)
		{
			buffer[0] = '\0';
		}
		return qfalse;
	};
	re.inPVS = [](const vec3_t, const vec3_t) { return qfalse; };

	re.TakeVideoFrame = [](int, int, byte *, byte *, qboolean) {};

	return &re;
}

}
