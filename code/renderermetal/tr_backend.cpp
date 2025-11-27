/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2025 Modern Metal Renderer Implementation

Metal renderer with RAII and modern C++ practices
===========================================================================
*/

#include "tr_backend.h"
#include "tr_capabilities.h"
#include "tr_scene.h"
#include "tr_extramath.h"

extern "C" {
#include "../qcommon/qfiles.h"
}

extern "C" {
	glconfig_t glConfig = {};
	extern refimport_t ri;
}

static void Metal_GfxInfo_f();
static void Metal_ImageList_f();
static void Metal_DebugPoly_f();

namespace {
constexpr int kBspLightmapWidth = 128;
constexpr int kBspLightmapHeight = 128;
constexpr int kBspLightmapBytes = kBspLightmapWidth * kBspLightmapHeight * 3;

struct StageFragmentParams {
	float alphaRef = 0.0f;
	float alphaFunc = 0.0f;
	float alphaTestEnabled = 0.0f;
	float texCoordSelector = 0.0f;
};

// Match Metal's PolyVertex structure EXACTLY with packed layout
// Metal shader uses packed_float3 which has no padding
struct __attribute__((packed)) MetalPolyVertex {
	float xyz[3];        // 12 bytes - position (packed_float3)
	float st[2];         // 8 bytes  - texCoord (packed_float2)
	float lightmap[2];   // 8 bytes  - lightmapCoord (packed_float2)
	float normal[3];     // 12 bytes - normal (packed_float3)
	byte modulate[4];    // 4 bytes  - color (uchar4)
	                     // Total: 44 bytes

	MetalPolyVertex() : xyz{}, st{}, lightmap{}, normal{}, modulate{255, 255, 255, 255} {}
};

// Verify structure matches Metal shader expectations
static_assert(sizeof(MetalPolyVertex) == 44, "MetalPolyVertex must be exactly 44 bytes to match Metal shader");
}

namespace {
inline MetalPolyVertex ConvertDrawVert(const drawVert_t& src) {
	MetalPolyVertex dst{};
	// Assume Little Endian platform and Little Endian BSP
	dst.xyz[0] = src.xyz[0];
	dst.xyz[1] = src.xyz[1];
	dst.xyz[2] = src.xyz[2];
	dst.st[0] = src.st[0];
	dst.st[1] = src.st[1];
	dst.lightmap[0] = src.lightmap[0];
	dst.lightmap[1] = src.lightmap[1];
	dst.normal[0] = src.normal[0];
	dst.normal[1] = src.normal[1];
	dst.normal[2] = src.normal[2];
	dst.modulate[0] = src.color[0];
	dst.modulate[1] = src.color[1];
	dst.modulate[2] = src.color[2];
	dst.modulate[3] = src.color[3];
	return dst;
}

inline MetalPolyVertex ConvertPolyVert(const polyVert_t& src) {
	MetalPolyVertex dst{};
	dst.xyz[0] = src.xyz[0];
	dst.xyz[1] = src.xyz[1];
	dst.xyz[2] = src.xyz[2];
	dst.st[0] = src.st[0];
	dst.st[1] = src.st[1];
	dst.lightmap[0] = src.st[0];
	dst.lightmap[1] = src.st[1];
	// polyVert_t has no normal, use default (0,0,1) or (0,0,0)
	dst.normal[0] = 0.0f;
	dst.normal[1] = 0.0f;
	dst.normal[2] = 1.0f;
	dst.modulate[0] = src.modulate[0];
	dst.modulate[1] = src.modulate[1];
	dst.modulate[2] = src.modulate[2];
	dst.modulate[3] = src.modulate[3];
	return dst;
}
}

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
#include <cstdint>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdlib>

#include "tr_utils.h"
#include "tr_texture.h"
#include "tr_dsa.h"
#include "tr_extensions.h"
#include "tr_shader.h"

//=============================================================================
// Bezier Patch Tessellation
//=============================================================================

