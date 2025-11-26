/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2025 Modern Metal Renderer Implementation

Metal renderer with RAII and modern C++ practices
===========================================================================
*/

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
#include <memory>
#include <vector>

#include "tr_metal_utils.h"

//=============================================================================
// Metal Renderer Class
//=============================================================================

class MetalRenderer {
public:
	MetalRenderer() = default;
	~MetalRenderer() { shutdown(qtrue); }

	// Delete copy/move (singleton pattern)
	MetalRenderer(const MetalRenderer&) = delete;
	MetalRenderer& operator=(const MetalRenderer&) = delete;

	bool initialize(refimport_t imports);
	void shutdown(qboolean destroyWindow);

	void beginRegistration(glconfig_t* configOut);
	void beginFrame();
	void endFrame(int* frontEndMsec, int* backEndMsec);

	void uploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);
	void drawCinematic(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);

	const glconfig_t& config() const { return config_; }

private:
	struct CinematicSlot {
		MetalPtr<MTL::Texture> texture;
		int cols = 0;
		int rows = 0;
	};

	struct PendingCinematic {
		std::vector<uint8_t> data;
		int client = 0;
		int cols = 0;
		int rows = 0;
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		bool dirty = false;
	};

	refimport_t ri_{};
	MetalPtr<MTL::Device> device_;
	MetalPtr<MTL::CommandQueue> commandQueue_;
	CA::MetalLayer* layer_ = nullptr;  // Not owned, managed by SDL
	MetalPtr<MTL::RenderPipelineState> pipeline_;
	MetalPtr<MTL::SamplerState> sampler_;

	std::array<CinematicSlot, 8> cinematicSlots_;
	PendingCinematic pendingCinematic_;

	// Current frame cinematic texture (not owned, points into cinematicSlots_)
	MTL::Texture* frameCinematicTexture_ = nullptr;
	int frameCinematicCols_ = 0;
	int frameCinematicRows_ = 0;

	glconfig_t config_{};
	bool inputInitialized_ = false;

	bool initializeWindow(int& width, int& height, qboolean& fullscreen);
	bool createPipeline(MTL::Texture* drawableTexture);
	void processPendingUploads();
	void fillConfigDefaults(int width, int height, qboolean fullscreen);
};

//=============================================================================
// Implementation
//=============================================================================

bool MetalRenderer::initialize(refimport_t imports) {
	ri_ = imports;
	return true;
}

void MetalRenderer::shutdown(qboolean destroyWindow) {
	// RAII handles Metal object cleanup automatically
	sampler_.reset();
	pipeline_.reset();
	
	for (auto& slot : cinematicSlots_) {
		slot.texture.reset();
		slot.cols = slot.rows = 0;
	}
	
	commandQueue_.reset();
	device_.reset();

	// Shutdown input before SDL
	if (inputInitialized_) {
		ri_.IN_Shutdown();
		inputInitialized_ = false;
	}

	SDLMetal_Shutdown(destroyWindow);
	layer_ = nullptr;
	frameCinematicTexture_ = nullptr;
}

void MetalRenderer::fillConfigDefaults(int width, int height, qboolean fullscreen) {
	std::snprintf(config_.renderer_string, sizeof(config_.renderer_string), "Metal");
	std::snprintf(config_.vendor_string, sizeof(config_.vendor_string), "Apple");
	std::snprintf(config_.version_string, sizeof(config_.version_string), "Metal");
	config_.extensions_string[0] = '\0';

	config_.maxTextureSize = 0;
	config_.numTextureUnits = 0;
	config_.colorBits = 32;
	config_.depthBits = 24;
	config_.stencilBits = 8;
	config_.driverType = GLDRV_ICD;
	config_.hardwareType = GLHW_GENERIC;
	config_.deviceSupportsGamma = qfalse;
	config_.textureCompression = TC_NONE;
	config_.textureEnvAddAvailable = qfalse;
	config_.vidWidth = width;
	config_.vidHeight = height;
	config_.windowAspect = static_cast<float>(width) / static_cast<float>(height);
	config_.displayFrequency = 0;
	config_.isFullscreen = fullscreen;
	config_.stereoEnabled = qfalse;
	config_.smpActive = qfalse;
}

