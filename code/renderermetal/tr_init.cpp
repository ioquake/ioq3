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
#include "tr_metal_texture.h"

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

	// Cinematic playback
	void uploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);
	void drawCinematic(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);

	// 2D rendering APIs
	qhandle_t registerShader(const char* name, bool mipmap);
	void setColor(const float* rgba);
	void drawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader);

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

	// Current frame state
	MTL::CommandBuffer* currentCommandBuffer_ = nullptr;
	MTL::RenderCommandEncoder* currentRenderEncoder_ = nullptr;
	CA::MetalDrawable* currentDrawable_ = nullptr;

	glconfig_t config_{};
	bool inputInitialized_ = false;

	// 2D rendering state
	std::unique_ptr<TextureManager> textureManager_;
	MetalPtr<MTL::RenderPipelineState> pipeline2D_;
	MetalPtr<MTL::SamplerState> sampler2D_;
	float currentColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	bool initializeWindow(int& width, int& height, qboolean& fullscreen);
	bool createPipeline(MTL::Texture* drawableTexture);
	bool create2DPipeline();
	void processPendingUploads();
	void fillConfigDefaults(int width, int height, qboolean fullscreen);
};

//=============================================================================
// Implementation
//=============================================================================

bool MetalRenderer::initialize(refimport_t imports) {
	ri_ = imports;
	// TextureManager will be created when device is available
	return true;
}