namespace {
// Tessellation level for curved surfaces (higher = smoother, more triangles)
constexpr int PATCH_SUBDIVISIONS = 8;

// Bezier patch tessellation for curved surfaces
// Quake 3 patches are quadratic Bezier surfaces defined by a grid of control points
// Each 3x3 block of control points defines one Bezier patch

inline MetalPolyVertex LerpVertex(const MetalPolyVertex& a, const MetalPolyVertex& b, float t) {
	MetalPolyVertex out{};
	float omt = 1.0f - t;
	out.xyz[0] = a.xyz[0] * omt + b.xyz[0] * t;
	out.xyz[1] = a.xyz[1] * omt + b.xyz[1] * t;
	out.xyz[2] = a.xyz[2] * omt + b.xyz[2] * t;
	out.st[0] = a.st[0] * omt + b.st[0] * t;
	out.st[1] = a.st[1] * omt + b.st[1] * t;
	out.lightmap[0] = a.lightmap[0] * omt + b.lightmap[0] * t;
	out.lightmap[1] = a.lightmap[1] * omt + b.lightmap[1] * t;
	out.normal[0] = a.normal[0] * omt + b.normal[0] * t;
	out.normal[1] = a.normal[1] * omt + b.normal[1] * t;
	out.normal[2] = a.normal[2] * omt + b.normal[2] * t;
	// Lerp colors
	out.modulate[0] = static_cast<byte>(a.modulate[0] * omt + b.modulate[0] * t);
	out.modulate[1] = static_cast<byte>(a.modulate[1] * omt + b.modulate[1] * t);
	out.modulate[2] = static_cast<byte>(a.modulate[2] * omt + b.modulate[2] * t);
	out.modulate[3] = static_cast<byte>(a.modulate[3] * omt + b.modulate[3] * t);
	return out;
}

// Evaluate quadratic Bezier curve at parameter t (0 to 1)
// control points: p0, p1, p2
inline MetalPolyVertex EvaluateBezier(const MetalPolyVertex& p0, const MetalPolyVertex& p1, const MetalPolyVertex& p2, float t) {
	// B(t) = (1-t)^2 * P0 + 2*(1-t)*t * P1 + t^2 * P2
	MetalPolyVertex a = LerpVertex(p0, p1, t);
	MetalPolyVertex b = LerpVertex(p1, p2, t);
	return LerpVertex(a, b, t);
}

// Evaluate a 3x3 Bezier patch at (u, v) where u,v are in [0,1]
inline MetalPolyVertex EvaluatePatch3x3(const MetalPolyVertex ctrl[3][3], float u, float v) {
	// First interpolate along u for each row
	MetalPolyVertex temp[3];
	temp[0] = EvaluateBezier(ctrl[0][0], ctrl[0][1], ctrl[0][2], u);
	temp[1] = EvaluateBezier(ctrl[1][0], ctrl[1][1], ctrl[1][2], u);
	temp[2] = EvaluateBezier(ctrl[2][0], ctrl[2][1], ctrl[2][2], u);
	// Then interpolate along v
	return EvaluateBezier(temp[0], temp[1], temp[2], v);
}

// Tessellate a single 3x3 Bezier patch into triangles
// tessLevel is the number of subdivisions (e.g., 8 means 8x8 grid = 64 quads = 128 triangles)
inline void TessellatePatch3x3(const MetalPolyVertex ctrl[3][3], int tessLevel,
                               std::vector<MetalPolyVertex>& outVerts) {
	if (tessLevel < 1) tessLevel = 1;
	if (tessLevel > 32) tessLevel = 32;
	
	const float step = 1.0f / static_cast<float>(tessLevel);
	
	// Generate a grid of vertices
	std::vector<MetalPolyVertex> grid(static_cast<size_t>((tessLevel + 1) * (tessLevel + 1)));
	for (int j = 0; j <= tessLevel; ++j) {
		float v = static_cast<float>(j) * step;
		for (int i = 0; i <= tessLevel; ++i) {
			float u = static_cast<float>(i) * step;
			grid[static_cast<size_t>(j * (tessLevel + 1) + i)] = EvaluatePatch3x3(ctrl, u, v);
		}
	}
	
	// Generate triangles from the grid
	for (int j = 0; j < tessLevel; ++j) {
		for (int i = 0; i < tessLevel; ++i) {
			size_t idx00 = static_cast<size_t>(j * (tessLevel + 1) + i);
			size_t idx10 = static_cast<size_t>(j * (tessLevel + 1) + i + 1);
			size_t idx01 = static_cast<size_t>((j + 1) * (tessLevel + 1) + i);
			size_t idx11 = static_cast<size_t>((j + 1) * (tessLevel + 1) + i + 1);
			
			// Two triangles per quad
			outVerts.push_back(grid[idx00]);
			outVerts.push_back(grid[idx10]);
			outVerts.push_back(grid[idx11]);
			
			outVerts.push_back(grid[idx00]);
			outVerts.push_back(grid[idx11]);
			outVerts.push_back(grid[idx01]);
		}
	}
}

// Tessellate a full Bezier patch mesh (width x height control points)
// Quake 3 patches have dimensions that are 2*n+1 (e.g., 3, 5, 7, 9...)
// Each overlapping 3x3 section is a separate Bezier patch
inline void TessellateBezierPatch(const drawVert_t* controlPoints, int width, int height,
                                  std::vector<MetalPolyVertex>& outVerts, int tessLevel) {
	// Convert all control points to our vertex format
	std::vector<MetalPolyVertex> ctrl(static_cast<size_t>(width * height));
	for (int i = 0; i < width * height; ++i) {
		ctrl[static_cast<size_t>(i)] = ConvertDrawVert(controlPoints[i]);
	}
	
	// Number of patches in each direction
	int numPatchesX = (width - 1) / 2;
	int numPatchesY = (height - 1) / 2;
	
	// Process each 3x3 patch
	for (int py = 0; py < numPatchesY; ++py) {
		for (int px = 0; px < numPatchesX; ++px) {
			// Extract 3x3 control points for this patch
			MetalPolyVertex patch[3][3];
			for (int j = 0; j < 3; ++j) {
				for (int i = 0; i < 3; ++i) {
					int cx = px * 2 + i;
					int cy = py * 2 + j;
					patch[j][i] = ctrl[static_cast<size_t>(cy * width + cx)];
				}
			}
			TessellatePatch3x3(patch, tessLevel, outVerts);
		}
	}
}
} // end anonymous namespace for patch tessellation

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
	void beginFrame(stereoFrame_t stereoFrame);
	void endFrame(int* frontEndMsec, int* backEndMsec);
	void submitScene();
	void processScene(const MetalSceneState& scene);
	void processEntities(const MetalSceneState& scene);
	void processPolys(const MetalSceneState& scene);
	void processLights(const MetalSceneState& scene);

	// Cinematic playback
	void uploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);
	void drawCinematic(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty);

	// 2D rendering APIs
	qhandle_t registerShader(const char* name, bool mipmap);
	qhandle_t registerModel(const char* name);
	qhandle_t registerSkin(const char* name);
	void setColor(const float* rgba);
	void drawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader);

	const glconfig_t& config() const { return config_; }
	bool callLoggingEnabled() const;
	void logRendererCall(const char* name) const;

	friend void Metal_GfxInfo_f();
	friend void Metal_ImageList_f();
	friend void Metal_DebugPoly_f();
	friend void MetalBackend_LoadWorld(const char* name);

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

	struct SceneStats {
		int entities = 0;
		int polys = 0;
		int lights = 0;
	} sceneStats_;

	struct SceneDrawPacket {
		refEntity_t entity;
	};

	struct ScenePolyPacket {
		qhandle_t shader = 0;
		int firstVertex = 0;
		int vertexCount = 0;
		MTL::PrimitiveType primitive = MTL::PrimitiveTypeTriangle;
		qhandle_t lightmapHandle = 0;
	};

	struct SceneLightPacket {
		MetalSceneLight light;
	};

	struct SceneLightGPU {
		float origin[4] = {};
		float color[4] = {};
		float params[4] = {};
	};

	struct SceneUniforms {
		float view[16] = {};
		float projection[16] = {};
		float viewProjection[16] = {};
		float inverseView[16] = {};
		float viewOrigin[4] = {};
		float clipInfo[4] = {};
		float timeInfo[4] = {};
	};

	struct SceneCamera {
		refdef_t refdef{};
		mat4_t viewMatrix{};
		mat4_t projectionMatrix{};
		mat4_t viewProjectionMatrix{};
		mat4_t inverseViewMatrix{};
		vec3_t viewOrigin{};
		vec3_t viewAxis[3]{};
		float zNear = 0.0f;
		float zFar = 0.0f;
		float zProj = 0.0f;
		float stereoSeparation = 0.0f;
		bool valid = false;
	};

	struct SceneDispatchSummary {
		int totalEntities = 0;
		int modelEntities = 0;
		int spriteEntities = 0;
		int beamEntities = 0;
		int polySurfaces = 0;
		int polyVertices = 0;
		int dynamicLights = 0;
	};

	struct MetalShaderResource {
		std::string name;
		MetalShaderScriptInfo script;
		bool hasScript = false;
		bool mipmap = true;
		std::vector<qhandle_t> imageHandles;
		qhandle_t primaryImageHandle = 0;
		struct MetalPipelineKey {
			MetalShaderBlendMode blendMode = MetalShaderBlendMode::Opaque;
			MetalBlendFactor srcBlend = MetalBlendFactor::One;
			MetalBlendFactor dstBlend = MetalBlendFactor::Zero;
			bool depthWrite = true;
			bool depthTest = true;
			bool alphaTest = false;
			bool depthWriteExplicit = false;

			bool operator==(const MetalPipelineKey& other) const noexcept {
				return blendMode == other.blendMode &&
				       srcBlend == other.srcBlend &&
				       dstBlend == other.dstBlend &&
				       depthWrite == other.depthWrite &&
				       depthTest == other.depthTest &&
				       alphaTest == other.alphaTest &&
				       depthWriteExplicit == other.depthWriteExplicit;
			}
		};

		struct MetalShaderStageRuntime {
			const MetalShaderStageInfo* stageInfo = nullptr;
			MetalPipelineKey pipelineKey;
			std::vector<qhandle_t> stageImageHandles;
			qhandle_t primaryStageImage = 0;
			bool usesLightmap = false;
			bool usesWhiteImage = false;
		};

		std::vector<MetalShaderStageRuntime> stageRuntimes;
		size_t primaryStageIndex = 0;
	};

	std::vector<SceneDrawPacket> drawPackets_;
	std::vector<ScenePolyPacket> polyPackets_;
	std::vector<MetalPolyVertex> polyVertices_;
	std::vector<SceneLightPacket> lightPackets_;
	std::vector<SceneLightGPU> lightGPUData_;
	SceneCamera sceneCamera_;
	SceneUniforms sceneUniforms_{};
	SceneDispatchSummary sceneDispatchSummary_{};

	refimport_t ri_{};
	MetalPtr<MTL::Device> device_;
	MetalPtr<MTL::CommandQueue> commandQueue_;
	CA::MetalLayer* layer_ = nullptr;  // Not owned, managed by SDL
	MetalPtr<MTL::RenderPipelineState> pipeline_;
	MetalPtr<MTL::SamplerState> sampler_;

	std::array<CinematicSlot, 8> cinematicSlots_;
	PendingCinematic pendingCinematic_;

	cvar_t* r_znear_ = nullptr;
	cvar_t* r_zproj_ = nullptr;
	cvar_t* r_stereoSeparation_ = nullptr;
	cvar_t* r_metalLogCalls_ = nullptr;
	cvar_t* r_swapInterval_ = nullptr;
	MetalPtr<MTL::Buffer> sceneUniformBuffer_;
	size_t sceneUniformBufferSize_ = 0;
	MetalPtr<MTL::Buffer> polyVertexBuffer_;
	size_t polyVertexBufferSize_ = 0;
	size_t polyVertexCountGPU_ = 0;
	MetalPtr<MTL::Buffer> lightBuffer_;
	size_t lightBufferSize_ = 0;
	size_t lightCountGPU_ = 0;
	struct StagePipelineEntry {
		MetalShaderResource::MetalPipelineKey key;
		MetalPtr<MTL::RenderPipelineState> pipeline;
		MetalPtr<MTL::DepthStencilState> depthState;

		StagePipelineEntry() = default;
		StagePipelineEntry(StagePipelineEntry&&) = default;
		StagePipelineEntry& operator=(StagePipelineEntry&&) = default;
		StagePipelineEntry(const StagePipelineEntry&) = delete;
		StagePipelineEntry& operator=(const StagePipelineEntry&) = delete;
	};

	struct MetalPipelineKeyHash {
		std::size_t operator()(const MetalShaderResource::MetalPipelineKey& key) const noexcept;
	};


	MetalPtr<MTL::SamplerState> sceneSampler_;
	MetalPtr<MTL::Library> sceneLibrary_;
	MetalPtr<MTL::Function> sceneVertexFunction_;
	MetalPtr<MTL::Function> sceneFragmentFunction_;
	MetalPtr<MTL::VertexDescriptor> sceneVertexDescriptor_;
	MetalPtr<MTL::Texture> depthTexture_;
	int depthTextureWidth_ = 0;
	int depthTextureHeight_ = 0;
	std::unordered_map<MetalShaderResource::MetalPipelineKey, StagePipelineEntry, MetalPipelineKeyHash> stagePipelineCache_;


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
	MetalPtr<MTL::DepthStencilState> depthState2D_;
	float currentColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	bool commandsRegistered_ = false;
	int processedFrameId_ = 0;
	stereoFrame_t stereoFrame_ = STEREO_CENTER;
	bool sceneReady_ = false;
	bool sceneDispatched_ = false;
	bool polyVertexBufferDirty_ = false;
	bool lightBufferDirty_ = false;
	bool debugPolyEnabled_ = false;
	std::string debugPolyShaderName_;
	qhandle_t debugPolyShaderHandle_ = 0;
	float debugPolySize_ = 256.0f;
	float debugPolyDistance_ = 256.0f;
	bool worldLoaded_ = false;
	std::string worldName_;
	std::vector<MetalPolyVertex> worldVertexTemplate_;
	std::vector<ScenePolyPacket> worldPacketTemplate_;
	std::vector<qhandle_t> worldLightmapHandles_;
	std::vector<std::string> registeredModels_;
	std::unordered_map<std::string, qhandle_t> modelLookup_;
	std::vector<std::string> registeredSkins_;
	std::unordered_map<std::string, qhandle_t> skinLookup_;
	std::vector<MetalShaderResource> shaderResources_;
	std::unordered_map<std::string, qhandle_t> shaderLookup_;

	bool initializeWindow(int& width, int& height, qboolean& fullscreen);
	bool createPipeline(MTL::Texture* drawableTexture);
	bool create2DPipeline();
	bool ensureDepthTexture(int width, int height);
	void processPendingUploads();
	void fillConfigDefaults(int width, int height, qboolean fullscreen);
	void publishConfig();
	void registerConsoleCommands();
	void unregisterConsoleCommands();
	void configureDebugPoly(bool enable, const char* shaderName, float size, float distance);
	void appendDebugPoly(const MetalSceneState& scene);
	qhandle_t ensureDebugPolyShaderHandle();
	bool loadWorldMap(const char* name);
	void unloadWorldMap();
	void appendWorldGeometry();
	void printGfxInfo();
	void printImageList();
	void updateCamera(const MetalSceneState& scene);
	void buildViewMatrix(SceneCamera& camera);
	void buildProjectionMatrix(SceneCamera& camera);
	float computeStereoOffset(stereoFrame_t frame, float zProj) const;
	float pickFarPlane(const MetalSceneState& scene) const;
	void ensureSceneRendered();
	void renderScenePackets();
	void configureSceneViewport(const refdef_t& refdef);
	bool uploadSceneUniforms();
	bool uploadPolyVertexBuffer();
	bool uploadLightBuffer();
	bool ensureSceneShaderResources();
	StagePipelineEntry* getStagePipeline(const MetalShaderResource::MetalPipelineKey& key);
	void resetStagePipelineCache();
	bool drawPolyPackets();
	void encodeEntityCommands(SceneDispatchSummary& summary);
	void encodePolyCommands(SceneDispatchSummary& summary);
	void encodeLightCommands(SceneDispatchSummary& summary);
	TextureManager* ensureTextureManager();
	void resetShaderCaches();
	void ensureScriptHasStages(MetalShaderResource& resource);
	void loadShaderImages(MetalShaderResource& resource);
	void buildShaderStageRuntime(MetalShaderResource& resource);
	MetalShaderResource* getShaderResource(qhandle_t handle);
	const MetalShaderResource* getShaderResource(qhandle_t handle) const;
	const MetalShaderResource::MetalShaderStageRuntime* getShaderStageRuntime(const MetalShaderResource& resource, size_t stageIndex) const;
	const MetalShaderResource::MetalShaderStageRuntime* getPrimaryStageRuntime(const MetalShaderResource& resource) const;
	qhandle_t selectStageImage(MetalShaderResource& resource,
	                          const MetalShaderResource::MetalShaderStageRuntime* runtime,
	                          float timeSeconds,
	                          qhandle_t lightmapHandle = 0);
	qhandle_t resolveImageHandle(MetalShaderResource& resource, size_t imageIndex);
	void updatePrimaryImageHandle(MetalShaderResource& resource);
	bool assetExists(const char* path) const;
	static MTL::BlendFactor ToMetalBlendFactor(MetalBlendFactor factor);
};

//=============================================================================
// Implementation
//=============================================================================

bool MetalRenderer::initialize(refimport_t imports) {
	ri_ = imports;
	ri = imports;
	registerConsoleCommands();
	r_znear_ = ri_.Cvar_Get("r_znear", "4", CVAR_CHEAT);
	if (ri_.Cvar_CheckRange) {
		ri_.Cvar_CheckRange(r_znear_, 0.001f, 200.0f, qfalse);
	}
	r_zproj_ = ri_.Cvar_Get("r_zproj", "64", CVAR_ARCHIVE);
	r_stereoSeparation_ = ri_.Cvar_Get("r_stereoSeparation", "64", CVAR_ARCHIVE);
	r_metalLogCalls_ = ri_.Cvar_Get("r_metalLogCalls", "0", CVAR_TEMP);
	r_swapInterval_ = ri_.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE | CVAR_LATCH);
	resetShaderCaches();
	// TextureManager will be created when device is available
	return true;
}