bool MetalRenderer::initializeWindow(int& width, int& height, qboolean& fullscreen) {
	// Get window size preferences from cvars
	int cw = ri_.Cvar_VariableIntegerValue("r_customwidth");
	int ch = ri_.Cvar_VariableIntegerValue("r_customheight");
	fullscreen = ri_.Cvar_VariableIntegerValue("r_fullscreen") ? qtrue : qfalse;
	
	if (cw > 0 && ch > 0) {
		width = cw;
		height = ch;
	} else {
		SDL_DisplayMode dm;
		if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
			width = dm.w;
			height = dm.h;
		}
	}

	if (!SDLMetal_Init(width, height, fullscreen)) {
		return false;
	}

	device_.reset(MTL::CreateSystemDefaultDevice());
	if (!device_) {
		ri_.Printf(PRINT_ALL, "Metal device creation failed\n");
		SDLMetal_Shutdown(qtrue);
		return false;
	}

	commandQueue_.reset(device_->newCommandQueue());
	layer_ = reinterpret_cast<CA::MetalLayer*>(SDLMetal_GetLayer());
	
	if (layer_) {
		layer_->setDevice(device_.get());
		layer_->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
		layer_->setFramebufferOnly(false);
		layer_->setDisplaySyncEnabled(true);
		layer_->setMaximumDrawableCount(3);
		
		CGSize drawableSize;
		drawableSize.width = static_cast<CGFloat>(width);
		drawableSize.height = static_cast<CGFloat>(height);
		layer_->setDrawableSize(drawableSize);
	}

	fillConfigDefaults(width, height, fullscreen);
	return true;
}

bool MetalRenderer::createPipeline(MTL::Texture* drawableTexture) {
	if (pipeline_ && sampler_) {
		return true;
	}
	if (!device_ || !drawableTexture) {
		return false;
	}

	// Load from precompiled default.metallib
	NS::Error* error = nullptr;
	MTL::Library* lib = device_->newDefaultLibrary();
	
	if (!lib) {
		ri_.Printf(PRINT_WARNING, "Metal: Failed to load default.metallib\n");
		return false;
	}

	// Load shader functions from library
	NS::String* vertexName = NS::String::string("vertex_cinematic", NS::ASCIIStringEncoding);
	NS::String* fragmentName = NS::String::string("fragment_cinematic", NS::ASCIIStringEncoding);
	
	MTL::Function* vfn = lib->newFunction(vertexName);
	MTL::Function* ffn = lib->newFunction(fragmentName);
	
	vertexName->release();
	fragmentName->release();

	if (!vfn || !ffn) {
		ri_.Printf(PRINT_WARNING, "Metal: Shader functions not found in library\n");
		if (vfn) vfn->release();
		if (ffn) ffn->release();
		lib->release();
		return false;
	}

	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexFunction(vfn);
	pd->setFragmentFunction(ffn);
	pd->colorAttachments()->object(0)->setPixelFormat(drawableTexture->pixelFormat());

	pipeline_.reset(device_->newRenderPipelineState(pd, &error));

	pd->release();
	vfn->release();
	ffn->release();
	lib->release();

	if (!pipeline_) {
		if (error) {
			ri_.Printf(PRINT_WARNING, "Metal pipeline creation failed: %s\n", error->localizedDescription()->utf8String());
			error->release();
		}
		return false;
	}

	// Create sampler
	MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
	sd->setMinFilter(MTL::SamplerMinMagFilterLinear);
	sd->setMagFilter(MTL::SamplerMinMagFilterLinear);
	sd->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
	sd->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
	sampler_.reset(device_->newSamplerState(sd));
	sd->release();

	return true;
}