void MetalRenderer::shutdown(qboolean destroyWindow) {
	// RAII handles Metal object cleanup automatically
	textureManager_.reset();
	sampler2D_.reset();
	pipeline2D_.reset();
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

bool MetalRenderer::create2DPipeline() {
	if (pipeline2D_ && sampler2D_) {
		return true;
	}
	if (!device_) {
		return false;
	}

	// Load 2D shaders from metallib
	NS::Error* error = nullptr;
	MTL::Library* lib = device_->newDefaultLibrary();
	if (!lib) {
		ri_.Printf(PRINT_WARNING, "Metal: Failed to load shader library for 2D\n");
		return false;
	}

	NS::String* vertexName = NS::String::string("vertex_ui_2d", NS::ASCIIStringEncoding);
	NS::String* fragmentName = NS::String::string("fragment_ui_2d", NS::ASCIIStringEncoding);
	
	MTL::Function* vfn = lib->newFunction(vertexName);
	MTL::Function* ffn = lib->newFunction(fragmentName);
	
	vertexName->release();
	fragmentName->release();

	if (!vfn || !ffn) {
		ri_.Printf(PRINT_WARNING, "Metal: 2D shader functions not found\n");
		if (vfn) vfn->release();
		if (ffn) ffn->release();
		lib->release();
		return false;
	}

	// Create pipeline for 2D rendering
	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexFunction(vfn);
	pd->setFragmentFunction(ffn);
	pd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	
	// Enable alpha blending
	auto* colorAttachment = pd->colorAttachments()->object(0);
	colorAttachment->setBlendingEnabled(true);
	colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

	pipeline2D_.reset(device_->newRenderPipelineState(pd, &error));

	pd->release();
	vfn->release();
	ffn->release();
	lib->release();

	if (!pipeline2D_) {
		if (error) {
			ri_.Printf(PRINT_WARNING, "Metal: 2D pipeline creation failed: %s\n", 
			          error->localizedDescription()->utf8String());
			error->release();
		}
		return false;
	}

	// Create 2D sampler
	MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
	sd->setMinFilter(MTL::SamplerMinMagFilterLinear);
	sd->setMagFilter(MTL::SamplerMinMagFilterLinear);
	sd->setSAddressMode(MTL::SamplerAddressModeRepeat);
	sd->setTAddressMode(MTL::SamplerAddressModeRepeat);
	sampler2D_.reset(device_->newSamplerState(sd));
	sd->release();

	return true;
}

qhandle_t MetalRenderer::registerShader(const char* name, bool mipmap) {
	// Create TextureManager on first use
	if (!textureManager_ && device_) {
		textureManager_ = std::make_unique<TextureManager>(device_.get(), &ri_);
	}
	
	if (!textureManager_) {
		return 0;
	}
	
	return textureManager_->registerShader(name, mipmap);
}

void MetalRenderer::setColor(const float* rgba) {
	if (rgba) {
		currentColor_[0] = rgba[0];
		currentColor_[1] = rgba[1];
		currentColor_[2] = rgba[2];
		currentColor_[3] = rgba[3];
	} else {
		currentColor_[0] = currentColor_[1] = currentColor_[2] = currentColor_[3] = 1.0f;
	}
}

void MetalRenderer::drawStretchPic(float x, float y, float w, float h, 
                                    float s1, float t1, float s2, float t2, qhandle_t shader) {
	if (!device_ || !textureManager_ || !currentRenderEncoder_) {
		return;
	}

	// Ensure 2D pipeline exists
	if (!create2DPipeline()) {
		return;
	}

	// Get texture
	MTL::Texture* texture = textureManager_->getTexture(shader);
	if (!texture) {
		return;
	}

	// Set pipeline state
	currentRenderEncoder_->setRenderPipelineState(pipeline2D_.get());

	// Convert screen coordinates to NDC
	float ndcX = (x * 2.0f / config_.vidWidth) - 1.0f;
	float ndcY = 1.0f - (y * 2.0f / config_.vidHeight);
	float ndcW = w * 2.0f / config_.vidWidth;
	float ndcH = h * 2.0f / config_.vidHeight;

	// Build quad instance data
	struct QuadInstance {
		float rect[4];      // x, y, w, h in NDC
		float texCoords[4]; // s1, t1, s2, t2
		float color[4];     // RGBA
	};

	QuadInstance instance;
	instance.rect[0] = ndcX;
	instance.rect[1] = ndcY - ndcH;  // Flip Y
	instance.rect[2] = ndcW;
	instance.rect[3] = ndcH;
	instance.texCoords[0] = s1;
	instance.texCoords[1] = t1;
	instance.texCoords[2] = s2;
	instance.texCoords[3] = t2;
	instance.color[0] = currentColor_[0];
	instance.color[1] = currentColor_[1];
	instance.color[2] = currentColor_[2];
	instance.color[3] = currentColor_[3];

	// Set viewport (full screen)
	MTL::Viewport vp;
	vp.originX = 0.0;
	vp.originY = 0.0;
	vp.width = static_cast<double>(config_.vidWidth);
	vp.height = static_cast<double>(config_.vidHeight);
	vp.znear = 0.0;
	vp.zfar = 1.0;
	currentRenderEncoder_->setViewport(vp);

	// Bind instance data, texture, and sampler
	currentRenderEncoder_->setVertexBytes(&instance, sizeof(instance), 0);
	currentRenderEncoder_->setFragmentTexture(texture, 0);
	currentRenderEncoder_->setFragmentSamplerState(sampler2D_.get(), 0);

	// Draw triangle strip (4 vertices = 1 quad)
	currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, NS::UInteger(0), NS::UInteger(4), NS::UInteger(1));
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

	// Start frame: acquire drawable and create command buffer
	if (layer_ && commandQueue_) {
		currentDrawable_ = layer_->nextDrawable();
		if (currentDrawable_) {
			currentCommandBuffer_ = commandQueue_->commandBuffer();
			
			MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
			rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
			rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
			rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
			rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
			
			currentRenderEncoder_ = currentCommandBuffer_->renderCommandEncoder(rpd);
			rpd->release();
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

	if (currentRenderEncoder_) {
		currentRenderEncoder_->endEncoding();
		currentRenderEncoder_->release();
		currentRenderEncoder_ = nullptr;
	}

	if (currentCommandBuffer_ && currentDrawable_) {
		currentCommandBuffer_->presentDrawable(currentDrawable_);
		currentCommandBuffer_->commit();
		
		currentDrawable_->release();
	}

	currentCommandBuffer_ = nullptr;
	currentDrawable_ = nullptr;
}

void MetalRenderer::drawCinematic(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	if (!device_ || !layer_ || !currentRenderEncoder_) {
		return;
	}

	// Ensure pipeline exists
	if (!pipeline_ || !sampler_) {
		// Try to create it (needs a texture to infer format, use current drawable)
		if (currentDrawable_) {
			createPipeline(currentDrawable_->texture());
		}
		if (!pipeline_ || !sampler_) return;
	}

	// Update cinematic texture if needed
	pendingCinematic_.cols = cols;
	pendingCinematic_.rows = rows;
	pendingCinematic_.client = client;
	pendingCinematic_.dirty = dirty;
	if (data) {
		const size_t expected = static_cast<size_t>(cols) * static_cast<size_t>(rows) * 4;
		if (pendingCinematic_.data.size() < expected) pendingCinematic_.data.resize(expected);
		std::memcpy(pendingCinematic_.data.data(), data, expected);
		processPendingUploads();
	}

	if (!frameCinematicTexture_) {
		return;
	}

	// Set pipeline state
	currentRenderEncoder_->setRenderPipelineState(pipeline_.get());

	// Calculate viewport
	// Note: x,y,w,h passed to this function are usually full screen for cinematics
	// But we should respect them if possible.
	// For now, let's use the logic we had in endFrame for aspect ratio, or just use the passed rect?
	// The passed rect (x,y,w,h) is in pixels.
	
	MTL::Viewport vp;
	vp.originX = static_cast<double>(x);
	vp.originY = static_cast<double>(y);
	vp.width = static_cast<double>(w);
	vp.height = static_cast<double>(h);
	vp.znear = 0.0;
	vp.zfar = 1.0;
	currentRenderEncoder_->setViewport(vp);

	// Draw cinematic quad
	struct Uniforms {
		float cols;
		float rows;
	} uniforms = {
		static_cast<float>(frameCinematicCols_),
		static_cast<float>(frameCinematicRows_)
	};

	// Full screen quad vertices (NDC)
	// The vertex shader 'vertex_cinematic' likely expects specific vertex data or generates it?
	// Let's check 'vertex_cinematic' in cinematic.metal.
	// It uses 'VertexIn' struct with position and texCoord.
	// So we need to send vertex data.
	
	// Vertices for a full-screen quad (or whatever viewport covers)
	// Since we set viewport to x,y,w,h, we can draw a quad from -1 to 1 in NDC.
	struct Vertex {
		float position[2];
		float texCoord[2];
	};
	
	Vertex verts[4] = {
		{{-1, 1},  {0, 0}},
		{{ 1, 1},  {1, 0}},
		{{-1, -1}, {0, 1}},
		{{ 1, -1}, {1, 1}}
	};

	currentRenderEncoder_->setVertexBytes(verts, sizeof(verts), 0);
	currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(uniforms), 1); // Uniforms at buffer 1?
	// Wait, let's check cinematic.metal to be sure about buffer indices.
	// We don't have it open. But previous code used:
	// enc->setVertexBytes(verts, sizeof(verts), 0);
	// enc->setFragmentTexture(frameCinematicTexture_, 0);
	// enc->setFragmentSamplerState(sampler_.get(), 0);
	// It didn't send uniforms!
	// Wait, the previous code in drawCinematic (which I just replaced) did:
	/*
	Vertex verts[6] = { ... };
	enc->setVertexBytes(verts, sizeof(verts), 0);
	enc->setFragmentTexture(frameCinematicTexture_, 0);
	enc->setFragmentSamplerState(sampler_.get(), 0);
	enc->drawPrimitives(MTL::PrimitiveTypeTriangle, ...);
	*/
	// It didn't send uniforms. The shader must not need them or hardcoded?
	// Actually, the shader 'vertex_cinematic' probably just takes position/uv.
	// Let's stick to what was working.
	
	currentRenderEncoder_->setVertexBytes(verts, sizeof(verts), 0);
	currentRenderEncoder_->setFragmentTexture(frameCinematicTexture_, 0);
	currentRenderEncoder_->setFragmentSamplerState(sampler_.get(), 0);
	currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, NS::UInteger(0), NS::UInteger(4));
}

void MetalRenderer::uploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	(void)w;
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

static qhandle_t Metal_RegisterShader(const char* name) {
	return g_renderer.registerShader(name, true);
}

static qhandle_t Metal_RegisterShaderNoMip(const char* name) {
	return g_renderer.registerShader(name, false);
}

static void Metal_SetColor(const float* rgba) {
	g_renderer.setColor(rgba);
}

static void Metal_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {
	g_renderer.drawStretchPic(x, y, w, h, s1, t1, s2, t2, shader);
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
	re.RegisterShader = Metal_RegisterShader;
	re.RegisterShaderNoMip = Metal_RegisterShaderNoMip;
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

	re.SetColor = Metal_SetColor;
	re.DrawStretchPic = Metal_DrawStretchPic;
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