void MetalRenderer::shutdown(qboolean destroyWindow) {
	unloadWorldMap();
	unregisterConsoleCommands();
	// RAII handles Metal object cleanup automatically
	textureManager_.reset();
	sampler2D_.reset();
	pipeline2D_.reset();
	sampler_.reset();
	pipeline_.reset();
	sceneSampler_.reset();
	sceneVertexDescriptor_.reset();
	sceneFragmentFunction_.reset();
	sceneVertexFunction_.reset();
	sceneLibrary_.reset();
	depthTexture_.reset();
	depthTextureWidth_ = depthTextureHeight_ = 0;
	stagePipelineCache_.clear();
	
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

	shaderLookup_.clear();
	shaderResources_.clear();

	if (destroyWindow) {
		SDLMetal_Shutdown(destroyWindow);
		layer_ = nullptr;
		frameCinematicTexture_ = nullptr;
	}
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

void MetalRenderer::publishConfig() {
	glConfig = config_;
}

void MetalRenderer::registerConsoleCommands() {
	if (commandsRegistered_ || !ri_.Cmd_AddCommand) {
		return;
	}

	ri_.Cmd_AddCommand("gfxinfo", Metal_GfxInfo_f);
	ri_.Cmd_AddCommand("imagelist", Metal_ImageList_f);
	ri_.Cmd_AddCommand("metal_debug_poly", Metal_DebugPoly_f);
	commandsRegistered_ = true;
}

void MetalRenderer::unregisterConsoleCommands() {
	if (!commandsRegistered_ || !ri_.Cmd_RemoveCommand) {
		return;
	}

	ri_.Cmd_RemoveCommand("gfxinfo");
	ri_.Cmd_RemoveCommand("imagelist");
	ri_.Cmd_RemoveCommand("metal_debug_poly");
	commandsRegistered_ = false;
}

bool MetalRenderer::callLoggingEnabled() const {
	return r_metalLogCalls_ && r_metalLogCalls_->integer != 0;
}

void MetalRenderer::logRendererCall(const char* name) const {
	if (!callLoggingEnabled() || !ri_.Printf || !name) {
		return;
	}

	const int frameTag = processedFrameId_;
	ri_.Printf(PRINT_DEVELOPER, "MetalCall[%d]: %s\n", frameTag, name);
}

void MetalRenderer::printGfxInfo() {
	if (!ri_.Printf) {
		return;
	}

	const char* fsMode = config_.isFullscreen ? "fullscreen" : "windowed";
	const char* memInfo = "none";
	switch (glRefConfig.memInfo) {
		case MI_NVX: memInfo = "NVX"; break;
		case MI_ATI: memInfo = "ATI"; break;
		default: break;
	}

	ri_.Printf(PRINT_ALL, "\nMetal Renderer Information\n");
	ri_.Printf(PRINT_ALL, " Vendor   : %s\n", config_.vendor_string);
	ri_.Printf(PRINT_ALL, " Renderer : %s\n", config_.renderer_string);
	ri_.Printf(PRINT_ALL, " Version  : %s\n", config_.version_string);
	ri_.Printf(PRINT_ALL, " Mode     : %dx%d %s\n", config_.vidWidth, config_.vidHeight, fsMode);
	ri_.Printf(PRINT_ALL, " Texture  : maxSize=%d units=%d\n", config_.maxTextureSize, config_.numTextureUnits);
	ri_.Printf(PRINT_ALL, " Depth    : color=%d depth=%d stencil=%d\n",
	           config_.colorBits, config_.depthBits, config_.stencilBits);
	ri_.Printf(PRINT_ALL, " Gamma HW : %d\n", config_.deviceSupportsGamma);

	ri_.Printf(PRINT_ALL, " Metal capabilities:\n");
	ri_.Printf(PRINT_ALL, "  Bones=%d  MaxAttribs=%d  memInfo=%s\n",
	           glRefConfig.glslMaxAnimatedBones,
	           glRefConfig.maxVertexAttribs,
	           memInfo);
	ri_.Printf(PRINT_ALL, "  FBO=%d MSAA=%d Blit=%d SeamlessCube=%d\n",
	           glRefConfig.framebufferObject,
	           glRefConfig.framebufferMultisample,
	           glRefConfig.framebufferBlit,
	           glRefConfig.seamlessCubeMap);
	ri_.Printf(PRINT_ALL, "  DepthClamp=%d ShadowSamplers=%d Derivatives=%d\n",
	           glRefConfig.depthClamp,
	           glRefConfig.shadowSamplers,
	           glRefConfig.standardDerivatives);
	ri_.Printf(PRINT_ALL, "  TextureFloat=%d CompressionMask=0x%02x\n",
	           glRefConfig.textureFloat,
	           static_cast<int>(glRefConfig.textureCompression));
}

void MetalRenderer::printImageList() {
	if (!ri_.Printf) {
		return;
	}

	if (!textureManager_) {
		ri_.Printf(PRINT_ALL, "Metal: texture system is not initialized.\n");
		return;
	}

	textureManager_->debugListTextures();
}

void MetalRenderer::configureDebugPoly(bool enable, const char* shaderName, float size, float distance) {
	if (!enable) {
		debugPolyEnabled_ = false;
		debugPolyShaderHandle_ = 0;
		debugPolyShaderName_.clear();
		if (ri_.Printf) {
			ri_.Printf(PRINT_ALL, "Metal: debug poly disabled\n");
		}
		return;
	}

	if (!shaderName || !shaderName[0]) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: debug poly requires a shader name\n");
		}
		return;
	}

	debugPolySize_ = std::max(4.0f, size);
	debugPolyDistance_ = std::max(4.0f, distance);
	debugPolyShaderName_ = shaderName;
	debugPolyShaderHandle_ = registerShader(shaderName, qtrue);
	if (debugPolyShaderHandle_ <= 0) {
		debugPolyEnabled_ = false;
		debugPolyShaderName_.clear();
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: failed to register debug poly shader '%s'\n", shaderName);
		}
		return;
	}

	debugPolyEnabled_ = true;
	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: debug poly enabled using '%s' size=%.1f distance=%.1f\n",
		           shaderName,
		           debugPolySize_,
		           debugPolyDistance_);
	}
}

qhandle_t MetalRenderer::ensureDebugPolyShaderHandle() {
	if (!debugPolyEnabled_ || debugPolyShaderName_.empty()) {
		return 0;
	}
	if (debugPolyShaderHandle_ > 0) {
		return debugPolyShaderHandle_;
	}
	debugPolyShaderHandle_ = registerShader(debugPolyShaderName_.c_str(), qtrue);
	return debugPolyShaderHandle_;
}

void MetalRenderer::appendDebugPoly(const MetalSceneState& scene) {
	if (!debugPolyEnabled_ || !scene.refdefValid) {
		return;
	}

	qhandle_t shaderHandle = ensureDebugPolyShaderHandle();
	if (shaderHandle <= 0) {
		return;
	}

	const float halfSize = std::max(2.0f, debugPolySize_ * 0.5f);
	const float distance = std::max(1.0f, debugPolyDistance_);
	vec3_t center;
	VectorMA(scene.refdef.vieworg, distance, scene.refdef.viewaxis[0], center);
	vec3_t right;
	vec3_t up;
	VectorScale(scene.refdef.viewaxis[1], halfSize, right);
	VectorScale(scene.refdef.viewaxis[2], halfSize, up);

	vec3_t topCenter;
	vec3_t bottomCenter;
	VectorAdd(center, up, topCenter);
	VectorSubtract(center, up, bottomCenter);

	vec3_t corners[4];
	VectorAdd(topCenter, right, corners[0]);      // top-right
	VectorSubtract(topCenter, right, corners[1]); // top-left
	VectorAdd(bottomCenter, right, corners[2]);   // bottom-right
	VectorSubtract(bottomCenter, right, corners[3]); // bottom-left

	MetalPolyVertex quad[4];
	VectorCopy(corners[1], quad[0].xyz);
	quad[0].st[0] = 0.0f;
	quad[0].st[1] = 0.0f;
	quad[0].lightmap[0] = 0.0f;
	quad[0].lightmap[1] = 0.0f;
	VectorCopy(corners[0], quad[1].xyz);
	quad[1].st[0] = 1.0f;
	quad[1].st[1] = 0.0f;
	quad[1].lightmap[0] = 1.0f;
	quad[1].lightmap[1] = 0.0f;
	VectorCopy(corners[3], quad[2].xyz);
	quad[2].st[0] = 0.0f;
	quad[2].st[1] = 1.0f;
	quad[2].lightmap[0] = 0.0f;
	quad[2].lightmap[1] = 1.0f;
	VectorCopy(corners[2], quad[3].xyz);
	quad[3].st[0] = 1.0f;
	quad[3].st[1] = 1.0f;
	quad[3].lightmap[0] = 1.0f;
	quad[3].lightmap[1] = 1.0f;
	for (MetalPolyVertex& vert : quad) {
		vert.modulate[0] = 255;
		vert.modulate[1] = 255;
		vert.modulate[2] = 255;
		vert.modulate[3] = 255;
	}

	const int firstVertex = static_cast<int>(polyVertices_.size());
	polyVertices_.push_back(quad[0]);
	polyVertices_.push_back(quad[2]);
	polyVertices_.push_back(quad[3]);
	polyVertices_.push_back(quad[0]);
	polyVertices_.push_back(quad[3]);
	polyVertices_.push_back(quad[1]);

	ScenePolyPacket packet;
	packet.shader = shaderHandle;
	packet.firstVertex = firstVertex;
	packet.vertexCount = 6;
	packet.primitive = MTL::PrimitiveTypeTriangle;
	polyPackets_.push_back(packet);
}

void MetalRenderer::appendWorldGeometry() {
	if (!worldLoaded_ || worldVertexTemplate_.empty() || worldPacketTemplate_.empty()) {
		return;
	}

	const size_t baseVertex = polyVertices_.size();
	polyVertices_.insert(polyVertices_.end(), worldVertexTemplate_.begin(), worldVertexTemplate_.end());
	for (const ScenePolyPacket& templatePacket : worldPacketTemplate_) {
		ScenePolyPacket packet = templatePacket;
		packet.firstVertex += static_cast<int>(baseVertex);
		polyPackets_.push_back(packet);
	}
}