void MetalRenderer::beginFrame() {
	if (!device_) {
		int width = 1280;
		int height = 720;
		qboolean fullscreen = qfalse;
		
		if (!initializeWindow(width, height, fullscreen)) {
			ri_.Error(ERR_FATAL, "Metal renderer failed to create window");
			return;
		}
	}

	int w = 0, h = 0;
	SDLMetal_GetDrawableSize(&w, &h);
	
	if (w > 0 && h > 0) {
		config_.vidWidth = w;
		config_.vidHeight = h;
		config_.windowAspect = static_cast<float>(w) / static_cast<float>(h);
		
		if (layer_) {
			CGSize drawableSize;
			drawableSize.width = static_cast<CGFloat>(w);
			drawableSize.height = static_cast<CGFloat>(h);
			layer_->setDrawableSize(drawableSize);
		}
	}
}

void MetalRenderer::processPendingUploads() {
	if (!pendingCinematic_.dirty || !device_) {
		return;
	}

	int clientIndex = std::clamp(pendingCinematic_.client, 0, static_cast<int>(cinematicSlots_.size()) - 1);
	CinematicSlot& slot = cinematicSlots_[clientIndex];

	// Recreate texture if dimensions changed
	if (!slot.texture || slot.cols != pendingCinematic_.cols || slot.rows != pendingCinematic_.rows) {
		MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
			MTL::PixelFormatRGBA8Unorm, pendingCinematic_.cols, pendingCinematic_.rows, false);
		desc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead);
		desc->setStorageMode(MTL::StorageModeShared);
		slot.texture.reset(device_->newTexture(desc));
		slot.cols = pendingCinematic_.cols;
		slot.rows = pendingCinematic_.rows;
		desc->release();
	}

	// Upload pixel data
	if (slot.texture) {
		MTL::Region region = MTL::Region::Make2D(0, 0, pendingCinematic_.cols, pendingCinematic_.rows);
		slot.texture->replaceRegion(region, 0, pendingCinematic_.data.data(), pendingCinematic_.cols * 4);
	}

	pendingCinematic_.dirty = false;
	frameCinematicTexture_ = slot.texture.get();
	frameCinematicCols_ = slot.cols;
	frameCinematicRows_ = slot.rows;
}