bool MetalRenderer::loadWorldMap(const char* name) {
	if (!name || !name[0]) {
		unloadWorldMap();
		return false;
	}

	std::string requestedName(name);
	if (requestedName.find('.') == std::string::npos) {
		requestedName = "maps/" + requestedName + ".bsp";
	}
	if (worldLoaded_ && worldName_ == requestedName) {
		return true;
	}

	unloadWorldMap();
	byte* buffer = nullptr;
	const int fileLen = ri_.FS_ReadFile(requestedName.c_str(), reinterpret_cast<void**>(&buffer));
	if (fileLen <= 0 || !buffer) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: failed to load world '%s'\n", requestedName.c_str());
		}
		return false;
	}
	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: loadWorldMap '%s' - file loaded, size %d\n", requestedName.c_str(), fileLen);
		ri_.Printf(PRINT_ALL, "Metal: sizeof(drawVert_t) = %zu\n", sizeof(drawVert_t));
	}

	const dheader_t* header = reinterpret_cast<const dheader_t*>(buffer);
	if (LittleLong(header->ident) != BSP_IDENT) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: '%s' is not a valid BSP\n", requestedName.c_str());
		}
		ri_.FS_FreeFile(buffer);
		return false;
	}
	if (LittleLong(header->version) != BSP_VERSION) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: '%s' has unsupported BSP version\n", requestedName.c_str());
		}
		ri_.FS_FreeFile(buffer);
		return false;
	}

	auto getLumpRange = [&](int lumpIndex, int& count, size_t elementSize) -> const byte* {
		const lump_t& lump = header->lumps[lumpIndex];
		const int offset = LittleLong(lump.fileofs);
		const int length = LittleLong(lump.filelen);
		if (offset < 0 || length < 0 || offset + length > fileLen || elementSize == 0) {
			count = 0;
			return nullptr;
		}
		count = length / static_cast<int>(elementSize);
		if (count <= 0) {
			return nullptr;
		}
		return reinterpret_cast<const byte*>(buffer + offset);
	};

	int shaderCount = 0;
	const dshader_t* shaderTable = reinterpret_cast<const dshader_t*>(getLumpRange(LUMP_SHADERS, shaderCount, sizeof(dshader_t)));
	int vertexCount = 0;
	const drawVert_t* drawVerts = reinterpret_cast<const drawVert_t*>(getLumpRange(LUMP_DRAWVERTS, vertexCount, sizeof(drawVert_t)));
	int indexCount = 0;
	const int* drawIndexes = reinterpret_cast<const int*>(getLumpRange(LUMP_DRAWINDEXES, indexCount, sizeof(int)));
	int surfaceCount = 0;
	const dsurface_t* surfaces = reinterpret_cast<const dsurface_t*>(getLumpRange(LUMP_SURFACES, surfaceCount, sizeof(dsurface_t)));
	int lightmapCount = 0;
	const byte* lightmapData = getLumpRange(LUMP_LIGHTMAPS, lightmapCount, kBspLightmapBytes);

	if (!shaderTable || !drawVerts || !drawIndexes || !surfaces) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: '%s' missing required BSP lumps\n", requestedName.c_str());
		}
		ri_.FS_FreeFile(buffer);
		return false;
	}

	TextureManager* textureManager = ensureTextureManager();
	if (!textureManager) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: texture manager unavailable for world '%s'\n", requestedName.c_str());
		}
		ri_.FS_FreeFile(buffer);
		return false;
	}

	worldLightmapHandles_.clear();
	if (lightmapData && lightmapCount > 0) {
		worldLightmapHandles_.reserve(lightmapCount);
		const int pixelCount = kBspLightmapWidth * kBspLightmapHeight;
		std::vector<byte> rgba(static_cast<size_t>(pixelCount) * 4u);
		for (int lm = 0; lm < lightmapCount; ++lm) {
			const byte* src = lightmapData + static_cast<size_t>(lm) * kBspLightmapBytes;
			for (int pix = 0; pix < pixelCount; ++pix) {
				rgba[static_cast<size_t>(pix) * 4 + 0] = src[static_cast<size_t>(pix) * 3 + 0];
				rgba[static_cast<size_t>(pix) * 4 + 1] = src[static_cast<size_t>(pix) * 3 + 1];
				rgba[static_cast<size_t>(pix) * 4 + 2] = src[static_cast<size_t>(pix) * 3 + 2];
				rgba[static_cast<size_t>(pix) * 4 + 3] = 255;
			}
			char texName[MAX_QPATH];
			std::snprintf(texName, sizeof(texName), "%s/lightmap_%d", requestedName.c_str(), lm);
			qhandle_t handle = textureManager->registerRawImage(texName, rgba.data(), kBspLightmapWidth, kBspLightmapHeight, false);
			worldLightmapHandles_.push_back(handle);
		}
	}

	std::vector<qhandle_t> shaderHandles(shaderCount, -1);
	auto resolveShaderHandle = [&](int shaderNum) -> qhandle_t {
		if (shaderNum < 0 || shaderNum >= shaderCount) {
			return 0;
		}
		qhandle_t& handle = shaderHandles[shaderNum];
		if (handle != -1) {
			return handle;
		}
		const dshader_t& shader = shaderTable[shaderNum];
		std::string shaderName(shader.shader);
		if (shaderName.empty()) {
			shaderName = "white";
		}
		handle = registerShader(shaderName.c_str(), true);
		return handle;
	};

	worldVertexTemplate_.clear();
	worldPacketTemplate_.clear();
	worldVertexTemplate_.reserve(static_cast<size_t>(vertexCount) * 2);
	worldPacketTemplate_.reserve(static_cast<size_t>(surfaceCount));

	for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex) {
		const dsurface_t& ds = surfaces[surfaceIndex];
		const int surfaceType = LittleLong(ds.surfaceType);
		
		// Skip unsupported surface types
		if (surfaceType != MST_PLANAR && surfaceType != MST_TRIANGLE_SOUP && surfaceType != MST_PATCH) {
			continue;
		}

		const int shaderNum = LittleLong(ds.shaderNum);
		qhandle_t shaderHandle = resolveShaderHandle(shaderNum);
		if (shaderHandle <= 0) {
			continue;
		}

		const size_t startVertex = worldVertexTemplate_.size();

		if (surfaceType == MST_PATCH) {
			// Handle curved patch surfaces (Bezier patches)
			const int patchWidth = LittleLong(ds.patchWidth);
			const int patchHeight = LittleLong(ds.patchHeight);
			const int firstVert = LittleLong(ds.firstVert);
			const int numVerts = LittleLong(ds.numVerts);

			// Validate patch dimensions - must be odd and at least 3
			if (patchWidth < 3 || patchHeight < 3 || (patchWidth & 1) == 0 || (patchHeight & 1) == 0) {
				continue;
			}
			if (firstVert < 0 || numVerts < patchWidth * patchHeight || firstVert + numVerts > vertexCount) {
				continue;
			}

			// Tessellate the patch into triangles
			std::vector<MetalPolyVertex> patchVerts;
			TessellateBezierPatch(&drawVerts[firstVert], patchWidth, patchHeight, patchVerts, PATCH_SUBDIVISIONS);

			// Add tessellated vertices to the world buffer
			for (const auto& v : patchVerts) {
				worldVertexTemplate_.push_back(v);
			}
		} else {
			// Handle MST_PLANAR and MST_TRIANGLE_SOUP (indexed geometry)
			const int firstVert = LittleLong(ds.firstVert);
			const int firstIndex = LittleLong(ds.firstIndex);
			const int numIndexes = LittleLong(ds.numIndexes);

			if (firstIndex < 0 || numIndexes < 3 || firstIndex + numIndexes > indexCount) {
				continue;
			}

			const int* surfIndexes = drawIndexes + firstIndex;
			for (int i = 0; i + 2 < numIndexes; i += 3) {
				// BSP indices are relative to firstVert, so add firstVert to get absolute index
				const int idx0 = firstVert + LittleLong(surfIndexes[i + 0]);
				const int idx1 = firstVert + LittleLong(surfIndexes[i + 1]);
				const int idx2 = firstVert + LittleLong(surfIndexes[i + 2]);
				if (idx0 < 0 || idx0 >= vertexCount || idx1 < 0 || idx1 >= vertexCount || idx2 < 0 || idx2 >= vertexCount) {
					continue;
				}
				worldVertexTemplate_.push_back(ConvertDrawVert(drawVerts[idx0]));
				worldVertexTemplate_.push_back(ConvertDrawVert(drawVerts[idx1]));
				worldVertexTemplate_.push_back(ConvertDrawVert(drawVerts[idx2]));
			}
		}

		const size_t addedVerts = worldVertexTemplate_.size() - startVertex;
		if (addedVerts == 0) {
			continue;
		}

		if (ri_.Printf && startVertex == 0 && addedVerts >= 3) {
			const MetalPolyVertex& v0 = worldVertexTemplate_[0];
			const MetalPolyVertex& v1 = worldVertexTemplate_[1];
			const MetalPolyVertex& v2 = worldVertexTemplate_[2];
			ri_.Printf(PRINT_DEVELOPER, "Metal: World Vert 0: xyz(%.2f %.2f %.2f) st(%.2f %.2f)\n",
				v0.xyz[0], v0.xyz[1], v0.xyz[2], v0.st[0], v0.st[1]);
			ri_.Printf(PRINT_DEVELOPER, "Metal: World Vert 1: xyz(%.2f %.2f %.2f) st(%.2f %.2f)\n",
				v1.xyz[0], v1.xyz[1], v1.xyz[2], v1.st[0], v1.st[1]);
			ri_.Printf(PRINT_DEVELOPER, "Metal: World Vert 2: xyz(%.2f %.2f %.2f) st(%.2f %.2f)\n",
				v2.xyz[0], v2.xyz[1], v2.xyz[2], v2.st[0], v2.st[1]);
		}

		ScenePolyPacket packet;
		packet.shader = shaderHandle;
		packet.firstVertex = static_cast<int>(startVertex);
		packet.vertexCount = static_cast<int>(addedVerts);
		packet.primitive = MTL::PrimitiveTypeTriangle;
		worldPacketTemplate_.push_back(packet);
	}

	ri_.FS_FreeFile(buffer);
	worldLoaded_ = !worldPacketTemplate_.empty();
	if (worldLoaded_) {
		worldName_ = requestedName;
		if (ri_.Printf) {
			ri_.Printf(PRINT_ALL, "Metal: loaded world '%s' (%zu surfaces, %zu verts)\n",
			           worldName_.c_str(),
			           worldPacketTemplate_.size(),
			           worldVertexTemplate_.size());
		}
	} else if (ri_.Printf) {
		ri_.Printf(PRINT_WARNING, "Metal: no supported surfaces found in '%s'\n", requestedName.c_str());
	}
	return worldLoaded_;
}

void MetalRenderer::unloadWorldMap() {
	worldLoaded_ = false;
	worldName_.clear();
	worldVertexTemplate_.clear();
	worldPacketTemplate_.clear();
	worldLightmapHandles_.clear();
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
		const bool vsyncEnabled = r_swapInterval_ && r_swapInterval_->integer != 0;
		layer_->setDisplaySyncEnabled(vsyncEnabled);
		layer_->setMaximumDrawableCount(3);
		
		CGSize drawableSize;
		drawableSize.width = static_cast<CGFloat>(width);
		drawableSize.height = static_cast<CGFloat>(height);
		layer_->setDrawableSize(drawableSize);
	}

	fillConfigDefaults(width, height, fullscreen);
	Metal_InitExtensions(device_.get(), &config_, &ri_);
	publishConfig();
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
	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

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

	if (!depthState2D_) {
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthWriteEnabled(false);
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
		depthState2D_.reset(device_->newDepthStencilState(depthDesc));
		depthDesc->release();
	}

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
	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
	
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

	// Create depth state for 2D (disable depth test/write)
	MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
	depthDesc->setDepthWriteEnabled(false);
	depthDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
	depthState2D_.reset(device_->newDepthStencilState(depthDesc));
	depthDesc->release();

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

bool MetalRenderer::ensureDepthTexture(int width, int height) {
	if (!device_ || width <= 0 || height <= 0) {
		depthTexture_.reset();
		depthTextureWidth_ = 0;
		depthTextureHeight_ = 0;
		return false;
	}

	if (depthTexture_ && depthTextureWidth_ == width && depthTextureHeight_ == height) {
		return true;
	}

	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
		MTL::PixelFormatDepth32Float, width, height, false);
	desc->setStorageMode(MTL::StorageModePrivate);
	desc->setUsage(MTL::TextureUsageRenderTarget);

	depthTexture_.reset(device_->newTexture(desc));
	depthTextureWidth_ = depthTexture_ ? width : 0;
	depthTextureHeight_ = depthTexture_ ? height : 0;
	desc->release();

	if (!depthTexture_) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: failed to allocate depth buffer %dx%d\n", width, height);
		}
		return false;
	}

	return true;
}

qhandle_t MetalRenderer::registerShader(const char* name, bool mipmap) {
	if (!name || !name[0]) {
		return 0;
	}

	if (shaderResources_.empty()) {
		resetShaderCaches();
	}

	const std::string normalized = MetalNormalizeShaderName(name);
	const auto existingIt = shaderLookup_.find(normalized);
	if (existingIt != shaderLookup_.end()) {
		return existingIt->second;
	}

	MetalShaderResource resource;
	resource.name = normalized;
	resource.mipmap = mipmap;

	MetalShaderScriptInfo info;
	if (MetalShaderScriptGetInfo(normalized, info)) {
		resource.hasScript = true;
		resource.script = info;
	} else {
		resource.hasScript = false;
		resource.script.imagePaths.push_back(normalized);
		resource.script.forceOpaque = qtrue;
	}

	if (resource.script.imagePaths.empty()) {
		resource.script.imagePaths.push_back("white");
	}

	ensureScriptHasStages(resource);
	loadShaderImages(resource);
	buildShaderStageRuntime(resource);
	const qhandle_t handle = static_cast<qhandle_t>(shaderResources_.size());
	shaderResources_.push_back(std::move(resource));
	shaderLookup_[normalized] = handle;
	return handle;
}

qhandle_t MetalRenderer::registerModel(const char* name) {
	if (!name || !name[0]) {
		return 0;
	}

	const std::string key(name);
	auto it = modelLookup_.find(key);
	if (it != modelLookup_.end()) {
		return it->second;
	}

	if (!assetExists(name)) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: model '%s' missing (stub register)\n", name);
		}
		return 0;
	}

	const qhandle_t handle = static_cast<qhandle_t>(registeredModels_.size() + 1);
	registeredModels_.push_back(key);
	modelLookup_[key] = handle;

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: registered model '%s' as handle %d (placeholder)\n", name, handle);
	}

	return handle;
}

qhandle_t MetalRenderer::registerSkin(const char* name) {
	if (!name || !name[0]) {
		return 0;
	}

	const std::string key(name);
	auto it = skinLookup_.find(key);
	if (it != skinLookup_.end()) {
		return it->second;
	}

	if (!assetExists(name)) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: skin '%s' missing (stub register)\n", name);
		}
		return 0;
	}

	const qhandle_t handle = static_cast<qhandle_t>(registeredSkins_.size() + 1);
	registeredSkins_.push_back(key);
	skinLookup_[key] = handle;

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: registered skin '%s' as handle %d (placeholder)\n", name, handle);
	}

	return handle;
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
	// If we have a pending 3D scene that hasn't been submitted for this frame, submit it now.
	// This ensures 3D draws before 2D.
	submitScene();
	ensureSceneRendered();
	
	if (!device_ || !currentRenderEncoder_) {
		return;
	}

	TextureManager* textureManager = ensureTextureManager();
	if (!textureManager) {
		return;
	}

	// Ensure 2D pipeline exists
	if (!create2DPipeline()) {
		return;
	}

	MetalShaderResource* shaderResource = getShaderResource(shader);
	if (!shaderResource) {
		shaderResource = getShaderResource(0);
	}
	if (!shaderResource) {
		return;
	}

	const auto* stageRuntime = getPrimaryStageRuntime(*shaderResource);
	float timeSeconds = 0.0f;
	if (ri_.Milliseconds) {
		timeSeconds = static_cast<float>(ri_.Milliseconds()) * 0.001f;
	}
	qhandle_t imageHandle = selectStageImage(*shaderResource, stageRuntime, timeSeconds);
	MTL::Texture* texture = textureManager->getTexture(imageHandle);
	if (!texture) {
		texture = textureManager->getTexture(0);
		if (!texture) {
			return;
		}
	}

	// Set pipeline state
	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipeline2D_.get());
	if (depthState2D_) {
		currentRenderEncoder_->setDepthStencilState(depthState2D_.get());
	}
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

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
	instance.rect[1] = ndcY;         // Top Y (draw downwards)
	instance.rect[2] = ndcW;
	instance.rect[3] = -ndcH;        // Negative height to draw down from top
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
	MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, texture);
	MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler2D_.get());
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

	// Draw triangle strip (4 vertices = 1 quad)
	currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, NS::UInteger(0), NS::UInteger(4), NS::UInteger(1));
}

void MetalRenderer::beginFrame(stereoFrame_t stereoFrame) {
	MetalScene_BeginFrame();
	stereoFrame_ = stereoFrame;
	sceneReady_ = false;
	sceneDispatched_ = false;

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

	// Refresh layer pointer in case SDL recreated it
	if (!layer_) {
		layer_ = reinterpret_cast<CA::MetalLayer*>(SDLMetal_GetLayer());
	}

	// Start frame: acquire drawable and create command buffer
	if (!layer_ || !commandQueue_) {
		return;
	}

	currentDrawable_ = layer_->nextDrawable();
	if (!currentDrawable_) {
		return;
	}

	currentCommandBuffer_ = commandQueue_->commandBuffer();
	if (!currentCommandBuffer_) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: beginFrame - failed to create command buffer\n");
		}
		return;
	}

	MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
	rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
	rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
	rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
	rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
	ensureDepthTexture(config_.vidWidth, config_.vidHeight);
	if (depthTexture_) {
		MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = rpd->depthAttachment();
		depthAttachment->setTexture(depthTexture_.get());
		depthAttachment->setLoadAction(MTL::LoadActionClear);
		depthAttachment->setClearDepth(1.0);
		depthAttachment->setStoreAction(MTL::StoreActionDontCare);
	}

	currentRenderEncoder_ = currentCommandBuffer_->renderCommandEncoder(rpd);
	rpd->release();

	if (!currentRenderEncoder_) {
		return;
	}

	MetalStateCache::Instance().resetEncoder(currentRenderEncoder_);
	currentRenderEncoder_->setFrontFacingWinding(MTL::WindingCounterClockwise);
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);
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

	submitScene();
	ensureSceneRendered();

	if (currentRenderEncoder_) {
		currentRenderEncoder_->endEncoding();
		currentRenderEncoder_->release();
		MetalStateCache::Instance().resetEncoder(nullptr);
		currentRenderEncoder_ = nullptr;
	}

	if (currentCommandBuffer_ && currentDrawable_) {
		currentCommandBuffer_->presentDrawable(currentDrawable_);
		currentCommandBuffer_->commit();
		
		currentDrawable_->release();
	}

	currentCommandBuffer_ = nullptr;
	currentDrawable_ = nullptr;
	MetalScene_EndFrame();
}

void MetalRenderer::submitScene() {
	MetalSceneState& state = MetalScene_MutableState();
	if (processedFrameId_ == state.frameId) {
		// if (ri_.Printf) ri_.Printf(PRINT_ALL, "Metal: submitScene - already processed frame %d\n", state.frameId);
		return;
	}
	sceneReady_ = false;
	sceneDispatched_ = false;

	if (!state.refdefValid) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: submitScene - refdefValid=FALSE, clearing scene state\n");
		}
		// Refdef invalid, clear scene state
		drawPackets_.clear();
		polyPackets_.clear();
		polyVertices_.clear();
		lightPackets_.clear();
		sceneCamera_.valid = false;
		polyVertexBufferDirty_ = true;
		lightBufferDirty_ = true;
		return;
	}

	processedFrameId_ = state.frameId;

	sceneStats_.entities = 0;
	sceneStats_.polys = 0;
	sceneStats_.lights = 0;

	processScene(state);
	sceneReady_ = true;
	sceneDispatched_ = false;
}

void MetalRenderer::processScene(const MetalSceneState& scene) {
	drawPackets_.clear();
	polyPackets_.clear();
	polyVertices_.clear();
	lightPackets_.clear();
	drawPackets_.reserve(scene.numEntities);
	polyPackets_.reserve(scene.numPolys);
	polyVertices_.reserve(static_cast<size_t>(scene.numPolyVerts) * 3u);
	lightPackets_.reserve(scene.numLights);

	processEntities(scene);
	processPolys(scene);
	processLights(scene);
	updateCamera(scene);
	polyVertexBufferDirty_ = true;
	lightBufferDirty_ = true;

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "MetalScene: processed %d entities, %d polys, %d lights\n",
		           sceneStats_.entities,
		           sceneStats_.polys,
		           sceneStats_.lights);
	}
}

void MetalRenderer::processEntities(const MetalSceneState& scene) {
	for (int i = 0; i < scene.numEntities; ++i) {
		SceneDrawPacket packet;
		packet.entity = scene.entities[i];
		drawPackets_.push_back(packet);
	}
	sceneStats_.entities = static_cast<int>(drawPackets_.size());
}

void MetalRenderer::processPolys(const MetalSceneState& scene) {
	appendWorldGeometry();
	for (int i = 0; i < scene.numPolys; ++i) {
		const MetalScenePolyRange& range = scene.polys[i];
		if (range.numVerts <= 0) {
			continue;
		}

		if (range.firstVertex < 0 || range.firstVertex >= scene.numPolyVerts) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_DEVELOPER, "MetalScene: skipping poly with invalid vertex range (%d)\n", i);
			}
			continue;
		}

		const int available = scene.numPolyVerts - range.firstVertex;
		const int fanVerts = std::min(range.numVerts, available);
		if (fanVerts < 3) {
			continue;
		}

		const polyVert_t* src = scene.polyVerts.data() + range.firstVertex;
		const int firstVertex = static_cast<int>(polyVertices_.size());
		for (int tri = 0; tri < fanVerts - 2; ++tri) {
			polyVertices_.push_back(ConvertPolyVert(src[0]));
			polyVertices_.push_back(ConvertPolyVert(src[tri + 1]));
			polyVertices_.push_back(ConvertPolyVert(src[tri + 2]));
		}

		ScenePolyPacket packet;
		packet.shader = range.shader;
		packet.firstVertex = firstVertex;
		packet.vertexCount = (fanVerts - 2) * 3;
		packet.primitive = MTL::PrimitiveTypeTriangle;
		polyPackets_.push_back(packet);
	}

	appendDebugPoly(scene);
	sceneStats_.polys = static_cast<int>(polyPackets_.size());
}

void MetalRenderer::processLights(const MetalSceneState& scene) {
	for (int i = 0; i < scene.numLights; ++i) {
		SceneLightPacket packet;
		packet.light = scene.lights[i];
		lightPackets_.push_back(packet);
	}

	sceneStats_.lights = static_cast<int>(lightPackets_.size());
}

void MetalRenderer::ensureSceneRendered() {
	if (!sceneReady_) {
		submitScene();
	}
	// if (ri_.Printf && !sceneReady_) {
	// 	ri_.Printf(PRINT_ALL, "Metal: renderScenePackets scott - sceneReady=%d\n", sceneReady_);
	// }
	if (!sceneReady_ || sceneDispatched_) {
		return;
	}
	renderScenePackets();
}

void MetalRenderer::renderScenePackets() {
	if (sceneDispatched_ || !sceneReady_) {
		if (ri_.Printf && !sceneReady_) {
			ri_.Printf(PRINT_WARNING, "Metal: renderScenePackets returning - sceneReady=%d, sceneDispatched=%d\n", sceneReady_, sceneDispatched_);
		}
		return;
	}
	if (!currentRenderEncoder_ || !device_ || !sceneCamera_.valid) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: renderScenePackets returning - encoder=%p, device=%p, camera=%d\n", 
			          currentRenderEncoder_, device_.get(), sceneCamera_.valid);
		}
		return;
	}

	const bool hasWork = !drawPackets_.empty() || !polyPackets_.empty() || !lightPackets_.empty();
	if (!hasWork) {
		sceneDispatched_ = true;
		return;
	}

	configureSceneViewport(sceneCamera_.refdef);
	if (!uploadSceneUniforms()) {
		return;
	}
	if (!uploadPolyVertexBuffer()) {
		return;
	}
	if (!uploadLightBuffer()) {
		return;
	}

	SceneDispatchSummary summary{};
	encodeEntityCommands(summary);
	encodePolyCommands(summary);
	encodeLightCommands(summary);
	if (!drawPolyPackets()) {
		return;
	}
	sceneDispatchSummary_ = summary;

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER,
		          "MetalScene: dispatch %d entities (%d model, %d sprite, %d beam), %d polys (%d verts), %d lights\n",
		          summary.totalEntities,
		          summary.modelEntities,
		          summary.spriteEntities,
		          summary.beamEntities,
		          summary.polySurfaces,
		          summary.polyVertices,
		          summary.dynamicLights);
	}

	sceneDispatched_ = true;
}

void MetalRenderer::configureSceneViewport(const refdef_t& refdef) {
	if (!currentRenderEncoder_) {
		return;
	}

	const double originX = static_cast<double>(std::clamp(refdef.x, 0, config_.vidWidth));
	const double originY = static_cast<double>(std::clamp(refdef.y, 0, config_.vidHeight));
	const double maxWidth = std::max(1.0, static_cast<double>(config_.vidWidth) - originX);
	const double maxHeight = std::max(1.0, static_cast<double>(config_.vidHeight) - originY);
	const double width = std::clamp(static_cast<double>(refdef.width), 1.0, maxWidth);
	const double height = std::clamp(static_cast<double>(refdef.height), 1.0, maxHeight);

	MTL::Viewport vp;
	vp.originX = originX;
	vp.originY = originY;
	vp.width = width;
	vp.height = height;
	vp.znear = 0.0;
	vp.zfar = 1.0;
	currentRenderEncoder_->setViewport(vp);

	// Don't use scissor rect - it can cause clipping issues
	MTL::ScissorRect scissor;
	scissor.x = 0;
	scissor.y = 0;
	scissor.width = static_cast<NS::UInteger>(config_.vidWidth);
	scissor.height = static_cast<NS::UInteger>(config_.vidHeight);
	currentRenderEncoder_->setScissorRect(scissor);
}

bool MetalRenderer::uploadSceneUniforms() {
	constexpr size_t requiredSize = sizeof(SceneUniforms);
	if (!device_) {
		return false;
	}
	if (!sceneUniformBuffer_ || sceneUniformBufferSize_ < requiredSize) {
		sceneUniformBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		sceneUniformBufferSize_ = requiredSize;
	}
	if (!sceneUniformBuffer_) {
		return false;
	}

	// Copy matrices directly - both C++ and Metal use column-major layout
	std::memcpy(sceneUniforms_.view, sceneCamera_.viewMatrix, sizeof(mat4_t));
	std::memcpy(sceneUniforms_.projection, sceneCamera_.projectionMatrix, sizeof(mat4_t));
	std::memcpy(sceneUniforms_.viewProjection, sceneCamera_.viewProjectionMatrix, sizeof(mat4_t));
	std::memcpy(sceneUniforms_.inverseView, sceneCamera_.inverseViewMatrix, sizeof(mat4_t));
	sceneUniforms_.viewOrigin[0] = sceneCamera_.viewOrigin[0];
	sceneUniforms_.viewOrigin[1] = sceneCamera_.viewOrigin[1];
	sceneUniforms_.viewOrigin[2] = sceneCamera_.viewOrigin[2];
	sceneUniforms_.viewOrigin[3] = 1.0f;
	sceneUniforms_.clipInfo[0] = sceneCamera_.zNear;
	sceneUniforms_.clipInfo[1] = sceneCamera_.zFar;
	sceneUniforms_.clipInfo[2] = sceneCamera_.zProj;
	sceneUniforms_.clipInfo[3] = sceneCamera_.stereoSeparation;

	// DEBUG: Print viewProjection matrix once
	static bool printed = false;
	if (!printed && ri_.Printf) {
		printed = true;
		ri_.Printf(PRINT_ALL, "Metal: ViewProjection Matrix (column-major):\n");
		Mat4Dump(sceneCamera_.viewProjectionMatrix);
		ri_.Printf(PRINT_ALL, "Metal: View Matrix:\n");
		Mat4Dump(sceneCamera_.viewMatrix);
		ri_.Printf(PRINT_ALL, "Metal: Projection Matrix:\n");
		Mat4Dump(sceneCamera_.projectionMatrix);
		ri_.Printf(PRINT_ALL, "Metal: ViewOrigin: %.2f, %.2f, %.2f\n",
			sceneCamera_.viewOrigin[0], sceneCamera_.viewOrigin[1], sceneCamera_.viewOrigin[2]);
	}

	std::memcpy(sceneUniformBuffer_->contents(), &sceneUniforms_, sizeof(SceneUniforms));
	return true;
}