void MetalRenderer::endFrame(int* frontEndMsec, int* backEndMsec) {
	if (frontEndMsec) *frontEndMsec = 0;
	if (backEndMsec) *backEndMsec = 0;

	if (!layer_ || !commandQueue_) {
		return;
	}

	CA::MetalDrawable* drawable = layer_->nextDrawable();
	if (!drawable) {
		return;
	}

	createPipeline(drawable->texture());
	if (!pipeline_ || !sampler_) {
		drawable->release();
		return;
	}

	processPendingUploads();

	if (!frameCinematicTexture_) {
		drawable->release();
		return;
	}

	// Build quad vertices
	struct Vertex {
		float pos[2];
		float uv[2];
	};

	const float fbWidth = static_cast<float>(config_.vidWidth);
	const float fbHeight = static_cast<float>(config_.vidHeight);
	const float ndcLeft = (pendingCinematic_.x * 2.0f / fbWidth) - 1.0f;
	const float ndcRight = ((pendingCinematic_.x + pendingCinematic_.w) * 2.0f / fbWidth) - 1.0f;
	const float ndcTop = 1.0f - (pendingCinematic_.y * 2.0f / fbHeight);
	const float ndcBottom = 1.0f - ((pendingCinematic_.y + pendingCinematic_.h) * 2.0f / fbHeight);

	const float u0 = 0.5f / static_cast<float>(frameCinematicCols_);
	const float v0 = 0.5f / static_cast<float>(frameCinematicRows_);
	const float u1 = (static_cast<float>(frameCinematicCols_) - 0.5f) / static_cast<float>(frameCinematicCols_);
	const float v1 = (static_cast<float>(frameCinematicRows_) - 0.5f) / static_cast<float>(frameCinematicRows_);

	Vertex verts[6] = {
		{{ndcLeft, ndcTop}, {u0, v0}},
		{{ndcRight, ndcTop}, {u1, v0}},
		{{ndcRight, ndcBottom}, {u1, v1}},
		{{ndcLeft, ndcTop}, {u0, v0}},
		{{ndcRight, ndcBottom}, {u1, v1}},
		{{ndcLeft, ndcBottom}, {u0, v1}},
	};

	// Render
	MTL::RenderPassDescriptor* rp = MTL::RenderPassDescriptor::renderPassDescriptor();
	rp->colorAttachments()->object(0)->setTexture(drawable->texture());
	rp->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
	rp->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

	MTL::CommandBuffer* cb = commandQueue_->commandBuffer();
	MTL::RenderCommandEncoder* enc = cb->renderCommandEncoder(rp);
	enc->setRenderPipelineState(pipeline_.get());

	MTL::Viewport vp;
	vp.originX = 0.0;
	vp.originY = 0.0;
	vp.width = static_cast<double>(drawable->texture()->width());
	vp.height = static_cast<double>(drawable->texture()->height());
	vp.znear = 0.0;
	vp.zfar = 1.0;
	enc->setViewport(vp);

	enc->setVertexBytes(verts, sizeof(verts), 0);
	enc->setFragmentTexture(frameCinematicTexture_, 0);
	enc->setFragmentSamplerState(sampler_.get(), 0);
	enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
	enc->endEncoding();

	cb->presentDrawable(drawable);
	cb->commit();

	enc->release();
	rp->release();
	drawable->release();
}

void MetalRenderer::uploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	(void)w;
	(void)h;

	if (!data || cols <= 0 || rows <= 0) {
		return;
	}

	// Validate power-of-two
	auto isPowerOfTwo = [](int v) { return v > 0 && (v & (v - 1)) == 0; };
	if (!isPowerOfTwo(cols) || !isPowerOfTwo(rows)) {
		ri_.Printf(PRINT_WARNING, "Metal: UploadCinematic got non-power-of-two %dx%d\n", cols, rows);
		return;
	}

	const size_t expected = static_cast<size_t>(cols) * static_cast<size_t>(rows) * 4;
	pendingCinematic_.data.assign(data, data + expected);
	pendingCinematic_.cols = cols;
	pendingCinematic_.rows = rows;
	pendingCinematic_.client = std::clamp(client, 0, static_cast<int>(cinematicSlots_.size()) - 1);
	pendingCinematic_.dirty = dirty ? true : false;
}

void MetalRenderer::drawCinematic(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	if (!data || cols <= 0 || rows <= 0 || w <= 0 || h <= 0) {
		return;
	}

	uploadCinematic(w, h, cols, rows, data, client, dirty);

	pendingCinematic_.x = x;
	pendingCinematic_.y = y;
	pendingCinematic_.w = w;
	pendingCinematic_.h = h;
}

void MetalRenderer::beginRegistration(glconfig_t* configOut) {
	if (!device_) {
		int width = 1280;
		int height = 720;
		qboolean fullscreen = qfalse;
		
		if (!initializeWindow(width, height, fullscreen)) {
			ri_.Error(ERR_FATAL, "Metal renderer failed to create SDL window");
			return;
		}
	}

	if (configOut) {
		*configOut = config_;
	}

	if (!inputInitialized_) {
		ri_.IN_Init(SDLMetal_GetWindow());
		inputInitialized_ = true;
	}
}

//=============================================================================
// Global Renderer Instance
//=============================================================================

static MetalRenderer g_renderer;

//=============================================================================
// C API Exports
//=============================================================================