bool MetalRenderer::uploadPolyVertexBuffer() {
	if (!polyVertexBufferDirty_) {
		return true;
	}
	polyVertexBufferDirty_ = false;
	polyVertexCountGPU_ = 0;

	if (polyVertices_.empty()) {
		return true;
	}
	if (!device_) {
		return false;
	}

	const size_t requiredSize = polyVertices_.size() * sizeof(MetalPolyVertex);
	if (!polyVertexBuffer_ || polyVertexBufferSize_ < requiredSize) {
		polyVertexBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		polyVertexBufferSize_ = requiredSize;
	}
	if (!polyVertexBuffer_) {
		return false;
	}

	std::memcpy(polyVertexBuffer_->contents(), polyVertices_.data(), requiredSize);
	polyVertexCountGPU_ = polyVertices_.size();
	return true;
}

bool MetalRenderer::uploadLightBuffer() {
	if (!lightBufferDirty_) {
		return true;
	}
	lightBufferDirty_ = false;
	lightCountGPU_ = 0;

	if (lightPackets_.empty()) {
		lightGPUData_.clear();
		return true;
	}
	if (!device_) {
		return false;
	}

	const size_t lightCount = lightPackets_.size();
	lightGPUData_.resize(lightCount);
	for (size_t i = 0; i < lightCount; ++i) {
		const MetalSceneLight& src = lightPackets_[i].light;
		SceneLightGPU& dst = lightGPUData_[i];
		dst.origin[0] = src.origin[0];
		dst.origin[1] = src.origin[1];
		dst.origin[2] = src.origin[2];
		dst.origin[3] = 1.0f;
		dst.color[0] = src.color[0];
		dst.color[1] = src.color[1];
		dst.color[2] = src.color[2];
		dst.color[3] = 1.0f;
		dst.params[0] = src.intensity;
		dst.params[1] = src.additive ? 1.0f : 0.0f;
		dst.params[2] = 0.0f;
		dst.params[3] = 0.0f;
	}

	const size_t requiredSize = lightCount * sizeof(SceneLightGPU);
	if (!lightBuffer_ || lightBufferSize_ < requiredSize) {
		lightBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		lightBufferSize_ = requiredSize;
	}
	if (!lightBuffer_) {
		return false;
	}

	std::memcpy(lightBuffer_->contents(), lightGPUData_.data(), requiredSize);
		lightCountGPU_ = lightCount;
	return true;
}

TextureManager* MetalRenderer::ensureTextureManager() {
	if (!textureManager_ && device_) {
		textureManager_ = std::make_unique<TextureManager>(device_.get(), &ri_);
	}
	return textureManager_.get();
}

void MetalRenderer::resetShaderCaches() {
	const bool hadWorld = worldLoaded_ && !worldName_.empty();
	const std::string pendingWorld = hadWorld ? worldName_ : std::string();
	unloadWorldMap();
	shaderLookup_.clear();
	shaderResources_.clear();
	resetStagePipelineCache();

	MetalShaderResource defaultShader;
	defaultShader.name = "*default";
	defaultShader.mipmap = false;
	defaultShader.script.forceOpaque = qtrue;
	defaultShader.script.imagePaths.push_back("white");
	ensureScriptHasStages(defaultShader);
	loadShaderImages(defaultShader);
	buildShaderStageRuntime(defaultShader);
	shaderResources_.push_back(std::move(defaultShader));
	shaderLookup_["*default"] = 0;
	if (hadWorld && !pendingWorld.empty()) {
		loadWorldMap(pendingWorld.c_str());
	}
}

void MetalRenderer::resetStagePipelineCache() {
	stagePipelineCache_.clear();
}

void MetalRenderer::ensureScriptHasStages(MetalRenderer::MetalShaderResource& resource) {
	MetalShaderScriptInfo& script = resource.script;
	if (script.stages.empty()) {
		MetalShaderStageInfo stage;
		stage.imagePaths = script.imagePaths;
		stage.blendMode = script.forceOpaque ? MetalShaderBlendMode::Opaque
		                                  : MetalShaderBlendMode::Alpha;
		stage.srcBlendFactor = script.forceOpaque ? MetalBlendFactor::One : MetalBlendFactor::SrcAlpha;
		stage.dstBlendFactor = script.forceOpaque ? MetalBlendFactor::Zero : MetalBlendFactor::OneMinusSrcAlpha;
		stage.depthWrite = script.forceOpaque ? true : false;
		stage.depthWriteExplicit = true;
		stage.alphaFunc = script.alphaFunc;
		stage.rgbGen.type = script.forceOpaque ? MetalRGBGen::IdentityLighting : MetalRGBGen::Identity;
		stage.alphaGen.type = MetalAlphaGen::Identity;
		if (stage.imagePaths.empty()) {
			stage.imagePaths.emplace_back("white");
			script.imagePaths.emplace_back("white");
		}
		script.stages.push_back(stage);
	}

	if (script.imageStageIndices.size() != script.imagePaths.size()) {
		script.imageStageIndices.assign(script.imagePaths.size(), 0);
	}

	for (int& index : script.imageStageIndices) {
		if (index < 0 || static_cast<size_t>(index) >= script.stages.size()) {
			index = 0;
		}
	}
}

void MetalRenderer::loadShaderImages(MetalRenderer::MetalShaderResource& resource) {
	resource.imageHandles.assign(resource.script.imagePaths.size(), 0);
	TextureManager* tm = ensureTextureManager();
	if (tm) {
		for (size_t i = 0; i < resource.script.imagePaths.size(); ++i) {
			resolveImageHandle(resource, i);
		}
	}
	updatePrimaryImageHandle(resource);
}

void MetalRenderer::buildShaderStageRuntime(MetalRenderer::MetalShaderResource& resource) {
	resource.stageRuntimes.clear();
	resource.primaryStageIndex = 0;

	const size_t stageCount = resource.script.stages.size();
	if (stageCount == 0) {
		updatePrimaryImageHandle(resource);
		return;
	}

	resource.stageRuntimes.resize(stageCount);
	std::vector<std::vector<qhandle_t>> stageHandles(stageCount);
	const size_t imageCount = resource.imageHandles.size();
	for (size_t i = 0; i < imageCount; ++i) {
		const int stageIndex = (i < resource.script.imageStageIndices.size())
			? resource.script.imageStageIndices[i]
			: 0;
		if (stageIndex < 0 || static_cast<size_t>(stageIndex) >= stageCount) {
			continue;
		}
		stageHandles[stageIndex].push_back(resource.imageHandles[i]);
	}

	for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
		MetalShaderResource::MetalShaderStageRuntime& runtime = resource.stageRuntimes[stageIndex];
		MetalShaderStageInfo& stageInfo = resource.script.stages[stageIndex];
		runtime.stageInfo = &stageInfo;
		runtime.usesLightmap = stageInfo.usesLightmap;
		runtime.usesWhiteImage = stageInfo.usesWhiteImage;
		runtime.stageImageHandles = stageHandles[stageIndex];
		runtime.pipelineKey.blendMode = stageInfo.blendMode;
		runtime.pipelineKey.srcBlend = stageInfo.srcBlendFactor;
		runtime.pipelineKey.dstBlend = stageInfo.dstBlendFactor;
		runtime.pipelineKey.depthWrite = stageInfo.depthWrite;
		runtime.pipelineKey.depthWriteExplicit = stageInfo.depthWriteExplicit;
		runtime.pipelineKey.alphaTest = stageInfo.alphaFunc != 0;
		runtime.pipelineKey.depthTest = true;
		if (runtime.stageImageHandles.empty() && stageInfo.usesWhiteImage) {
			runtime.stageImageHandles.push_back(0);
		}
		runtime.primaryStageImage = 0;
		for (qhandle_t handle : runtime.stageImageHandles) {
			if (handle != 0) {
				runtime.primaryStageImage = handle;
				break;
			}
		}
		if (runtime.primaryStageImage == 0 && !runtime.stageImageHandles.empty()) {
			runtime.primaryStageImage = runtime.stageImageHandles[0];
		}
	}

	updatePrimaryImageHandle(resource);

}

std::size_t MetalRenderer::MetalPipelineKeyHash::operator()(const MetalRenderer::MetalShaderResource::MetalPipelineKey& key) const noexcept {
	uint64_t value = 0;
	value |= (static_cast<uint64_t>(key.blendMode) & 0xF) << 0;
	value |= (static_cast<uint64_t>(key.srcBlend) & 0xF) << 4;
	value |= (static_cast<uint64_t>(key.dstBlend) & 0xF) << 8;
	value |= (key.depthWrite ? 1ULL : 0ULL) << 12;
	value |= (key.depthTest ? 1ULL : 0ULL) << 13;
	value |= (key.alphaTest ? 1ULL : 0ULL) << 14;
	value |= (key.depthWriteExplicit ? 1ULL : 0ULL) << 15;
	return static_cast<std::size_t>(value);
}

MetalRenderer::MetalShaderResource* MetalRenderer::getShaderResource(qhandle_t handle) {
	if (handle < 0) {
		return nullptr;
	}
	const size_t index = static_cast<size_t>(handle);
	if (index >= shaderResources_.size()) {
		return nullptr;
	}
	return &shaderResources_[index];
}

const MetalRenderer::MetalShaderResource* MetalRenderer::getShaderResource(qhandle_t handle) const {
	if (handle < 0) {
		return nullptr;
	}
	const size_t index = static_cast<size_t>(handle);
	if (index >= shaderResources_.size()) {
		return nullptr;
	}
	return &shaderResources_[index];
}

const MetalRenderer::MetalShaderResource::MetalShaderStageRuntime*
MetalRenderer::getShaderStageRuntime(const MetalRenderer::MetalShaderResource& resource, size_t stageIndex) const {
	if (stageIndex >= resource.stageRuntimes.size()) {
		return nullptr;
	}
	return &resource.stageRuntimes[stageIndex];
}

const MetalRenderer::MetalShaderResource::MetalShaderStageRuntime*
MetalRenderer::getPrimaryStageRuntime(const MetalRenderer::MetalShaderResource& resource) const {
	if (resource.stageRuntimes.empty()) {
		return nullptr;
	}
	const size_t index = std::min(resource.primaryStageIndex, resource.stageRuntimes.size() - 1);
	return &resource.stageRuntimes[index];
}

qhandle_t MetalRenderer::selectStageImage(MetalRenderer::MetalShaderResource& resource,
	const MetalRenderer::MetalShaderResource::MetalShaderStageRuntime* runtime,
	float timeSeconds,
	qhandle_t lightmapHandle) {
	if (runtime) {
		const MetalShaderStageInfo* info = runtime->stageInfo;
		if (runtime->usesLightmap) {
			if (lightmapHandle > 0) {
				return lightmapHandle;
			}
			// Fall through to shader image fallback when lightmap is missing
		}
		const size_t frameCount = runtime->stageImageHandles.size();
		if (frameCount > 0) {
			size_t frameIndex = 0;
			if (info && info->animFrequency > 0.0f && frameCount > 1) {
				const float frames = std::floor(std::max(0.0f, timeSeconds) * info->animFrequency);
				frameIndex = static_cast<size_t>(static_cast<long long>(frames) % static_cast<long long>(frameCount));
			}
			const qhandle_t handle = runtime->stageImageHandles[frameIndex];
			if (handle != 0) {
				return handle;
			}
		}
		if (runtime->primaryStageImage != 0) {
			return runtime->primaryStageImage;
		}
	}
	if (resource.primaryImageHandle == 0) {
		updatePrimaryImageHandle(resource);
	}
	if (resource.primaryImageHandle == 0 && !resource.script.imagePaths.empty()) {
		resource.primaryImageHandle = resolveImageHandle(resource, 0);
	}
	return resource.primaryImageHandle;
}

qhandle_t MetalRenderer::resolveImageHandle(MetalRenderer::MetalShaderResource& resource, size_t imageIndex) {
	if (imageIndex >= resource.imageHandles.size()) {
		return 0;
	}

	qhandle_t& handle = resource.imageHandles[imageIndex];
	if (handle != 0) {
		return handle;
	}

	TextureManager* tm = ensureTextureManager();
	if (!tm) {
		return 0;
	}

	const std::string& path = resource.script.imagePaths[imageIndex];
	if (path.empty()) {
		return 0;
	}
	if (!path.empty() && path[0] == '$') {
		return 0;
	}
	if (path == "<white>") {
		handle = tm->registerShader("white", false);
		return handle;
	}

	handle = tm->registerShader(path.c_str(), resource.mipmap);
	return handle;
}

void MetalRenderer::updatePrimaryImageHandle(MetalRenderer::MetalShaderResource& resource) {
	resource.primaryImageHandle = 0;
	for (const auto& runtime : resource.stageRuntimes) {
		if (runtime.primaryStageImage != 0) {
			resource.primaryImageHandle = runtime.primaryStageImage;
			return;
		}
	}
	for (size_t i = 0; i < resource.imageHandles.size(); ++i) {
		const qhandle_t handle = resolveImageHandle(resource, i);
		if (handle != 0) {
			resource.primaryImageHandle = handle;
			return;
		}
	}
}

bool MetalRenderer::assetExists(const char* path) const {
	if (!path || !path[0]) {
		return false;
	}

	if (ri_.FS_FileExists && ri_.FS_FileExists(path)) {
		return true;
	}

	if (ri_.FS_FileIsInPAK && ri_.FS_FileIsInPAK(path, nullptr) == 1) {
		return true;
	}

	if (ri_.FS_ReadFile && ri_.FS_FreeFile) {
		void* buffer = nullptr;
		const long len = ri_.FS_ReadFile(path, &buffer);
		if (buffer) {
			ri_.FS_FreeFile(buffer);
		}
		if (len >= 0) {
			return true;
		}
	}

	return false;
}

MTL::BlendFactor MetalRenderer::ToMetalBlendFactor(MetalBlendFactor factor) {
	switch (factor) {
		case MetalBlendFactor::Zero:
			return MTL::BlendFactorZero;
		case MetalBlendFactor::One:
			return MTL::BlendFactorOne;
		case MetalBlendFactor::SrcColor:
			return MTL::BlendFactorSourceColor;
		case MetalBlendFactor::OneMinusSrcColor:
			return MTL::BlendFactorOneMinusSourceColor;
		case MetalBlendFactor::DstColor:
			return MTL::BlendFactorDestinationColor;
		case MetalBlendFactor::OneMinusDstColor:
			return MTL::BlendFactorOneMinusDestinationColor;
		case MetalBlendFactor::SrcAlpha:
			return MTL::BlendFactorSourceAlpha;
		case MetalBlendFactor::OneMinusSrcAlpha:
			return MTL::BlendFactorOneMinusSourceAlpha;
		case MetalBlendFactor::DstAlpha:
			return MTL::BlendFactorDestinationAlpha;
		case MetalBlendFactor::OneMinusDstAlpha:
			return MTL::BlendFactorOneMinusDestinationAlpha;
		default:
			return MTL::BlendFactorOne;
	}
}

bool MetalRenderer::ensureSceneShaderResources() {
	if (sceneSampler_ && sceneVertexDescriptor_ && sceneVertexFunction_ && sceneFragmentFunction_) {
		return true;
	}
	if (ri_.Printf) ri_.Printf(PRINT_ALL, "Metal: ensureSceneShaderResources loading shaders...\n");
	if (!device_) {
		return false;
	}

	if (!sceneLibrary_) {
		sceneLibrary_.reset(device_->newDefaultLibrary());
		if (!sceneLibrary_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: failed to load default.metallib for scene shaders\n");
			}
			return false;
		}
	}

	if (!sceneVertexFunction_) {
		NS::String* vertexName = NS::String::string("vertex_scene_basic", NS::ASCIIStringEncoding);
		sceneVertexFunction_.reset(sceneLibrary_->newFunction(vertexName));
		vertexName->release();
		if (!sceneVertexFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: missing vertex_scene_basic function\n");
			}
			return false;
		}
	}

	if (!sceneFragmentFunction_) {
		NS::String* fragmentName = NS::String::string("fragment_scene_basic", NS::ASCIIStringEncoding);
		sceneFragmentFunction_.reset(sceneLibrary_->newFunction(fragmentName));
		fragmentName->release();
		if (!sceneFragmentFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: missing fragment_scene_basic function\n");
			}
			return false;
		}
	}

	if (!sceneVertexDescriptor_) {
		MTL::VertexDescriptor* vertexDesc = MTL::VertexDescriptor::alloc()->init();
		
		// Attribute 0: Position (float3) - Offset 0
		MTL::VertexAttributeDescriptor* attr0 = vertexDesc->attributes()->object(0);
		attr0->setFormat(MTL::VertexFormatFloat3);
		attr0->setOffset(0);
		attr0->setBufferIndex(0);

		// Attribute 1: TexCoord (float2) - Offset 12
		MTL::VertexAttributeDescriptor* attr1 = vertexDesc->attributes()->object(1);
		attr1->setFormat(MTL::VertexFormatFloat2);
		attr1->setOffset(sizeof(float) * 3);
		attr1->setBufferIndex(0);

		// Attribute 2: Lightmap (float2) - Offset 20
		MTL::VertexAttributeDescriptor* attr2 = vertexDesc->attributes()->object(2);
		attr2->setFormat(MTL::VertexFormatFloat2);
		attr2->setOffset(sizeof(float) * 5);
		attr2->setBufferIndex(0);

		// Attribute 3: Normal (float3) - Offset 28
		MTL::VertexAttributeDescriptor* attr3 = vertexDesc->attributes()->object(3);
		attr3->setFormat(MTL::VertexFormatFloat3);
		attr3->setOffset(sizeof(float) * 7);
		attr3->setBufferIndex(0);

		// Attribute 4: Color (uchar4) - Offset 40
		MTL::VertexAttributeDescriptor* attr4 = vertexDesc->attributes()->object(4);
		attr4->setFormat(MTL::VertexFormatUChar4Normalized);
		attr4->setOffset(sizeof(float) * 10);
		attr4->setBufferIndex(0);

		MTL::VertexBufferLayoutDescriptor* layout = vertexDesc->layouts()->object(0);
		layout->setStride(sizeof(MetalPolyVertex));
		layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
		layout->setStepRate(1);
		sceneVertexDescriptor_.reset(vertexDesc);

		if (ri_.Printf) {
			ri_.Printf(PRINT_ALL, "Metal: Vertex Descriptor Layout:\n");
			ri_.Printf(PRINT_ALL, "  Stride: %zu\n", sizeof(MetalPolyVertex));
			ri_.Printf(PRINT_ALL, "  Offset Position: %zu\n", offsetof(MetalPolyVertex, xyz));
			ri_.Printf(PRINT_ALL, "  Offset TexCoord: %zu\n", offsetof(MetalPolyVertex, st));
			ri_.Printf(PRINT_ALL, "  Offset Lightmap: %zu\n", offsetof(MetalPolyVertex, lightmap));
			ri_.Printf(PRINT_ALL, "  Offset Normal:   %zu\n", offsetof(MetalPolyVertex, normal));
			ri_.Printf(PRINT_ALL, "  Offset Color:    %zu\n", offsetof(MetalPolyVertex, modulate));
		}
	}

	if (!sceneSampler_) {
		MTL::SamplerDescriptor* sampDesc = MTL::SamplerDescriptor::alloc()->init();
		sampDesc->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampDesc->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampDesc->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampDesc->setTAddressMode(MTL::SamplerAddressModeRepeat);
		sceneSampler_.reset(device_->newSamplerState(sampDesc));
		sampDesc->release();
	}

	return sceneSampler_.get() != nullptr &&
	       sceneVertexDescriptor_.get() != nullptr &&
	       sceneVertexFunction_.get() != nullptr &&
	       sceneFragmentFunction_.get() != nullptr;
}

MetalRenderer::StagePipelineEntry* MetalRenderer::getStagePipeline(const MetalRenderer::MetalShaderResource::MetalPipelineKey& key) {
	if (!ensureSceneShaderResources()) {
		return nullptr;
	}

	auto it = stagePipelineCache_.find(key);
	if (it != stagePipelineCache_.end()) {
		return &it->second;
	}

	StagePipelineEntry entry;
	entry.key = key;

	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexDescriptor(sceneVertexDescriptor_.get());
	pd->setVertexFunction(sceneVertexFunction_.get());
	pd->setFragmentFunction(sceneFragmentFunction_.get());
	MTL::RenderPipelineColorAttachmentDescriptor* colorAttachment = pd->colorAttachments()->object(0);
	colorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	const bool enableBlend = !(key.srcBlend == MetalBlendFactor::One && key.dstBlend == MetalBlendFactor::Zero);
	colorAttachment->setBlendingEnabled(enableBlend);
	colorAttachment->setSourceRGBBlendFactor(ToMetalBlendFactor(key.srcBlend));
	colorAttachment->setSourceAlphaBlendFactor(ToMetalBlendFactor(key.srcBlend));
	colorAttachment->setDestinationRGBBlendFactor(ToMetalBlendFactor(key.dstBlend));
	colorAttachment->setDestinationAlphaBlendFactor(ToMetalBlendFactor(key.dstBlend));
	colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	NS::Error* error = nullptr;
	entry.pipeline.reset(device_->newRenderPipelineState(pd, &error));
	pd->release();
	if (!entry.pipeline) {
		if (ri_.Printf) {
			const char* msg = error ? error->localizedDescription()->utf8String() : "unknown";
			ri_.Printf(PRINT_WARNING, "Metal: failed to build stage pipeline (%s)\n", msg);
		}
		if (error) {
			error->release();
		}
		return nullptr;
	}
	if (error) {
		error->release();
	}

	MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
	depthDesc->setDepthWriteEnabled(key.depthWrite);
	depthDesc->setDepthCompareFunction(key.depthTest ? MTL::CompareFunctionLessEqual : MTL::CompareFunctionAlways);
	entry.depthState.reset(device_->newDepthStencilState(depthDesc));
	depthDesc->release();
	if (!entry.depthState) {
		return nullptr;
	}

	auto [insertedIt, _] = stagePipelineCache_.emplace(key, std::move(entry));
	return &insertedIt->second;
}

bool MetalRenderer::drawPolyPackets() {
	if (polyPackets_.empty()) {
		return true;
	}
	if (!currentRenderEncoder_ || !polyVertexBuffer_ || polyVertexCountGPU_ == 0) {
		// if (ri_.Printf) ri_.Printf(PRINT_ALL, "Metal: drawPolyPackets early exit - encoder=%p buffer=%p verts=%d\n",
		//                           currentRenderEncoder_, polyVertexBuffer_.get(), static_cast<int>(polyVertexCountGPU_));
		return true;
	}
	if (!sceneUniformBuffer_) {
		if (ri_.Printf) ri_.Printf(PRINT_ALL, "Metal: drawPolyPackets - no uniform buffer\n");
		return false;
	}
	if (!ensureSceneShaderResources()) {
		if (ri_.Printf) ri_.Printf(PRINT_ALL, "Metal: ensureSceneShaderResources failed\n");
		return false;
	}

	// if (ri_.Printf) {
	// 	static int frameCount = 0;
	// 	if (frameCount++ % 60 == 0) {
	// 		ri_.Printf(PRINT_ALL, "Metal: drawPolyPackets - %zu packets, %d GPU verts\n",
	// 		          polyPackets_.size(), static_cast<int>(polyVertexCountGPU_));
	// 	}
	// }

	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);

	TextureManager* texManager = ensureTextureManager();
	if (!texManager) {
		return false;
	}
	MTL::Texture* defaultTexture = texManager->getTexture(0);
	if (!defaultTexture) {
		return false;
	}
	MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sceneSampler_.get());

	MTL::RenderPipelineState* boundPipeline = nullptr;
	MTL::DepthStencilState* boundDepthState = nullptr;
	qhandle_t boundImageHandle = -1;
	MTL::Texture* boundTexture = nullptr;
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
	StageFragmentParams lastFragmentParams{};
	bool hasFragmentParams = false;
	auto bindStageParams = [&](const StageFragmentParams& params) {
		if (!hasFragmentParams || std::memcmp(&params, &lastFragmentParams, sizeof(StageFragmentParams)) != 0) {
			currentRenderEncoder_->setFragmentBytes(&params, sizeof(StageFragmentParams), 0);
			lastFragmentParams = params;
			hasFragmentParams = true;
		}
	};
	auto buildStageParams = [](const MetalShaderStageInfo* stageInfo) {
		StageFragmentParams params{};
		if (!stageInfo) {
			return params;
		}
		const bool useAlternateCoords = stageInfo->tcGen.type == MetalTCGen::Lightmap;
		params.texCoordSelector = useAlternateCoords ? 1.0f : 0.0f;
		const int alphaFunc = stageInfo->alphaFunc;
		if (alphaFunc == 0) {
			return params;
		}
		params.alphaTestEnabled = 1.0f;
		params.alphaFunc = static_cast<float>(alphaFunc);
		switch (alphaFunc) {
			case 1: // GT0
				params.alphaRef = 1.0f / 255.0f;
				break;
			case 2: // LT128
			case 3: // GE128
			default:
				params.alphaRef = 128.0f / 255.0f;
				break;
		}
		return params;
	};

	for (const ScenePolyPacket& packet : polyPackets_) {
		if (packet.vertexCount <= 0) {
			continue;
		}
		const size_t endVertex = static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount);
		if (endVertex > polyVertexCountGPU_) {
			continue;
		}

		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (!shaderResource) {
			shaderResource = getShaderResource(0);
		}
		if (!shaderResource || shaderResource->stageRuntimes.empty()) {
			continue;
		}

		const size_t stageCount = shaderResource->stageRuntimes.size();
		for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
			const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
			if (!stageRuntime) {
				continue;
			}
			const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
			bindStageParams(buildStageParams(stageInfo));

			StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
			if (!pipelineEntry || !pipelineEntry->pipeline || !pipelineEntry->depthState) {
				static bool warned = false;
				if (!warned && ri_.Printf) {
					ri_.Printf(PRINT_ALL, "Metal: Missing pipeline for shader %s stage %zu\n", shaderResource->name.c_str(), stageIndex);
					warned = true;
				}
				continue;
			}

			if (pipelineEntry->pipeline.get() != boundPipeline) {
				MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
				boundPipeline = pipelineEntry->pipeline.get();
			}
			if (pipelineEntry->depthState.get() != boundDepthState) {
				currentRenderEncoder_->setDepthStencilState(pipelineEntry->depthState.get());
				boundDepthState = pipelineEntry->depthState.get();
			}

			qhandle_t desiredHandle = selectStageImage(*shaderResource, stageRuntime, sceneTimeSeconds, packet.lightmapHandle);
			if (desiredHandle < 0) {
				desiredHandle = 0;
			}

			if (!boundTexture || desiredHandle != boundImageHandle) {
				MTL::Texture* packetTexture = texManager->getTexture(desiredHandle);
				if (!packetTexture) {
					packetTexture = defaultTexture;
					desiredHandle = 0;
				}
				MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, packetTexture);
				boundTexture = packetTexture;
				boundImageHandle = desiredHandle;
			}
			currentRenderEncoder_->drawPrimitives(packet.primitive,
			                                   static_cast<NS::UInteger>(packet.firstVertex),
			                                   static_cast<NS::UInteger>(packet.vertexCount));
		}
	}
	return true;
}