extern "C" {

static void Metal_Shutdown(qboolean destroyWindow) {
	g_renderer.shutdown(destroyWindow);
}

static void Metal_BeginRegistration(glconfig_t* glconfigOut) {
	g_renderer.beginRegistration(glconfigOut);
}

static void Metal_BeginFrame(stereoFrame_t stereoFrame) {
	(void)stereoFrame;
	g_renderer.beginFrame();
}

static void Metal_EndFrame(int* frontEndMsec, int* backEndMsec) {
	g_renderer.endFrame(frontEndMsec, backEndMsec);
}

static void Metal_UploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	g_renderer.uploadCinematic(w, h, cols, rows, data, client, dirty);
}

static void Metal_DrawStretchRaw(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	g_renderer.drawCinematic(x, y, w, h, cols, rows, data, client, dirty);
}

#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t* QDECL GetRefAPI(int apiVersion, refimport_t* rimp)
#else
refexport_t* GetRefAPI(int apiVersion, refimport_t* rimp)
#endif
{
	static refexport_t re;

	if (apiVersion != REF_API_VERSION) {
		return nullptr;
	}

	if (!g_renderer.initialize(*rimp)) {
		return nullptr;
	}

	std::memset(&re, 0, sizeof(re));

	re.Shutdown = Metal_Shutdown;
	re.BeginRegistration = Metal_BeginRegistration;
	re.RegisterModel = [](const char*) { return 0; };
	re.RegisterSkin = [](const char*) { return 0; };
	re.RegisterShader = [](const char*) { return 0; };
	re.RegisterShaderNoMip = [](const char*) { return 0; };
	re.LoadWorld = [](const char*) {};
	re.SetWorldVisData = [](const byte*) {};
	re.EndRegistration = []() {};

	re.BeginFrame = Metal_BeginFrame;
	re.EndFrame = Metal_EndFrame;

	re.MarkFragments = [](int, const vec3_t*, const vec3_t, int, vec3_t, int, markFragment_t*) { return 0; };
	re.LerpTag = [](orientation_t* tag, qhandle_t, int, int, float, const char*) {
		if (tag) Com_Memset(tag, 0, sizeof(*tag));
		return 0;
	};
	re.ModelBounds = [](qhandle_t, vec3_t mins, vec3_t maxs) {
		if (mins) VectorClear(mins);
		if (maxs) VectorClear(maxs);
	};

	re.ClearScene = []() {};
	re.AddRefEntityToScene = [](const refEntity_t*) {};
	re.AddPolyToScene = [](qhandle_t, int, const polyVert_t*, int) {};
	re.LightForPoint = [](vec3_t, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
		if (ambientLight) VectorClear(ambientLight);
		if (directedLight) VectorClear(directedLight);
		if (lightDir) VectorClear(lightDir);
		return 0;
	};
	re.AddLightToScene = [](const vec3_t, float, float, float, float) {};
	re.AddAdditiveLightToScene = [](const vec3_t, float, float, float, float) {};
	re.RenderScene = [](const refdef_t*) {};

	re.SetColor = [](const float*) {};
	re.DrawStretchPic = [](float, float, float, float, float, float, float, float, qhandle_t) {};
	re.DrawStretchRaw = Metal_DrawStretchRaw;
	re.UploadCinematic = Metal_UploadCinematic;

	re.RegisterFont = [](const char*, int, fontInfo_t* font) {
		if (font) std::memset(font, 0, sizeof(*font));
	};
	re.RemapShader = [](const char*, const char*, const char*) {};
	re.GetEntityToken = [](char* buffer, int size) {
		if (buffer && size > 0) buffer[0] = '\0';
		return qfalse;
	};
	re.inPVS = [](const vec3_t, const vec3_t) { return qfalse; };

	re.TakeVideoFrame = [](int, int, byte*, byte*, qboolean) {};

	return &re;
}

} // extern "C"