void MetalRenderer::encodeEntityCommands(SceneDispatchSummary& summary) {
	for (const SceneDrawPacket& packet : drawPackets_) {
		++summary.totalEntities;
		switch (packet.entity.reType) {
			case RT_MODEL:
				++summary.modelEntities;
				break;
			case RT_SPRITE:
			case RT_RAIL_CORE:
			case RT_RAIL_RINGS:
				++summary.spriteEntities;
				break;
			case RT_BEAM:
			case RT_LIGHTNING:
				++summary.beamEntities;
				break;
			default:
				break;
		}
	}
}

void MetalRenderer::encodePolyCommands(SceneDispatchSummary& summary) {
	if (polyPackets_.empty()) {
		return;
	}
	summary.polySurfaces += static_cast<int>(polyPackets_.size());
	int totalVerts = 0;
	for (const ScenePolyPacket& packet : polyPackets_) {
		totalVerts += packet.vertexCount;
	}
	summary.polyVertices += totalVerts;
}

void MetalRenderer::encodeLightCommands(SceneDispatchSummary& summary) {
	summary.dynamicLights += static_cast<int>(lightPackets_.size());
}

void MetalRenderer::updateCamera(const MetalSceneState& scene) {
	sceneCamera_.valid = false;
	if (!scene.refdefValid) {
		return;
	}

	sceneCamera_.refdef = scene.refdef;
	VectorCopy(scene.refdef.vieworg, sceneCamera_.viewOrigin);
	VectorCopy(scene.refdef.viewaxis[0], sceneCamera_.viewAxis[0]);
	VectorCopy(scene.refdef.viewaxis[1], sceneCamera_.viewAxis[1]);
	VectorCopy(scene.refdef.viewaxis[2], sceneCamera_.viewAxis[2]);

	const float zNear = r_znear_ ? r_znear_->value : 4.0f;
	float zFar = pickFarPlane(scene);
	const float zProj = r_zproj_ ? r_zproj_->value : 64.0f;

	const float minNear = 0.001f;
	sceneCamera_.zNear = std::max(zNear, minNear);
	if (zFar <= sceneCamera_.zNear + 1.0f) {
		zFar = sceneCamera_.zNear + 1.0f;
	}
	sceneCamera_.zFar = zFar;
	sceneCamera_.zProj = std::max(1.0f, zProj);
	sceneCamera_.stereoSeparation = computeStereoOffset(stereoFrame_, sceneCamera_.zProj);

	buildViewMatrix(sceneCamera_);
	buildProjectionMatrix(sceneCamera_);

	Mat4Multiply(sceneCamera_.projectionMatrix, sceneCamera_.viewMatrix, sceneCamera_.viewProjectionMatrix);
	Mat4SimpleInverse(sceneCamera_.viewMatrix, sceneCamera_.inverseViewMatrix);
	
	sceneCamera_.valid = true;
}

void MetalRenderer::buildViewMatrix(SceneCamera& camera) {
	vec3_t axes[3];
	VectorCopy(camera.viewAxis[0], axes[0]);
	VectorCopy(camera.viewAxis[1], axes[1]);
	VectorCopy(camera.viewAxis[2], axes[2]);
	vec3_t origin;
	VectorCopy(camera.viewOrigin, origin);
	Mat4View(axes, origin, camera.viewMatrix);
}

void MetalRenderer::buildProjectionMatrix(SceneCamera& camera) {
	// Match GL2 R_SetupProjection + R_SetupProjectionZ exactly
	// then convert to Metal's [0,1] depth range
	
	const float fovX = camera.refdef.fov_x;
	const float fovY = camera.refdef.fov_y;
	const float zNear = camera.zNear;
	const float zFar = camera.zFar;
	const float zProj = camera.zProj;

	// GL2 uses zProj for the frustum calculation
	float ymax = zProj * std::tan(fovY * M_PI / 360.0f);
	float ymin = -ymax;
	float xmax = zProj * std::tan(fovX * M_PI / 360.0f);
	float xmin = -xmax;

	float width = xmax - xmin;
	float height = ymax - ymin;

	Mat4Zero(camera.projectionMatrix);

	// GL2 projection setup
	camera.projectionMatrix[0] = 2.0f * zProj / width;
	camera.projectionMatrix[5] = 2.0f * zProj / height;
	camera.projectionMatrix[8] = (xmax + xmin) / width;   // normally 0
	camera.projectionMatrix[9] = (ymax + ymin) / height;  // normally 0
	camera.projectionMatrix[11] = -1.0f;

	// GL2 Z setup uses [-1,1] depth: -(zFar+zNear)/(zFar-zNear), -2*zFar*zNear/(zFar-zNear)
	// Metal needs [0,1] depth: -zFar/(zFar-zNear), -zFar*zNear/(zFar-zNear)
	float depth = zFar - zNear;
	camera.projectionMatrix[10] = -zFar / depth;
	camera.projectionMatrix[14] = -zFar * zNear / depth;
}

float MetalRenderer::computeStereoOffset(stereoFrame_t frame, float zProj) const {
	if (!r_stereoSeparation_) {
		return 0.0f;
	}
	const float baseSeparation = r_stereoSeparation_->value;
	if (baseSeparation == 0.0f) {
		return 0.0f;
	}
	if (frame == STEREO_LEFT) {
		return zProj / baseSeparation;
	}
	if (frame == STEREO_RIGHT) {
		return zProj / -baseSeparation;
	}
	return 0.0f;
}

float MetalRenderer::pickFarPlane(const MetalSceneState& scene) const {
	if (scene.refdef.rdflags & RDF_NOWORLDMODEL) {
		return 2048.0f;
	}
	return 8192.0f;
}

void MetalRenderer::drawCinematic(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	// If we have a pending 3D scene that hasn't been submitted for this frame, submit it now.
	// This ensures 3D draws before 2D cinematics.
	submitScene();
	ensureSceneRendered();

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
	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipeline_.get());
	if (depthState2D_) {
		currentRenderEncoder_->setDepthStencilState(depthState2D_.get());
	}
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

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

	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: Viewport x=%.1f y=%.1f w=%.1f h=%.1f\n", vp.originX, vp.originY, vp.width, vp.height);
	}

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
	MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, frameCinematicTexture_);
	MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler_.get());
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

	resetShaderCaches();

	if (configOut) {
		*configOut = config_;
	}
	publishConfig();

	if (!inputInitialized_) {
		ri_.IN_Init(SDLMetal_GetWindow());
		inputInitialized_ = true;
	}
}

//=============================================================================
// Global Renderer Instance
//=============================================================================

static MetalRenderer g_renderer;

static void Metal_GfxInfo_f() {
	g_renderer.printGfxInfo();
}

static void Metal_ImageList_f() {
	g_renderer.printImageList();
}

static void Metal_DebugPoly_f() {
	if (!ri.Cmd_Argc || !ri.Cmd_Argv) {
		return;
	}

	const int argc = ri.Cmd_Argc();
	if (argc <= 1) {
		g_renderer.configureDebugPoly(false, nullptr, 0.0f, 0.0f);
		return;
	}

	const char* shaderName = ri.Cmd_Argv(1);
	const float size = (argc > 2) ? static_cast<float>(std::atof(ri.Cmd_Argv(2))) : 256.0f;
	const float distance = (argc > 3) ? static_cast<float>(std::atof(ri.Cmd_Argv(3))) : 256.0f;
	g_renderer.configureDebugPoly(true, shaderName, size, distance);
}

bool MetalBackend_Initialize(refimport_t imports) {
	const bool initialized = g_renderer.initialize(imports);
	if (initialized) {
		Metal_LogRendererCall("re.Initialize");
	}
	return initialized;
}

void MetalBackend_Shutdown(qboolean destroyWindow) {
	Metal_LogRendererCall("re.Shutdown");
	g_renderer.shutdown(destroyWindow);
}

void MetalBackend_BeginRegistration(glconfig_t* glconfigOut) {
	Metal_LogRendererCall("re.BeginRegistration");
	g_renderer.beginRegistration(glconfigOut);
}

void MetalBackend_BeginFrame(stereoFrame_t stereoFrame) {
	// Metal_LogRendererCall("re.BeginFrame");
	g_renderer.beginFrame(stereoFrame);
}

void MetalBackend_EndFrame(int* frontEndMsec, int* backEndMsec) {
	// Metal_LogRendererCall("re.EndFrame");
	g_renderer.endFrame(frontEndMsec, backEndMsec);
}

void MetalBackend_UploadCinematic(int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	Metal_LogRendererCall("re.UploadCinematic");
	g_renderer.uploadCinematic(w, h, cols, rows, data, client, dirty);
}

void MetalBackend_DrawStretchRaw(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty) {
	Metal_LogRendererCall("re.DrawStretchRaw");
	g_renderer.drawCinematic(x, y, w, h, cols, rows, data, client, dirty);
}

qhandle_t MetalBackend_RegisterShader(const char* name, bool mipmap) {
	Metal_LogRendererCall(mipmap ? "re.RegisterShader" : "re.RegisterShaderNoMip");
	return g_renderer.registerShader(name, mipmap);
}

qhandle_t MetalBackend_RegisterModel(const char* name) {
	Metal_LogRendererCall("re.RegisterModel");
	return g_renderer.registerModel(name);
}

qhandle_t MetalBackend_RegisterSkin(const char* name) {
	Metal_LogRendererCall("re.RegisterSkin");
	return g_renderer.registerSkin(name);
}

void MetalBackend_SetColor(const float* rgba) {
	Metal_LogRendererCall("re.SetColor");
	g_renderer.setColor(rgba);
}

void MetalBackend_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {
	// Metal_LogRendererCall("re.DrawStretchPic");
	g_renderer.drawStretchPic(x, y, w, h, s1, t1, s2, t2, shader);
}

void MetalBackend_LoadWorld(const char* name) {
	Metal_LogRendererCall("re.LoadWorld");
	g_renderer.loadWorldMap(name);
}

void MetalBackend_SetWorldVisData(const byte* vis) {
	Metal_LogRendererCall("re.SetWorldVisData");
	(void)vis;
}

void MetalBackend_EndRegistration() {
	Metal_LogRendererCall("re.EndRegistration");
}

void Metal_LogRendererCall(const char* name) {
	g_renderer.logRendererCall(name);
}
