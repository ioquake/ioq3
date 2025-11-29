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
#include "tr_local.h"
#include "tr_model.h"

extern "C" {
#include "../qcommon/qfiles.h"
}

extern "C" {
	glconfig_t glConfig = {};
	refimport_t ri;

	// Lighting console variables (defined here, declared in tr_local.h)
	cvar_t* r_ambientScale = nullptr;
	cvar_t* r_directedScale = nullptr;
	cvar_t* r_debugLight = nullptr;
	cvar_t* r_dlightMode = nullptr;
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
	float texCoordSelector = 0.0f;  // Legacy - now use tcGenType instead
	float rgbGenType = 0.0f;  // 0 = Vertex, 1 = Identity, 2 = IdentityLighting
	float tcGenType = 0.0f;   // 0 = Texture, 1 = Lightmap, 2 = Environment
	float overBrightBits = 0.0f;
	float padding3 = 0.0f;
};

// Dynamic light uniforms - matches dlight.metal DlightUniforms
struct alignas(16) DlightUniforms {
	float modelViewProjection[16] = {};
	float dlightInfo[4] = {};  // xyz = light position, w = 1/radius
	float color[4] = {};       // rgba = light color
	int32_t deformGen = 0;
	float deformParams[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	float time = 0.0f;
	float vertexLerp = 0.0f;
	float padding[2] = {0.0f, 0.0f};
};

static_assert(sizeof(DlightUniforms) % 16 == 0, "DlightUniforms must be 16-byte aligned");

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

// Skin system data structures (matches OpenGL2 tr_local.h:792-801)
constexpr int MAX_SKIN_SURFACES = 256;

struct MetalSkinSurface {
	char name[MAX_QPATH];
	qhandle_t shader;

	MetalSkinSurface() : name{}, shader(0) {}
};

struct MetalSkin {
	char name[MAX_QPATH];
	int numSurfaces;
	std::vector<MetalSkinSurface> surfaces;

	MetalSkin() : name{}, numSurfaces(0) {}
};

// Helper function for parsing comma-separated skin files (matches OpenGL2 CommaParse)
static char* CommaParse(char** data_p) {
	static char com_token[MAX_TOKEN_CHARS];
	int c = 0, len;
	char* data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data) {
		*data_p = NULL;
		return com_token;
	}

	while (1) {
		// skip whitespace
		while ((c = *data) <= ' ') {
			if (!c) {
				break;
			}
			data++;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/') {
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*') {
			data += 2;
			while (*data && (*data != '*' || data[1] != '/')) {
				data++;
			}
			if (*data) {
				data += 2;
			}
		}
		else {
			break;
		}
	}

	if (c == 0) {
		return com_token;
	}

	// handle quoted strings
	if (c == '\"') {
		data++;
		while (1) {
			c = *data++;
			if (c == '\"' || !c) {
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do {
		if (len < MAX_TOKEN_CHARS - 1) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32 && c != ',');

	com_token[len] = 0;

	*data_p = data;
	return com_token;
}
}

namespace {
// Apply overbright color shift to vertex colors (matches OpenGL2's R_ColorShiftLightingFloats)
inline void ColorShiftLightingBytes(const byte in[4], byte out[4], int mapOverBrightBits, int overBrightBits) {
	// Shift the color data based on overbright range
	int shift = mapOverBrightBits - overBrightBits;

	// Shift the data based on overbright range
	int r = in[0] << shift;
	int g = in[1] << shift;
	int b = in[2] << shift;

	// Normalize by color instead of saturating to white
	if (shift > 0 && (r > 255 || g > 255 || b > 255)) {
		int max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r > 255 ? 255 : (r < 0 ? 0 : r);
	out[1] = g > 255 ? 255 : (g < 0 ? 0 : g);
	out[2] = b > 255 ? 255 : (b < 0 ? 0 : b);
	out[3] = in[3];
}

inline MetalPolyVertex ConvertDrawVert(const drawVert_t& src, int mapOverBrightBits, int overBrightBits) {
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
	// Apply overbright color shift to vertex colors (lightmap data)
	byte shiftedColor[4];
	ColorShiftLightingBytes(src.color, shiftedColor, mapOverBrightBits, overBrightBits);
	dst.modulate[0] = shiftedColor[0];
	dst.modulate[1] = shiftedColor[1];
	dst.modulate[2] = shiftedColor[2];
	dst.modulate[3] = shiftedColor[3];
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
#include <limits>
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
                                  std::vector<MetalPolyVertex>& outVerts, int tessLevel,
                                  int mapOverBrightBits, int overBrightBits) {
	// Convert all control points to our vertex format
	std::vector<MetalPolyVertex> ctrl(static_cast<size_t>(width * height));
	for (int i = 0; i < width * height; ++i) {
		ctrl[static_cast<size_t>(i)] = ConvertDrawVert(controlPoints[i], mapOverBrightBits, overBrightBits);
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
// TCMod Computation Helpers
//=============================================================================

namespace {

// Evaluate a wave function at a given time
inline float EvalWaveForm(const MetalWaveForm& wave, float time) {
	if (wave.frequency == 0.0f) {
		return wave.base;
	}
	
	float phase = wave.phase + time * wave.frequency;
	float value = 0.0f;
	
	switch (wave.func) {
		case MetalWaveFunc::Sin:
			value = std::sin(phase * 2.0f * static_cast<float>(M_PI));
			break;
		case MetalWaveFunc::Triangle:
			value = std::fabs(std::fmod(phase + 0.75f, 1.0f) - 0.5f) * 4.0f - 1.0f;
			break;
		case MetalWaveFunc::Square:
			value = (std::fmod(phase, 1.0f) < 0.5f) ? 1.0f : -1.0f;
			break;
		case MetalWaveFunc::Sawtooth:
			value = std::fmod(phase, 1.0f);
			break;
		case MetalWaveFunc::InverseSawtooth:
			value = 1.0f - std::fmod(phase, 1.0f);
			break;
		case MetalWaveFunc::Noise:
			// Simple pseudo-noise based on phase
			value = std::sin(phase * 12.9898f) * 43758.5453f;
			value = value - std::floor(value);
			value = value * 2.0f - 1.0f;
			break;
		default:
			break;
	}
	
	return wave.base + value * wave.amplitude;
}

// Compute a 2x3 texture matrix for a single tcMod operation
// Matrix format: [0]=scaleX, [1]=shearY, [2]=shearX, [3]=scaleY, [4]=translateX, [5]=translateY
inline void ComputeTCModMatrix(const MetalTCMod& mod, float time, float outMatrix[6], float& turbAmp, float& turbPhase) {
	// Initialize to identity
	outMatrix[0] = 1.0f; outMatrix[2] = 0.0f; outMatrix[4] = 0.0f;
	outMatrix[1] = 0.0f; outMatrix[3] = 1.0f; outMatrix[5] = 0.0f;
	turbAmp = 0.0f;
	turbPhase = 0.0f;
	
	switch (mod.type) {
		case MetalTCModType::Scroll: {
			// args[0] = sSpeed, args[1] = tSpeed
			float scrollS = mod.args[0] * time;
			float scrollT = mod.args[1] * time;
			// Clamp to prevent precision issues
			scrollS = scrollS - std::floor(scrollS);
			scrollT = scrollT - std::floor(scrollT);
			outMatrix[4] = scrollS;
			outMatrix[5] = scrollT;
			break;
		}
		case MetalTCModType::Scale: {
			// args[0] = sScale, args[1] = tScale
			outMatrix[0] = mod.args[0];
			outMatrix[3] = mod.args[1];
			break;
		}
		case MetalTCModType::Rotate: {
			// args[0] = degsPerSecond
			float degs = -mod.args[0] * time;
			float rads = degs * static_cast<float>(M_PI) / 180.0f;
			float sinVal = std::sin(rads);
			float cosVal = std::cos(rads);
			// Rotate around (0.5, 0.5)
			outMatrix[0] = cosVal;
			outMatrix[2] = -sinVal;
			outMatrix[4] = 0.5f - 0.5f * cosVal + 0.5f * sinVal;
			outMatrix[1] = sinVal;
			outMatrix[3] = cosVal;
			outMatrix[5] = 0.5f - 0.5f * sinVal - 0.5f * cosVal;
			break;
		}
		case MetalTCModType::Stretch: {
			// wave-based stretch
			float stretchValue = EvalWaveForm(mod.wave, time);
			if (stretchValue != 0.0f) {
				float p = 1.0f / stretchValue;
				outMatrix[0] = p;
				outMatrix[3] = p;
				outMatrix[4] = 0.5f - 0.5f * p;
				outMatrix[5] = 0.5f - 0.5f * p;
			}
			break;
		}
		case MetalTCModType::Turbulence: {
			// Turbulence params stored in args: [0]=base, [1]=amplitude, [2]=phase, [3]=frequency
			// The actual sin() is applied in the shader based on position
			// Note: base is unused in the shader, it just offsets the wave
			turbAmp = mod.args[1];  // amplitude
			turbPhase = mod.args[2] + time * mod.args[3];  // phase + time * frequency
			break;
		}
		case MetalTCModType::Transform: {
			// Direct 2x3 matrix: args[0-5]
			outMatrix[0] = mod.args[0];
			outMatrix[1] = mod.args[1];
			outMatrix[2] = mod.args[2];
			outMatrix[3] = mod.args[3];
			outMatrix[4] = mod.args[4];
			outMatrix[5] = mod.args[5];
			break;
		}
		case MetalTCModType::EntityTranslate:
			// Entity-based translation - would need entity context
			// For now, treat as identity
			break;
		default:
			break;
	}
}

} // end anonymous namespace for tcMod helpers

//=============================================================================
// DeformVertexes Helpers
//=============================================================================

namespace {

// Apply deformVertexes wave to a set of vertices
// This modifies vertex positions based on wave function, like GL2's RB_CalcDeformVertexes
inline void ApplyDeformVertexesWave(
	MetalPolyVertex* vertices, 
	int vertexCount,
	const MetalDeformInfo& deform,
	float time)
{
	if (deform.type != MetalDeformType::Wave) {
		return;
	}
	
	const MetalWaveForm& wave = deform.wave;
	const float spread = deform.spread;
	
	// If frequency is 0, all vertices get the same offset
	if (wave.frequency == 0.0f) {
		float scale = EvalWaveForm(wave, time);
		for (int i = 0; i < vertexCount; ++i) {
			MetalPolyVertex& v = vertices[i];
			v.xyz[0] += v.normal[0] * scale;
			v.xyz[1] += v.normal[1] * scale;
			v.xyz[2] += v.normal[2] * scale;
		}
	} else {
		// Each vertex gets a phase offset based on its position
		// This creates the wave-like ripple effect
		for (int i = 0; i < vertexCount; ++i) {
			MetalPolyVertex& v = vertices[i];
			
			// Calculate position-based phase offset
			// The spread parameter controls how "tight" the waves are
			float posOffset = 0.0f;
			if (spread != 0.0f) {
				posOffset = (v.xyz[0] + v.xyz[1] + v.xyz[2]) / spread;
			}
			
			// Create a modified wave with the position-based phase offset
			MetalWaveForm modWave = wave;
			modWave.phase = wave.phase + posOffset;
			
			float scale = EvalWaveForm(modWave, time);
			
			v.xyz[0] += v.normal[0] * scale;
			v.xyz[1] += v.normal[1] * scale;
			v.xyz[2] += v.normal[2] * scale;
		}
	}
}

// Apply deformVertexes bulge to a set of vertices
// This creates a bulge effect based on texture coordinates
inline void ApplyDeformVertexesBulge(
	MetalPolyVertex* vertices,
	int vertexCount,
	const MetalDeformInfo& deform,
	float time)
{
	if (deform.type != MetalDeformType::Bulge) {
		return;
	}
	
	const float bulgeWidth = deform.bulgeWidth;
	const float bulgeHeight = deform.bulgeHeight;
	const float bulgeSpeed = deform.bulgeSpeed;
	
	const float now = time * bulgeSpeed;
	
	for (int i = 0; i < vertexCount; ++i) {
		MetalPolyVertex& v = vertices[i];
		
		// Calculate offset based on S texture coordinate and time
		// GL2 uses: off = (FUNCTABLE_SIZE / (M_PI*2)) * (st[0] * bulgeWidth + now)
		// Then: scale = sinTable[off & FUNCTABLE_MASK] * bulgeHeight
		// We approximate with direct sin calculation
		float off = v.st[0] * bulgeWidth + now;
		float scale = std::sin(off) * bulgeHeight;
		
		v.xyz[0] += v.normal[0] * scale;
		v.xyz[1] += v.normal[1] * scale;
		v.xyz[2] += v.normal[2] * scale;
	}
}

} // end anonymous namespace for deformVertexes helpers

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
	
	int lerpTag(orientation_t* tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char* tagName);
	void modelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);

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
		int fogIndex = 0;  // 0 = no fog, 1+ = fog volume index (1-based)
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

		// Fog parameters
		float fogDistanceVector[4] = {};  // Distance from eye to fog volume
		float fogDepthVector[4] = {};     // Fog surface plane equation
		float fogColor[4] = {};           // Fog RGB color + alpha
		float fogSurface[4] = {};         // Fog surface normal and distance
		float fogBoundsMin[4] = {};       // Fog volume min bounds (xyz)
		float fogBoundsMax[4] = {};       // Fog volume max bounds (xyz)
		float fogEyeT = 0.0f;             // Eye position relative to fog surface
		float fogTcScale = 0.0f;          // Texture coordinate scale
		float fogHasSurface = 0.0f;       // Whether fog has a visible surface plane
		float fogEnabled = 0.0f;          // Enable/disable fog rendering
		float overBrightBits = 0.0f;      // r_overBrightBits value
	};

	// Texture coordinate modification params (matches Metal shader TCModParams)
	struct TCModParams {
		float texMatrix0[4] = {1.0f, 0.0f, 0.0f, 0.0f};  // Identity: scaleX=1, shearX=0, translateX=0, turbAmp=0
		float texMatrix1[4] = {0.0f, 1.0f, 0.0f, 0.0f};  // Identity: shearY=0, scaleY=1, translateY=0, turbPhase=0
		float texMatrix2[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		float texMatrix3[4] = {0.0f, 1.0f, 0.0f, 0.0f};
		float texMatrix4[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		float texMatrix5[4] = {0.0f, 1.0f, 0.0f, 0.0f};
		float texMatrix6[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		float texMatrix7[4] = {0.0f, 1.0f, 0.0f, 0.0f};
	};

	// Entity lighting parameters - matches scene.metal EntityLightingParams
	struct EntityLightingParams {
		float ambientLight[3] = {150.0f, 150.0f, 150.0f};
		float padding0 = 0.0f;
		float directedLight[3] = {150.0f, 150.0f, 150.0f};
		float padding1 = 0.0f;
		float lightDir[3] = {0.0f, 0.0f, 1.0f};
		float padding2 = 0.0f;
		float modelLightDir[3] = {0.0f, 0.0f, 1.0f};
		float overBrightBits = 1.0f;  // 1 << overbrightBits = overbright multiplier
	};

	struct ModelFogParams {
		float fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		float fogDistanceVector[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		float fogDepthVector[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		float fogEyeT = 0.0f;
		float fogTcScale = 0.0f;
		float fogEnabled = 0.0f;
		float fogHasSurface = 0.0f;
	};

	static_assert(sizeof(ModelFogParams) % 16 == 0, "ModelFogParams must be 16-byte aligned");

	// Model stage parameters - matches scene.metal ModelStageParams
	// Includes tcMod matrices for animated texture effects
	struct ModelStageParams {
		float tcGenType = 0.0f;  // 0 = Texture, 1 = Lightmap, 2 = Environment
		float padding[3] = {0.0f, 0.0f, 0.0f};
		// TCMod matrices (same format as TCModParams)
		float texMatrix0[4] = {1.0f, 0.0f, 0.0f, 0.0f};  // Identity: scaleX=1, shearX=0, translateX=0, turbAmp=0
		float texMatrix1[4] = {0.0f, 1.0f, 0.0f, 0.0f};  // Identity: shearY=0, scaleY=1, translateY=0, turbPhase=0
		float texMatrix2[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		float texMatrix3[4] = {0.0f, 1.0f, 0.0f, 0.0f};
		float texMatrix4[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		float texMatrix5[4] = {0.0f, 1.0f, 0.0f, 0.0f};
		float texMatrix6[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		float texMatrix7[4] = {0.0f, 1.0f, 0.0f, 0.0f};
	};

	static_assert(sizeof(ModelStageParams) % 16 == 0, "ModelStageParams must be 16-byte aligned");

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
		float frustumPlanes[6][4]{};
		bool frustumValid = false;
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
		
		// Skybox textures (6 faces: rt, lf, ft, bk, up, dn)
		std::array<qhandle_t, 6> skyboxOuterHandles = {0, 0, 0, 0, 0, 0};
		std::array<qhandle_t, 6> skyboxInnerHandles = {0, 0, 0, 0, 0, 0};
		bool skyboxLoaded = false;
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

	cvar_t* r_lodBias_ = nullptr;
	cvar_t* r_lodScale_ = nullptr;
	cvar_t* r_znear_ = nullptr;
	cvar_t* r_zproj_ = nullptr;
	cvar_t* r_stereoSeparation_ = nullptr;
	cvar_t* r_metalLogCalls_ = nullptr;
	cvar_t* r_swapInterval_ = nullptr;
	cvar_t* r_mapOverBrightBits_ = nullptr;
	cvar_t* r_overBrightBits_ = nullptr;
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

	// Dynamic lighting resources
	MetalPtr<MTL::Function> dlightVertexFunction_;
	MetalPtr<MTL::Function> dlightFragmentFunction_;
	MetalPtr<MTL::RenderPipelineState> dlightPipeline_;
	MetalPtr<MTL::Texture> dlightTexture_;  // Radial falloff texture


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
	// Fog rendering
	struct FogVolume {
		int originalBrushNumber;
		float bounds[2][3];  // min, max
		unsigned colorInt;
		float tcScale;
		bool hasSurface;
		float surface[4];    // plane equation
		float fogColor[3];
		float depthForOpaque;
	};
	std::vector<FogVolume> worldFogs_;
	MetalPtr<MTL::RenderPipelineState> fogPipeline_;
	MetalPtr<MTL::Function> fogVertexFunction_;
	MetalPtr<MTL::Function> fogFragmentFunction_;
	MetalPtr<MTL::DepthStencilState> fogDepthState_;

	// Model rendering (MD3)
	MetalPtr<MTL::RenderPipelineState> modelPipeline_;           // Standard alpha blend
	MetalPtr<MTL::RenderPipelineState> modelPipelineAdditive_;   // Additive blend (GL_ONE, GL_ONE)
	MetalPtr<MTL::Function> modelVertexFunction_;
	MetalPtr<MTL::Function> modelFragmentFunction_;
	MetalPtr<MTL::VertexDescriptor> modelVertexDescriptor_;
	MetalPtr<MTL::DepthStencilState> modelDepthState_;
	MetalPtr<MTL::DepthStencilState> modelDepthStateNoWrite_;    // For additive surfaces (no depth write)
	
	// Model stage pipeline cache for multi-pass rendering
	struct ModelStagePipelineEntry {
		MetalShaderResource::MetalPipelineKey key;
		MetalPtr<MTL::RenderPipelineState> pipeline;
		MetalPtr<MTL::DepthStencilState> depthState;

		ModelStagePipelineEntry() = default;
		ModelStagePipelineEntry(ModelStagePipelineEntry&&) = default;
		ModelStagePipelineEntry& operator=(ModelStagePipelineEntry&&) = default;
		ModelStagePipelineEntry(const ModelStagePipelineEntry&) = delete;
		ModelStagePipelineEntry& operator=(const ModelStagePipelineEntry&) = delete;
	};
	std::unordered_map<MetalShaderResource::MetalPipelineKey, ModelStagePipelineEntry, MetalPipelineKeyHash> modelStagePipelineCache_;

	// Skybox and cloud sky dome rendering
	bool skyboxRenderedThisFrame_ = false;
	MetalPtr<MTL::Buffer> skyboxVertexBuffer_;
	MetalPtr<MTL::DepthStencilState> skyboxDepthState_;
	static constexpr size_t SKYBOX_VERTEX_COUNT = 36;  // 6 faces * 6 vertices per face (2 triangles)
	
	// Cloud sky dome data (like OpenGL's R_BuildCloudData)
	static constexpr int SKY_SUBDIVISIONS = 8;
	static constexpr int HALF_SKY_SUBDIVISIONS = SKY_SUBDIVISIONS / 2;
	std::vector<MetalPolyVertex> cloudDomeVertices_;
	std::vector<uint32_t> cloudDomeIndices_;
	float cloudTexCoords_[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];  // Pre-computed cloud tex coords
	bool cloudTexCoordsInitialized_ = false;
	qhandle_t lastCloudSkyShader_ = 0;

	bool worldLoaded_ = false;
	std::string worldName_;
	std::vector<MetalPolyVertex> worldVertexTemplate_;
	std::vector<ScenePolyPacket> worldPacketTemplate_;
	std::vector<qhandle_t> worldLightmapHandles_;
	std::vector<int> worldSurfaceToPacket_;  // Maps BSP surface index to packet index (-1 if skipped)
	std::vector<MetalBrushModel> brushModels_;  // Brush models (inline BSP models)
	int numWorldSurfaces_ = 0;  // Number of surfaces in model 0 (the world itself)
	std::vector<MetalModel*> models_;  // Model storage (index 0 is reserved for BAD model)
	std::unordered_map<std::string, qhandle_t> modelLookup_;
	std::vector<MetalSkin> registeredSkins_;  // Skin storage (index 0 is reserved)
	std::unordered_map<std::string, qhandle_t> skinLookup_;
	std::vector<MetalShaderResource> shaderResources_;
	std::unordered_map<std::string, qhandle_t> shaderLookup_;

	bool initializeWindow(int& width, int& height, qboolean& fullscreen);
	bool createPipeline(MTL::Texture* drawableTexture);
	bool create2DPipeline();
	bool ensureDepthTexture(int width, int height);
	bool ensureDlightResources();
	void createDlightTexture();
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
	bool ensureFogPipeline();
	bool ensureModelPipeline();
	enum class CullResult { Out, In, Clip };
	void renderModel(const refEntity_t& ent, MetalModel& model, MetalModelLOD& lodData,
	                int fogIndex, CullResult cullState);
	void renderBrushModel(const refEntity_t& ent, MetalBrushModel& bmodel);
	void renderModelSurface(const refEntity_t& ent, MetalModelSurface& surface,
	                        const float* mvpMatrix, const float* modelMatrix, float vertexLerp,
	                        const ModelFogParams& fogParams,
	                        const vec3_t ambientLight, const vec3_t directedLight, const vec3_t lightDir);
	MTL::Buffer* createModelVertexBuffer(const refEntity_t& ent, MetalModelSurface& surface);
	void calculateEntityTransform(const refEntity_t& ent, float* matrix);
	void setupEntityLighting(const refEntity_t& ent, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
	void multiplyMatrices4x4(const float* a, const float* b, const float* c, float* result);
	StagePipelineEntry* getStagePipeline(const MetalShaderResource::MetalPipelineKey& key);
	ModelStagePipelineEntry* getModelStagePipeline(const MetalShaderResource::MetalPipelineKey& key);
	void resetStagePipelineCache();
	bool drawPolyPackets();
	bool drawModelEntities();
	bool drawDynamicLights();
	bool drawFogPasses();
	bool ensureSkyboxResources();
	void loadSkyboxTextures(MetalShaderResource& resource);
	void buildSkyboxVertexBuffer();
	bool drawSkybox(qhandle_t skyShader);
	void initCloudSkyTexCoords(float cloudHeight);
	void buildCloudSkyDome(qhandle_t skyShader);
	bool drawCloudSky(qhandle_t skyShader);
	void buildFrustumPlanes(SceneCamera& camera);
	float projectRadius(float radius, const vec3_t location) const;
	CullResult cullBoundingSphere(const vec3_t center, float radius) const;
	void transformModelPoint(const refEntity_t& ent, const float point[3], vec3_t out) const;
	CullResult cullBoundingBox(const float mins[3], const float maxs[3], const refEntity_t& ent) const;
	CullResult cullModel(const MetalModelLOD& lod, const refEntity_t& ent) const;
	int computeModelLod(const MetalModel& model, const refEntity_t& ent) const;
	int computeModelFogIndex(const MetalModelLOD& lod, const refEntity_t& ent) const;
	ModelFogParams buildModelFogParams(int fogIndex) const;
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
	MetalSkin* getSkinByHandle(qhandle_t handle);
	static MTL::BlendFactor ToMetalBlendFactor(MetalBlendFactor factor);
	TCModParams computeTCModParams(const MetalShaderStageInfo* stageInfo, float timeSeconds) const;
};

//=============================================================================
// Implementation
//=============================================================================



void MetalRenderer::shutdown(qboolean destroyWindow) {
	unloadWorldMap();
	unregisterConsoleCommands();

	// Shutdown lighting system
	R_ShutdownLightingSystem();

	// Free all loaded models
	for (MetalModel* model : models_) {
		if (model) {
			// Free GPU buffers first
			MetalModel_FreeGPUBuffers(model);
			// Then free the model itself
			MetalModel_Free(model);
		}
	}
	models_.clear();
	modelLookup_.clear();

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
	fogVertexFunction_.reset();
	fogFragmentFunction_.reset();
	fogPipeline_.reset();
	dlightVertexFunction_.reset();
	dlightFragmentFunction_.reset();
	dlightPipeline_.reset();
	dlightTexture_.reset();
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
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
	
	// Copy template vertices - we may modify them for deformVertexes
	polyVertices_.insert(polyVertices_.end(), worldVertexTemplate_.begin(), worldVertexTemplate_.end());
	
	for (const ScenePolyPacket& templatePacket : worldPacketTemplate_) {
		ScenePolyPacket packet = templatePacket;
		packet.firstVertex += static_cast<int>(baseVertex);
		
		// Check if this shader has deformVertexes and apply it
		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (shaderResource && shaderResource->hasScript && shaderResource->script.hasDeform) {
			const MetalDeformInfo& deform = shaderResource->script.deform;
			
			if (deform.type == MetalDeformType::Wave || deform.type == MetalDeformType::Bulge) {
				// Get pointer to the vertices for this packet
				MetalPolyVertex* packetVerts = &polyVertices_[packet.firstVertex];
				
				if (deform.type == MetalDeformType::Wave) {
					ApplyDeformVertexesWave(packetVerts, packet.vertexCount, deform, sceneTimeSeconds);
				} else if (deform.type == MetalDeformType::Bulge) {
					ApplyDeformVertexesBulge(packetVerts, packet.vertexCount, deform, sceneTimeSeconds);
				}
			}
		}
		
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
	int fogCount = 0;
	const dfog_t* fogs = reinterpret_cast<const dfog_t*>(getLumpRange(LUMP_FOGS, fogCount, sizeof(dfog_t)));

	// Load planes, brushes, and brush sides for fog surface calculation
	int planeCount = 0;
	const dplane_t* planes = reinterpret_cast<const dplane_t*>(getLumpRange(LUMP_PLANES, planeCount, sizeof(dplane_t)));
	int brushCount = 0;
	const dbrush_t* brushes = reinterpret_cast<const dbrush_t*>(getLumpRange(LUMP_BRUSHES, brushCount, sizeof(dbrush_t)));
	int brushSideCount = 0;
	const dbrushside_t* brushSides = reinterpret_cast<const dbrushside_t*>(getLumpRange(LUMP_BRUSHSIDES, brushSideCount, sizeof(dbrushside_t)));

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

		// Calculate overbright shift
		// Must match OpenGL2's R_ColorShiftLightingBytes: shift = mapOverBrightBits - overBrightBits
		int mapOverBrightBits = r_mapOverBrightBits_ ? r_mapOverBrightBits_->integer : 2;
		int overBrightBits = r_overBrightBits_ ? r_overBrightBits_->integer : 1;
		if (mapOverBrightBits < 0) mapOverBrightBits = 0;
		if (mapOverBrightBits > 4) mapOverBrightBits = 4;
		if (overBrightBits < 0) overBrightBits = 0;
		if (overBrightBits > 4) overBrightBits = 4;
		const int shift = mapOverBrightBits - overBrightBits;  // CRITICAL: Must subtract, not use mapOverBrightBits directly!

		for (int lm = 0; lm < lightmapCount; ++lm) {
			const byte* src = lightmapData + static_cast<size_t>(lm) * kBspLightmapBytes;
			for (int pix = 0; pix < pixelCount; ++pix) {
				int r = src[static_cast<size_t>(pix) * 3 + 0];
				int g = src[static_cast<size_t>(pix) * 3 + 1];
				int b = src[static_cast<size_t>(pix) * 3 + 2];

				// Apply overbright scaling
				r <<= shift;
				g <<= shift;
				b <<= shift;

				// Normalize if overflowing to preserve color
				int max = (r > g) ? r : g;
				max = (max > b) ? max : b;
				if (max > 255) {
					r = r * 255 / max;
					g = g * 255 / max;
					b = b * 255 / max;
				}

				rgba[static_cast<size_t>(pix) * 4 + 0] = static_cast<byte>(r);
				rgba[static_cast<size_t>(pix) * 4 + 1] = static_cast<byte>(g);
				rgba[static_cast<size_t>(pix) * 4 + 2] = static_cast<byte>(b);
				rgba[static_cast<size_t>(pix) * 4 + 3] = 255;
			}
			char texName[MAX_QPATH];
			std::snprintf(texName, sizeof(texName), "%s/lightmap_%d", requestedName.c_str(), lm);
			qhandle_t handle = textureManager->registerRawImage(texName, rgba.data(), kBspLightmapWidth, kBspLightmapHeight, false);
			worldLightmapHandles_.push_back(handle);
		}
	}

	// Load fog volumes
	worldFogs_.clear();
	if (fogs && fogCount > 0) {
		worldFogs_.reserve(fogCount);
		for (int i = 0; i < fogCount; ++i) {
			const dfog_t& dfog = fogs[i];
			FogVolume fog{};

			const int brushNum = LittleLong(dfog.brushNum);
			fog.originalBrushNumber = brushNum;

			// Register the fog shader to get its fog parameters
			std::string fogShaderName(dfog.shader);
			qhandle_t fogShaderHandle = registerShader(fogShaderName.c_str(), true);
			MetalShaderResource* fogShader = getShaderResource(fogShaderHandle);

			if (fogShader && fogShader->hasScript && fogShader->script.hasFogParms) {
				fog.fogColor[0] = fogShader->script.fogColor[0];
				fog.fogColor[1] = fogShader->script.fogColor[1];
				fog.fogColor[2] = fogShader->script.fogColor[2];
				fog.depthForOpaque = fogShader->script.fogDepthForOpaque;

				// Calculate tcScale from depth
				const float depth = (fog.depthForOpaque < 1.0f) ? 1.0f : fog.depthForOpaque;
				fog.tcScale = 1.0f / (depth * 8.0f);

				// Pack color into int for compatibility (RGBA)
				fog.colorInt = (static_cast<unsigned>(fog.fogColor[0] * 255.0f) << 0) |
				               (static_cast<unsigned>(fog.fogColor[1] * 255.0f) << 8) |
				               (static_cast<unsigned>(fog.fogColor[2] * 255.0f) << 16) |
				               (255u << 24);

				// Initialize fog bounds and surface
				fog.hasSurface = false;
				fog.surface[0] = fog.surface[1] = fog.surface[2] = fog.surface[3] = 0.0f;
				fog.bounds[0][0] = fog.bounds[0][1] = fog.bounds[0][2] = 0.0f;
				fog.bounds[1][0] = fog.bounds[1][1] = fog.bounds[1][2] = 0.0f;

				// Extract fog bounds and surface plane from BSP brush/plane data
				if (brushes && brushSides && planes && brushNum >= 0 && brushNum < brushCount) {
					const dbrush_t& brush = brushes[brushNum];
					const int firstSide = LittleLong(brush.firstSide);
					const int numSides = LittleLong(brush.numSides);

					// Brushes have axial sides first (6 sides for bounds)
					if (numSides >= 6 && firstSide >= 0 && (firstSide + 5) < brushSideCount) {
						// Extract bounds from the 6 axial brush sides
						// Side 0: -X plane, Side 1: +X plane
						// Side 2: -Y plane, Side 3: +Y plane
						// Side 4: -Z plane, Side 5: +Z plane
						int planeNum;
						
						planeNum = LittleLong(brushSides[firstSide + 0].planeNum);
						if (planeNum >= 0 && planeNum < planeCount)
							fog.bounds[0][0] = -planes[planeNum].dist;
						
						planeNum = LittleLong(brushSides[firstSide + 1].planeNum);
						if (planeNum >= 0 && planeNum < planeCount)
							fog.bounds[1][0] = planes[planeNum].dist;
						
						planeNum = LittleLong(brushSides[firstSide + 2].planeNum);
						if (planeNum >= 0 && planeNum < planeCount)
							fog.bounds[0][1] = -planes[planeNum].dist;
						
						planeNum = LittleLong(brushSides[firstSide + 3].planeNum);
						if (planeNum >= 0 && planeNum < planeCount)
							fog.bounds[1][1] = planes[planeNum].dist;
						
						planeNum = LittleLong(brushSides[firstSide + 4].planeNum);
						if (planeNum >= 0 && planeNum < planeCount)
							fog.bounds[0][2] = -planes[planeNum].dist;
						
						planeNum = LittleLong(brushSides[firstSide + 5].planeNum);
						if (planeNum >= 0 && planeNum < planeCount)

						if (ri_.Printf) {
							ri_.Printf(PRINT_ALL, "Metal: Fog %d bounds: min(%.1f, %.1f, %.1f) max(%.1f, %.1f, %.1f)\n",
								i, fog.bounds[0][0], fog.bounds[0][1], fog.bounds[0][2],
								fog.bounds[1][0], fog.bounds[1][1], fog.bounds[1][2]);
						}
					}

					// Extract visible surface plane
					const int visibleSide = LittleLong(dfog.visibleSide);
					if (visibleSide != -1 && visibleSide < numSides) {
						const int sideIndex = firstSide + visibleSide;
						if (sideIndex >= 0 && sideIndex < brushSideCount) {
							const dbrushside_t& side = brushSides[sideIndex];
							const int planeNum = LittleLong(side.planeNum);

							if (planeNum >= 0 && planeNum < planeCount) {
								const dplane_t& plane = planes[planeNum];
								// Store surface plane: negate normal as per OpenGL renderer
								fog.surface[0] = -plane.normal[0];
								fog.surface[1] = -plane.normal[1];
								fog.surface[2] = -plane.normal[2];
								fog.surface[3] = -plane.dist;
								fog.hasSurface = true;

								if (ri_.Printf) {
									ri_.Printf(PRINT_ALL, "Metal: Fog %d surface plane: (%.2f, %.2f, %.2f, %.2f)\n",
										i, fog.surface[0], fog.surface[1], fog.surface[2], fog.surface[3]);
								}
							}
						}
					}
				}

				worldFogs_.push_back(fog);

				if (ri_.Printf) {
					ri_.Printf(PRINT_ALL, "Metal: Loaded fog %d: shader='%s', color=(%.2f,%.2f,%.2f), depth=%.1f, hasSurface=%d\n",
						i, fogShaderName.c_str(),
						fog.fogColor[0], fog.fogColor[1], fog.fogColor[2],
						fog.depthForOpaque, fog.hasSurface ? 1 : 0);
				}
			}
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

	// Get overbright bits for vertex color shifting (matches OpenGL2)
	int mapOverBrightBits = r_mapOverBrightBits_ ? r_mapOverBrightBits_->integer : 2;
	int overBrightBits = r_overBrightBits_ ? r_overBrightBits_->integer : 1;
	if (mapOverBrightBits < 0) mapOverBrightBits = 0;
	if (mapOverBrightBits > 4) mapOverBrightBits = 4;
	if (overBrightBits < 0) overBrightBits = 0;
	if (overBrightBits > 4) overBrightBits = 4;

	worldVertexTemplate_.clear();
	worldPacketTemplate_.clear();
	worldSurfaceToPacket_.clear();
	brushModels_.clear();
	numWorldSurfaces_ = 0;
	worldVertexTemplate_.reserve(static_cast<size_t>(vertexCount) * 2);
	worldPacketTemplate_.reserve(static_cast<size_t>(surfaceCount));
	worldSurfaceToPacket_.resize(surfaceCount, -1);  // -1 means surface was skipped

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

		// Check if this is a fog volume shader
		// Fog shaders with no texture stages should be skipped (volumetric fog only)
		// Fog shaders WITH texture stages (like hellfog with animated clouds) should be rendered
		MetalShaderResource* shaderResource = getShaderResource(shaderHandle);
		if (shaderResource && shaderResource->hasScript && shaderResource->script.hasFogParms) {
			// Check if shader has any actual texture stages to render
			// Note: stageRuntimes isn't built yet, check script.stages instead
			if (shaderResource->script.stages.empty()) {
				// Pure fog volume with no visible surface - skip it
				// The volumetric fog is handled by drawFogPasses()
				continue;
			}
			// Has texture stages - render the animated fog surface
			if (ri_.Printf) {
				ri_.Printf(PRINT_ALL, "Metal: Fog shader '%s' has %zu stages, will render surface\n",
					shaderResource->name.c_str(), shaderResource->script.stages.size());
			}
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
			TessellateBezierPatch(&drawVerts[firstVert], patchWidth, patchHeight, patchVerts, PATCH_SUBDIVISIONS, mapOverBrightBits, overBrightBits);

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
				worldVertexTemplate_.push_back(ConvertDrawVert(drawVerts[idx0], mapOverBrightBits, overBrightBits));
				worldVertexTemplate_.push_back(ConvertDrawVert(drawVerts[idx1], mapOverBrightBits, overBrightBits));
				worldVertexTemplate_.push_back(ConvertDrawVert(drawVerts[idx2], mapOverBrightBits, overBrightBits));
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
		// Store fog index for this surface (add 1 to match OpenGL convention: 0 = no fog)
		packet.fogIndex = LittleLong(ds.fogNum) + 1;

		// Store lightmap handle
		int lightmapNum = LittleLong(ds.lightmapNum);
		if (lightmapNum >= 0 && lightmapNum < static_cast<int>(worldLightmapHandles_.size())) {
			packet.lightmapHandle = worldLightmapHandles_[lightmapNum];
		} else {
			packet.lightmapHandle = 0;
		}
		// Record mapping from BSP surface index to packet index
		worldSurfaceToPacket_[surfaceIndex] = static_cast<int>(worldPacketTemplate_.size());
		worldPacketTemplate_.push_back(packet);
	}

	// Load brush models (inline BSP models for doors, platforms, etc.)
	{
		int modelsLen = 0;
		const dmodel_t* bspModels = reinterpret_cast<const dmodel_t*>(getLumpRange(LUMP_MODELS, modelsLen, sizeof(dmodel_t)));
		if (bspModels && modelsLen >= 1) {
			brushModels_.resize(modelsLen);
			
			for (int i = 0; i < modelsLen; i++) {
				const dmodel_t& dm = bspModels[i];
				MetalBrushModel& bm = brushModels_[i];
				
				// Copy bounds
				for (int j = 0; j < 3; j++) {
					bm.bounds[0][j] = LittleFloat(dm.mins[j]);
					bm.bounds[1][j] = LittleFloat(dm.maxs[j]);
				}
				
				// Store surface range (these are BSP surface indices)
				bm.firstSurface = LittleLong(dm.firstSurface);
				bm.numSurfaces = LittleLong(dm.numSurfaces);
				
				if (i == 0) {
					// Model 0 is the world itself - record how many surfaces it has
					numWorldSurfaces_ = bm.numSurfaces;
				}
				
				// Register the brush model as a model handle (for inline models like *1, *2, etc.)
				if (i > 0) {
					char modelName[16];
					Com_sprintf(modelName, sizeof(modelName), "*%d", i);
					
					// Allocate a new model entry
					const qhandle_t handle = static_cast<qhandle_t>(models_.size());
					MetalModel* model = MetalModel_Alloc(handle);
					std::strncpy(model->name, modelName, sizeof(model->name) - 1);
					model->name[sizeof(model->name) - 1] = '\0';
					model->type = MetalModelType::BRUSH;
					model->bmodel = &brushModels_[i];
					
					models_.push_back(model);
					modelLookup_[std::string(modelName)] = handle;
				}
			}
			
			if (ri_.Printf && modelsLen > 1) {
				ri_.Printf(PRINT_ALL, "Metal: Loaded %d brush models (doors, platforms, movers)\n", modelsLen - 1);
			}
		}
	}

	// Load lightGrid for entity lighting
	// First parse entities to get gridSize, then load the grid data
	{
		vec3_t gridSize = {64.0f, 64.0f, 128.0f}; // Default grid size
		vec3_t gridOrigin{};
		int gridBounds[3] = {0, 0, 0};

		// Parse entities lump to find worldspawn gridsize
		int entitiesLen = 0;
		const char* entitiesData = reinterpret_cast<const char*>(getLumpRange(LUMP_ENTITIES, entitiesLen, 1));
		if (entitiesData && entitiesLen > 0) {
			// Simple parser to find "gridsize" key in worldspawn
			const char* p = entitiesData;
			const char* end = entitiesData + entitiesLen;
			while (p < end && *p) {
				// Skip whitespace
				while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
				if (p >= end) break;

				// Look for "gridsize" token
				if (strncmp(p, "\"gridsize\"", 10) == 0) {
					p += 10;
					// Skip to value
					while (p < end && *p != '"') p++;
					if (p < end && *p == '"') {
						p++;
						const char* valueStart = p;
						while (p < end && *p != '"') p++;
						if (p > valueStart) {
							char valueStr[64];
							size_t valueLen = std::min(size_t(p - valueStart), sizeof(valueStr) - 1);
							memcpy(valueStr, valueStart, valueLen);
							valueStr[valueLen] = '\0';
							sscanf(valueStr, "%f %f %f", &gridSize[0], &gridSize[1], &gridSize[2]);
							break;
						}
					}
				}
				// Skip to next line
				while (p < end && *p != '\n') p++;
			}
		}

		// Load lightGrid lump
		int lightGridLen = 0;
		const byte* lightGridData = getLumpRange(LUMP_LIGHTGRID, lightGridLen, 1);

		if (lightGridData && lightGridLen > 0) {
			// Get world bounds from first bmodel (the world itself) in LUMP_MODELS
			vec3_t worldMins, worldMaxs;
			int modelsLen = 0;
			const dmodel_t* models = reinterpret_cast<const dmodel_t*>(getLumpRange(LUMP_MODELS, modelsLen, sizeof(dmodel_t)));

			if (models && modelsLen >= 1) {
				// First model is always the world
				VectorCopy(models[0].mins, worldMins);
				VectorCopy(models[0].maxs, worldMaxs);
			} else {
				// Fallback to large default if we can't get bounds
				VectorSet(worldMins, -4096, -4096, -4096);
				VectorSet(worldMaxs, 4096, 4096, 4096);
				if (ri_.Printf) {
					ri_.Printf(PRINT_WARNING, "Metal: Could not read LUMP_MODELS, using default world bounds\n");
				}
			}

			// Calculate grid origin and bounds (same algorithm as OpenGL2 renderer)
			for (int i = 0; i < 3; i++) {
				gridOrigin[i] = gridSize[i] * ceil(worldMins[i] / gridSize[i]);
				float maxs = gridSize[i] * floor(worldMaxs[i] / gridSize[i]);
				gridBounds[i] = (int)((maxs - gridOrigin[i]) / gridSize[i]) + 1;
			}

			int numGridPoints = gridBounds[0] * gridBounds[1] * gridBounds[2];
			int expectedSize = numGridPoints * 8; // 8 bytes per grid point

			if (lightGridLen == expectedSize) {
				// Valid lightGrid data - pass to lighting system
				R_LoadLightGrid(lightGridData, lightGridLen, nullptr, 0,
				                gridOrigin, gridSize, gridBounds);
				if (ri_.Printf) {
					ri_.Printf(PRINT_ALL, "^2Metal: lightGrid loaded successfully (%d points, bounds %dx%dx%d)\n",
					           numGridPoints, gridBounds[0], gridBounds[1], gridBounds[2]);
				}
			} else if (ri_.Printf) {
				ri_.Printf(PRINT_ALL, "^3Metal: lightGrid size mismatch (got %d, expected %d)\n",
				           lightGridLen, expectedSize);
				ri_.Printf(PRINT_ALL, "^3  gridSize=(%.1f %.1f %.1f), gridBounds=(%d %d %d)\n",
				           gridSize[0], gridSize[1], gridSize[2],
				           gridBounds[0], gridBounds[1], gridBounds[2]);
				ri_.Printf(PRINT_ALL, "^3  worldMins=(%.1f %.1f %.1f), worldMaxs=(%.1f %.1f %.1f)\n",
				           worldMins[0], worldMins[1], worldMins[2],
				           worldMaxs[0], worldMaxs[1], worldMaxs[2]);
			}
		}
	}

	ri_.FS_FreeFile(buffer);
	worldLoaded_ = !worldPacketTemplate_.empty();
	if (worldLoaded_) {
		worldName_ = requestedName;
		if (ri_.Printf) {
			// Count surfaces with fog
			int foggedSurfaces = 0;
			for (const auto& p : worldPacketTemplate_) {
				if (p.fogIndex > 0) foggedSurfaces++;
			}
			ri_.Printf(PRINT_ALL, "Metal: loaded world '%s' (%zu surfaces, %d fogged, %zu verts)\n",
			           worldName_.c_str(),
			           worldPacketTemplate_.size(),
			           foggedSurfaces,
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
	worldSurfaceToPacket_.clear();
	brushModels_.clear();
	numWorldSurfaces_ = 0;
	worldFogs_.clear();
	
	// Remove brush models from model registry
	// They become invalid when the world is unloaded
	for (auto it = modelLookup_.begin(); it != modelLookup_.end(); ) {
		if (!it->first.empty() && it->first[0] == '*') {
			it = modelLookup_.erase(it);
		} else {
			++it;
		}
	}
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
		// Debug: Log when shader script is found with blend info
		if (ri_.Printf && !info.stages.empty()) {
			const auto& firstStage = info.stages[0];
			ri_.Printf(PRINT_DEVELOPER, "SHADER_SCRIPT: '%s' found script, stages=%zu, stage0_src=%d dst=%d\n",
			          normalized.c_str(), info.stages.size(),
			          static_cast<int>(firstStage.srcBlendFactor),
			          static_cast<int>(firstStage.dstBlendFactor));
		}
	} else {
		resource.hasScript = false;
		resource.script.imagePaths.push_back(normalized);
		resource.script.forceOpaque = qtrue;
		// Debug: Log when NO shader script is found for known problematic shaders
		if (ri_.Printf && (normalized.find("plasma") != std::string::npos ||
		                    normalized.find("shard") != std::string::npos ||
		                    normalized.find("ammo") != std::string::npos)) {
			ri_.Printf(PRINT_ALL, "^1NO_SCRIPT: '%s' (from raw '%s') - using default opaque\n",
			          normalized.c_str(), name);
		}
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
		ri_.Printf(PRINT_ALL, "MetalRenderer::registerModel: NULL name\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH) {
		ri_.Printf(PRINT_ALL, "Model name exceeds MAX_QPATH\n");
		return 0;
	}

	// Search currently loaded models
	const std::string key(name);
	auto it = modelLookup_.find(key);
	if (it != modelLookup_.end()) {
		MetalModel* mod = models_[it->second];
		if (mod->type == MetalModelType::BAD) {
			return 0;  // Failed load cached
		}
		return it->second;
	}

	// Handle inline brush models (names like "*1", "*2", etc.)
	// These should have been registered during world load
	if (name[0] == '*') {
		// Inline model not found - this means the world isn't loaded yet
		// or the model index is invalid
		ri_.Printf(PRINT_DEVELOPER, "Metal: Inline model '%s' not found (world may not be loaded)\n", name);
		return 0;
	}

	// Allocate a new model
	const qhandle_t handle = static_cast<qhandle_t>(models_.size());
	MetalModel* model = MetalModel_Alloc(handle);
	std::strncpy(model->name, name, sizeof(model->name) - 1);
	model->name[sizeof(model->name) - 1] = '\0';

	models_.push_back(model);
	modelLookup_[key] = handle;

	// Determine model type by extension
	const char* ext = strrchr(name, '.');
	qhandle_t result = 0;

	if (ext && !Q_stricmp(ext, ".md3")) {
		// Load MD3 model
		result = MetalModel_RegisterMD3(name, model, ri_);
	} else {
		// Try MD3 as default if no extension
		char namebuf[MAX_QPATH];
		Com_sprintf(namebuf, sizeof(namebuf), "%s.md3", name);
		result = MetalModel_RegisterMD3(namebuf, model, ri_);

		if (!result) {
			// Try other formats in the future (MDR, IQM)
			ri_.Printf(PRINT_WARNING, "MetalRenderer::registerModel: couldn't load %s\n", name);
			model->type = MetalModelType::BAD;
		}
	}

	if (result) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: loaded model '%s' as handle %d\n", name, handle);

		// Create GPU buffers for the model
		if (device_) {
			if (!MetalModel_CreateGPUBuffers(model, device_.get())) {
				ri_.Printf(PRINT_WARNING, "Metal: Failed to create GPU buffers for model '%s'\n", name);
			}
		}

		// Register shaders for all surfaces in all LODs
		// This matches OpenGL2's approach where shaders are registered during model load
		for (int lod = 0; lod < model->numLods; lod++) {
			MetalModelLOD* lodData = model->lods[lod];
			if (!lodData) continue;

			for (int surf = 0; surf < lodData->numSurfaces; surf++) {
				MetalModelSurface& surface = lodData->surfaces[surf];

				// Register each shader name and store the handle
				for (size_t i = 0; i < surface.shaderNames.size(); i++) {
					const std::string& shaderName = surface.shaderNames[i];
					if (!shaderName.empty()) {
						qhandle_t shaderHandle = registerShader(shaderName.c_str(), true);
					surface.shaderIndexes[i] = shaderHandle;

					if (ri_.Printf && shaderHandle > 0 && shaderHandle < shaderResources_.size()) {
						const MetalShaderResource& res = shaderResources_[shaderHandle];
						ri_.Printf(PRINT_ALL, "MD3: Registered shader '%s' -> handle %d, primaryImage %d for model '%s' surf %d\n",
						          shaderName.c_str(), shaderHandle, res.primaryImageHandle, name, surf);
					}
					}
				}
			}
		}
	}

	return result;
}

// Get skin by handle (matches OpenGL2's R_GetSkinByHandle)
MetalSkin* MetalRenderer::getSkinByHandle(qhandle_t handle) {
	if (handle < 1 || handle > static_cast<qhandle_t>(registeredSkins_.size())) {
		return nullptr;
	}
	return &registeredSkins_[handle - 1];
}

// Full skin system implementation (matches OpenGL2 RE_RegisterSkin in tr_image.c:3182-3296)
qhandle_t MetalRenderer::registerSkin(const char* name) {
	if (!name || !name[0]) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_DEVELOPER, "Metal: Empty name passed to registerSkin\n");
		}
		return 0;
	}

	if (strlen(name) >= MAX_QPATH) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_DEVELOPER, "Metal: Skin name exceeds MAX_QPATH\n");
		}
		return 0;
	}

	// Check if skin is already loaded
	const std::string key(name);
	auto it = skinLookup_.find(key);
	if (it != skinLookup_.end()) {
		qhandle_t handle = it->second;
		MetalSkin* skin = getSkinByHandle(handle);
		if (skin && skin->numSurfaces == 0) {
			return 0;  // default skin
		}
		return handle;
	}

	// Allocate a new skin
	const qhandle_t handle = static_cast<qhandle_t>(registeredSkins_.size() + 1);
	registeredSkins_.emplace_back();
	MetalSkin& skin = registeredSkins_.back();
	Q_strncpyz(skin.name, name, sizeof(skin.name));
	skin.numSurfaces = 0;
	skinLookup_[key] = handle;

	// If not a .skin file, load as a single shader
	const char* ext = name + strlen(name) - 5;
	if (strlen(name) < 5 || strcmp(ext, ".skin") != 0) {
		skin.numSurfaces = 1;
		skin.surfaces.resize(1);
		skin.surfaces[0].shader = registerShader(name, qtrue);
		if (ri_.Printf) {
			ri_.Printf(PRINT_DEVELOPER, "Metal: registered non-skin shader '%s' as skin handle %d\n", name, handle);
		}
		return handle;
	}

	// Load and parse the skin file
	union {
		char* c;
		void* v;
	} text;

	ri_.FS_ReadFile(name, &text.v);
	if (!text.c) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: skin file '%s' not found\n", name);
		}
		return 0;
	}

	// Parse skin file with temporary array (like OpenGL2)
	MetalSkinSurface parseSurfaces[MAX_SKIN_SURFACES];
	int totalSurfaces = 0;
	char* text_p = text.c;
	char surfName[MAX_QPATH];

	while (text_p && *text_p) {
		// Get surface name FIRST (matches OpenGL2 tr_image.c:3249-3250)
		char* token = CommaParse(&text_p);
		Q_strncpyz(surfName, token, sizeof(surfName));

		if (!token[0]) {
			break;
		}

		// Lowercase the surface name so skin compares are faster
		Q_strlwr(surfName);

		// Skip comma
		if (*text_p == ',') {
			text_p++;
		}

		// Skip if this looks like a tag (after reading surface name)
		if (strstr(surfName, "tag_")) {
			continue;
		}

		// Parse the shader name SECOND (matches OpenGL2 tr_image.c:3267)
		token = CommaParse(&text_p);
		char shaderName[MAX_QPATH];
		Q_strncpyz(shaderName, token, sizeof(shaderName));

		// Note: surfName is already lowercased above
		Q_strlwr(surfName);

		if (skin.numSurfaces < MAX_SKIN_SURFACES) {
			MetalSkinSurface& surf = parseSurfaces[skin.numSurfaces];
			Q_strncpyz(surf.name, surfName, sizeof(surf.name));
			surf.shader = registerShader(shaderName, qtrue);
			skin.numSurfaces++;

			if (ri_.Printf) {
				ri_.Printf(PRINT_DEVELOPER, "Metal: skin '%s' surface '%s' -> shader '%s' (handle %d)\n",
				          name, surfName, shaderName, surf.shader);
			}
		}

		totalSurfaces++;
	}

	ri_.FS_FreeFile(text.v);

	if (totalSurfaces > MAX_SKIN_SURFACES) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "WARNING: Ignoring excess surfaces (found %d, max is %d) in skin '%s'!\n",
			          totalSurfaces, MAX_SKIN_SURFACES, name);
		}
	}

	// Never let a skin have 0 shaders
	if (skin.numSurfaces == 0) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: skin '%s' has no surfaces, using default\n", name);
		}
		return 0;  // use default skin
	}

	// Copy surfaces to skin
	skin.surfaces.resize(skin.numSurfaces);
	for (int i = 0; i < skin.numSurfaces; ++i) {
		skin.surfaces[i] = parseSurfaces[i];
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: registered skin '%s' with %d surfaces as handle %d\n",
		          name, skin.numSurfaces, handle);
	}

	return handle;
}

int MetalRenderer::lerpTag(orientation_t* tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char* tagName) {
	// Validate model handle
	if (handle <= 0 || static_cast<size_t>(handle) >= models_.size()) {
		return 0;
	}

	MetalModel* model = models_[handle];
	if (!model || model->type != MetalModelType::MD3) {
		return 0;
	}

	// For now, just use LOD 0
	if (!model->lods[0]) {
		return 0;
	}

	MetalModelLOD* lodData = model->lods[0];

	// Clamp frame numbers
	if (startFrame < 0) startFrame = 0;
	if (startFrame >= lodData->numFrames) startFrame = lodData->numFrames - 1;
	if (endFrame < 0) endFrame = 0;
	if (endFrame >= lodData->numFrames) endFrame = lodData->numFrames - 1;

	// Find the tag index
	int tagIndex = -1;
	for (int i = 0; i < lodData->numTags; i++) {
		if (!strcmp(lodData->tagNames[i].name, tagName)) {
			tagIndex = i;
			break;
		}
	}

	if (tagIndex < 0) {
		return 0;
	}

	// Get tags for start and end frames
	const MetalModelTag& startTag = lodData->tags[startFrame * lodData->numTags + tagIndex];
	const MetalModelTag& endTag = lodData->tags[endFrame * lodData->numTags + tagIndex];

	// Interpolate origin
	tag->origin[0] = startTag.origin[0] + frac * (endTag.origin[0] - startTag.origin[0]);
	tag->origin[1] = startTag.origin[1] + frac * (endTag.origin[1] - startTag.origin[1]);
	tag->origin[2] = startTag.origin[2] + frac * (endTag.origin[2] - startTag.origin[2]);

	// Interpolate axis
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			tag->axis[i][j] = startTag.axis[i][j] + frac * (endTag.axis[i][j] - startTag.axis[i][j]);
		}
	}
	
	// Normalize axis
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);

	return 1;
}

void MetalRenderer::modelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs) {
	if (handle <= 0 || static_cast<size_t>(handle) >= models_.size()) {
		VectorClear(mins);
		VectorClear(maxs);
		return;
	}

	MetalModel* model = models_[handle];
	MetalModel_Bounds(model, mins, maxs);
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
		// if (ri_.Printf) {
		// 	ri_.Printf(PRINT_WARNING, "Metal: submitScene - refdefValid=FALSE, clearing scene state\n");
		// }
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

	// Debug logging disabled - too noisy
	// if (ri_.Printf) {
	// 	ri_.Printf(PRINT_DEVELOPER, "MetalScene: processed %d entities, %d polys, %d lights\n",
	// 	           sceneStats_.entities,
	// 	           sceneStats_.polys,
	// 	           sceneStats_.lights);
	// }
}

void MetalRenderer::processEntities(const MetalSceneState& scene) {
	// Process only the entities from the world scene (saved by RE_RenderScene)
	// Ignore UI scene entities (those were discarded)
	const int firstEntity = scene.worldSceneFirstEntity;
	const int numEntities = scene.worldSceneNumEntities;

	// DEBUG: Log entity processing
	// if (ri_.Printf) {
	// 	ri_.Printf(PRINT_ALL, "DEBUG: processEntities called - worldScene range [%d, %d) = %d entities\n",
	// 	          firstEntity, firstEntity + numEntities, numEntities);
	// }

	for (int i = 0; i < numEntities; ++i) {
		const int entityIndex = firstEntity + i;
		SceneDrawPacket packet;
		packet.entity = scene.entities[entityIndex];
		drawPackets_.push_back(packet);

		// if (packet.entity.reType == RT_MODEL && ri_.Printf) {
		// 	ri_.Printf(PRINT_ALL, "DEBUG: Entity %d - type=RT_MODEL, hModel=%d, renderfx=0x%x\n",
		// 	          entityIndex, packet.entity.hModel, packet.entity.renderfx);
		// }
	}
	sceneStats_.entities = static_cast<int>(drawPackets_.size());

	// if (ri_.Printf) {
	// 	ri_.Printf(PRINT_ALL, "DEBUG: processEntities added %d entities to drawPackets (%d are models)\n",
	// 	          numEntities, modelCount);
	// }
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
	// Draw model entities after world geometry
	if (!drawModelEntities()) {
		return;
	}
	// Draw dynamic lights after model rendering
	if (!drawDynamicLights()) {
		return;
	}
	// Draw fog passes after dynamic lights
	if (!drawFogPasses()) {
		return;
	}
	sceneDispatchSummary_ = summary;

	// Debug logging disabled - too noisy
	// if (ri_.Printf) {
	// 	ri_.Printf(PRINT_DEVELOPER,
	// 	          "MetalScene: dispatch %d entities (%d model, %d sprite, %d beam), %d polys (%d verts), %d lights\n",
	// 	          summary.totalEntities,
	// 	          summary.modelEntities,
	// 	          summary.spriteEntities,
	// 	          summary.beamEntities,
	// 	          summary.polySurfaces,
	// 	          summary.polyVertices,
	// 	          summary.dynamicLights);
	// }

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

	// Initialize fog uniforms using proper fog surface plane calculations
	// Based on RB_CalcFogTexCoords and ComputeFogValues from the OpenGL renderer
	if (!worldFogs_.empty()) {
		const FogVolume& fog = worldFogs_[0];  // Use first fog volume

		// Set fog color and alpha
		sceneUniforms_.fogColor[0] = fog.fogColor[0];
		sceneUniforms_.fogColor[1] = fog.fogColor[1];
		sceneUniforms_.fogColor[2] = fog.fogColor[2];
		sceneUniforms_.fogColor[3] = 1.0f;  // Alpha for fog density

		const float* viewOrigin = sceneCamera_.viewOrigin;

		// For fog distance, we want view-independent distance calculation
		// Use the fog surface plane normal direction for distance calculation
		// This makes fog density based on height (Z) rather than view direction
		// fogDistanceVector: use fog surface normal direction scaled by tcScale
		// For typical ground fog, surface points up (0,0,-1 after negation), so we use Z depth
		sceneUniforms_.fogDistanceVector[0] = fog.surface[0] * fog.tcScale;
		sceneUniforms_.fogDistanceVector[1] = fog.surface[1] * fog.tcScale;
		sceneUniforms_.fogDistanceVector[2] = fog.surface[2] * fog.tcScale;
		// W = offset based on camera position relative to fog surface
		sceneUniforms_.fogDistanceVector[3] = -(viewOrigin[0] * fog.surface[0] +
		                                        viewOrigin[1] * fog.surface[1] +
		                                        viewOrigin[2] * fog.surface[2]) * fog.tcScale;

		// Calculate fog depth vector based on whether fog has a surface plane
		if (fog.hasSurface) {
			// For world geometry (identity axis), fogDepthVector = fog.surface directly
			// fogDepthVector[3] = -fog.surface[3] + dot(modelOrigin, fog.surface)
			// For world with origin at (0,0,0): fogDepthVector[3] = -fog.surface[3]
			sceneUniforms_.fogDepthVector[0] = fog.surface[0];
			sceneUniforms_.fogDepthVector[1] = fog.surface[1];
			sceneUniforms_.fogDepthVector[2] = fog.surface[2];
			sceneUniforms_.fogDepthVector[3] = -fog.surface[3];  // Note: fog.surface[3] was already negated during load

			// eyeT = dot(viewOrigin, fogDepthVector) + fogDepthVector[3]
			// viewOrigin here should be in model space, but for world it's the same as world space
			sceneUniforms_.fogEyeT = viewOrigin[0] * sceneUniforms_.fogDepthVector[0] +
			                         viewOrigin[1] * sceneUniforms_.fogDepthVector[1] +
			                         viewOrigin[2] * sceneUniforms_.fogDepthVector[2] +
			                         sceneUniforms_.fogDepthVector[3];
		} else {
			// No surface - fog fills volume, eye is always "inside"
			sceneUniforms_.fogDepthVector[0] = 0.0f;
			sceneUniforms_.fogDepthVector[1] = 0.0f;
			sceneUniforms_.fogDepthVector[2] = 0.0f;
			sceneUniforms_.fogDepthVector[3] = 0.0f;
			sceneUniforms_.fogEyeT = 1.0f;  // Positive = eye inside fog
		}

		// Copy surface plane for reference
		sceneUniforms_.fogSurface[0] = fog.surface[0];
		sceneUniforms_.fogSurface[1] = fog.surface[1];
		sceneUniforms_.fogSurface[2] = fog.surface[2];
		sceneUniforms_.fogSurface[3] = fog.surface[3];

		// Copy fog volume bounds
		sceneUniforms_.fogBoundsMin[0] = fog.bounds[0][0];
		sceneUniforms_.fogBoundsMin[1] = fog.bounds[0][1];
		sceneUniforms_.fogBoundsMin[2] = fog.bounds[0][2];
		sceneUniforms_.fogBoundsMin[3] = 0.0f;
		sceneUniforms_.fogBoundsMax[0] = fog.bounds[1][0];
		sceneUniforms_.fogBoundsMax[1] = fog.bounds[1][1];
		sceneUniforms_.fogBoundsMax[2] = fog.bounds[1][2];
		sceneUniforms_.fogBoundsMax[3] = 0.0f;

		sceneUniforms_.fogTcScale = fog.tcScale;
		sceneUniforms_.fogHasSurface = fog.hasSurface ? 1.0f : 0.0f;
		sceneUniforms_.fogEnabled = 1.0f;

		// Debug: Print fog parameters once per map load
		static bool fogPrinted = false;
		if (!fogPrinted && ri_.Printf) {
			fogPrinted = true;
			ri_.Printf(PRINT_ALL, "Metal: Fog Uniforms:\n");
			ri_.Printf(PRINT_ALL, "  fogDistanceVector: (%.4f, %.4f, %.4f, %.4f)\n",
				sceneUniforms_.fogDistanceVector[0], sceneUniforms_.fogDistanceVector[1],
				sceneUniforms_.fogDistanceVector[2], sceneUniforms_.fogDistanceVector[3]);
			ri_.Printf(PRINT_ALL, "  fogDepthVector: (%.4f, %.4f, %.4f, %.4f)\n",
				sceneUniforms_.fogDepthVector[0], sceneUniforms_.fogDepthVector[1],
				sceneUniforms_.fogDepthVector[2], sceneUniforms_.fogDepthVector[3]);
			ri_.Printf(PRINT_ALL, "  fogEyeT: %.4f (eye %s fog)\n",
				sceneUniforms_.fogEyeT, sceneUniforms_.fogEyeT < 0.0f ? "outside" : "inside");
			ri_.Printf(PRINT_ALL, "  fogColor: (%.2f, %.2f, %.2f, %.2f)\n",
				sceneUniforms_.fogColor[0], sceneUniforms_.fogColor[1],
				sceneUniforms_.fogColor[2], sceneUniforms_.fogColor[3]);
			ri_.Printf(PRINT_ALL, "  fogTcScale: %.6f, hasSurface: %d\n",
				sceneUniforms_.fogTcScale, fog.hasSurface ? 1 : 0);
			ri_.Printf(PRINT_ALL, "  fogBoundsMin: (%.1f, %.1f, %.1f)\n",
				sceneUniforms_.fogBoundsMin[0], sceneUniforms_.fogBoundsMin[1], sceneUniforms_.fogBoundsMin[2]);
			ri_.Printf(PRINT_ALL, "  fogBoundsMax: (%.1f, %.1f, %.1f)\n",
				sceneUniforms_.fogBoundsMax[0], sceneUniforms_.fogBoundsMax[1], sceneUniforms_.fogBoundsMax[2]);
			ri_.Printf(PRINT_ALL, "  viewOrigin: (%.1f, %.1f, %.1f)\n",
				viewOrigin[0], viewOrigin[1], viewOrigin[2]);
		}
	} else {
		// No fog - disable fog rendering
		sceneUniforms_.fogEnabled = 0.0f;
	}

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

	// Set overbright bits
	if (r_overBrightBits_) {
		sceneUniforms_.overBrightBits = static_cast<float>(r_overBrightBits_->integer);
	} else {
		sceneUniforms_.overBrightBits = 0.0f;
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
	modelStagePipelineCache_.clear();
}

void MetalRenderer::ensureScriptHasStages(MetalRenderer::MetalShaderResource& resource) {
	MetalShaderScriptInfo& script = resource.script;
	
	// Don't add default stages to fog volume shaders - they should remain invisible
	// The volumetric fog effect is handled separately by drawFogPasses()
	if (script.hasFogParms && script.stages.empty()) {
		// Fog-only shader with no texture stages - leave it empty
		return;
	}
	
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
		// For shaders without explicit stages, use vertex colors (from BSP lightmaps)
		// NOT Identity/IdentityLighting which would force white
		stage.rgbGen.type = MetalRGBGen::Vertex;
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
	
	// Load skybox textures if this is a sky shader
	if (resource.hasScript && resource.script.isSky) {
		loadSkyboxTextures(resource);
	}
}

void MetalRenderer::loadSkyboxTextures(MetalRenderer::MetalShaderResource& resource) {
	TextureManager* tm = ensureTextureManager();
	if (!tm) {
		return;
	}
	
	resource.skyboxLoaded = false;
	bool hasOuterbox = false;
	
	// Load outer skybox textures (6 faces)
	for (size_t i = 0; i < 6; ++i) {
		const std::string& path = resource.script.outerboxTextures[i];
		if (!path.empty()) {
			qhandle_t handle = tm->registerShader(path.c_str(), true);
			resource.skyboxOuterHandles[i] = handle;
			if (handle > 0) {
				hasOuterbox = true;
			}
		}
	}
	
	// Load inner skybox textures if present
	for (size_t i = 0; i < 6; ++i) {
		const std::string& path = resource.script.innerboxTextures[i];
		if (!path.empty()) {
			qhandle_t handle = tm->registerShader(path.c_str(), true);
			resource.skyboxInnerHandles[i] = handle;
		}
	}
	
	resource.skyboxLoaded = hasOuterbox;
	
	if (hasOuterbox && ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: Loaded skybox textures for '%s'\n", resource.name.c_str());
	}
}

void MetalRenderer::buildShaderStageRuntime(MetalRenderer::MetalShaderResource& resource) {
	resource.stageRuntimes.clear();
	resource.primaryStageIndex = 0;

	const size_t stageCount = resource.script.stages.size();
	if (stageCount == 0) {
		updatePrimaryImageHandle(resource);
		return;
	}

	// Helper to normalize path for comparison (lowercase, no extension)
	auto normalizePath = [](const std::string& path) -> std::string {
		std::string result = path;
		// Remove extension if present
		size_t dotPos = result.rfind('.');
		size_t slashPos = result.rfind('/');
		if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos)) {
			result = result.substr(0, dotPos);
		}
		// Convert to lowercase
		for (auto& c : result) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
		return result;
	};

	// Build a lookup map from normalized image path to handle
	std::unordered_map<std::string, qhandle_t> pathToHandle;
	for (size_t i = 0; i < resource.imageHandles.size() && i < resource.script.imagePaths.size(); ++i) {
		std::string normalized = normalizePath(resource.script.imagePaths[i]);
		pathToHandle[normalized] = resource.imageHandles[i];
	}

	resource.stageRuntimes.resize(stageCount);

	for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
		MetalShaderResource::MetalShaderStageRuntime& runtime = resource.stageRuntimes[stageIndex];
		MetalShaderStageInfo& stageInfo = resource.script.stages[stageIndex];
		runtime.stageInfo = &stageInfo;
		runtime.usesLightmap = stageInfo.usesLightmap;
		runtime.usesWhiteImage = stageInfo.usesWhiteImage;
		
		// Find handles for this stage's image paths
		for (const std::string& path : stageInfo.imagePaths) {
			std::string normalized = normalizePath(path);
			auto it = pathToHandle.find(normalized);
			if (it != pathToHandle.end()) {
				runtime.stageImageHandles.push_back(it->second);
			}
		}
		
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
			// If lightmap is missing but stage expects one, use white image
			// to avoid sampling wrong texture (e.g. primary shader image)
			// This fixes the orange tint issue where lightmap stages were sampling the lava texture
			if (textureManager_) {
				return textureManager_->registerShader("white", false);
			}
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

MetalRenderer::TCModParams MetalRenderer::computeTCModParams(const MetalShaderStageInfo* stageInfo, float timeSeconds) const {
	TCModParams params;  // Initialize to identity matrices
	
	if (!stageInfo || stageInfo->tcMods.empty()) {
		return params;
	}
	
	// Process up to 4 tcMod stages
	const size_t numMods = std::min(stageInfo->tcMods.size(), static_cast<size_t>(4));
	
	for (size_t i = 0; i < numMods; ++i) {
		const MetalTCMod& mod = stageInfo->tcMods[i];
		float matrix[6];
		float turbAmp, turbPhase;
		
		ComputeTCModMatrix(mod, timeSeconds, matrix, turbAmp, turbPhase);
		
		// Store in the appropriate slot
		// Each tcMod uses 2 vec4s: [i*2] and [i*2+1]
		// Format: [even].xyz = (scaleX, shearX, translateX), [even].w = turbAmp
		//         [odd].xyz = (shearY, scaleY, translateY), [odd].w = turbPhase
		float* destEven = nullptr;
		float* destOdd = nullptr;
		
		switch (i) {
			case 0: destEven = params.texMatrix0; destOdd = params.texMatrix1; break;
			case 1: destEven = params.texMatrix2; destOdd = params.texMatrix3; break;
			case 2: destEven = params.texMatrix4; destOdd = params.texMatrix5; break;
			case 3: destEven = params.texMatrix6; destOdd = params.texMatrix7; break;
		}
		
		if (destEven && destOdd) {
			destEven[0] = matrix[0];  // scaleX
			destEven[1] = matrix[2];  // shearX
			destEven[2] = matrix[4];  // translateX
			destEven[3] = turbAmp;
			
			destOdd[0] = matrix[1];   // shearY
			destOdd[1] = matrix[3];   // scaleY
			destOdd[2] = matrix[5];   // translateY
			destOdd[3] = turbPhase;
		}
	}
	
	return params;
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

	// Load fog shader functions
	if (!fogVertexFunction_) {
		NS::String* fogVertexName = NS::String::string("vertex_fog", NS::ASCIIStringEncoding);
		fogVertexFunction_.reset(sceneLibrary_->newFunction(fogVertexName));
		fogVertexName->release();
		if (!fogVertexFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: missing vertex_fog function\n");
			}
			return false;
		}
	}

	if (!fogFragmentFunction_) {
		NS::String* fogFragmentName = NS::String::string("fragment_fog", NS::ASCIIStringEncoding);
		fogFragmentFunction_.reset(sceneLibrary_->newFunction(fogFragmentName));
		fogFragmentName->release();
		if (!fogFragmentFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: missing fragment_fog function\n");
			}
			return false;
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

bool MetalRenderer::ensureFogPipeline() {
	if (fogPipeline_) {
		return true;
	}
	if (!device_ || !depthTexture_ || !fogVertexFunction_ || !fogFragmentFunction_ || !sceneVertexDescriptor_) {
		return false;
	}

	NS::Error* error = nullptr;
	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();

	pd->setVertexFunction(fogVertexFunction_.get());
	pd->setFragmentFunction(fogFragmentFunction_.get());
	pd->setVertexDescriptor(sceneVertexDescriptor_.get());

	// Color attachment with alpha blending
	MTL::RenderPipelineColorAttachmentDescriptor* colorAttachment = pd->colorAttachments()->object(0);
	colorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	colorAttachment->setBlendingEnabled(true);
	colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	fogPipeline_.reset(device_->newRenderPipelineState(pd, &error));
	pd->release();

	if (!fogPipeline_) {
		if (error && ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to create fog pipeline: %s\n",
				error->localizedDescription()->utf8String());
			error->release();
		}
		return false;
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: Created fog pipeline with alpha blending\n");
	}

	// Create fog depth stencil state if needed
	if (!fogDepthState_) {
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthWriteEnabled(false);  // Don't write to depth buffer
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);  // Draw on existing geometry
		fogDepthState_.reset(device_->newDepthStencilState(depthDesc));
		depthDesc->release();

		if (ri_.Printf && fogDepthState_) {
			ri_.Printf(PRINT_ALL, "Metal: Created fog depth stencil state\n");
		}
	}

	return true;
}

bool MetalRenderer::ensureModelPipeline() {
	if (modelPipeline_) {
		return true;
	}
	if (!device_ || !depthTexture_) {
		return false;
	}

	// Load shaders
	if (!sceneLibrary_) {
		sceneLibrary_.reset(device_->newDefaultLibrary());
		if (!sceneLibrary_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: Failed to load default library for model pipeline\n");
			}
			return false;
		}
	}

	if (!modelVertexFunction_) {
		NS::String* vertName = NS::String::string("vertex_model", NS::UTF8StringEncoding);
		modelVertexFunction_.reset(sceneLibrary_->newFunction(vertName));
		if (!modelVertexFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: Failed to load vertex_model shader function\n");
			}
			return false;
		}
	}

	if (!modelFragmentFunction_) {
		NS::String* fragName = NS::String::string("fragment_model", NS::UTF8StringEncoding);
		modelFragmentFunction_.reset(sceneLibrary_->newFunction(fragName));
		if (!modelFragmentFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: Failed to load fragment_model shader function\n");
			}
			return false;
		}
	}

	// Create vertex descriptor with 5 attributes
	if (!modelVertexDescriptor_) {
		modelVertexDescriptor_.reset(MTL::VertexDescriptor::alloc()->init());

		// Attribute 0: position (old frame) - float3
		MTL::VertexAttributeDescriptor* attr0 = modelVertexDescriptor_->attributes()->object(0);
		attr0->setFormat(MTL::VertexFormatFloat3);
		attr0->setOffset(0);
		attr0->setBufferIndex(0);

		// Attribute 1: normal (old frame) - float3
		MTL::VertexAttributeDescriptor* attr1 = modelVertexDescriptor_->attributes()->object(1);
		attr1->setFormat(MTL::VertexFormatFloat3);
		attr1->setOffset(12);  // 3 floats * 4 bytes
		attr1->setBufferIndex(0);

		// Attribute 2: texcoord - float2
		MTL::VertexAttributeDescriptor* attr2 = modelVertexDescriptor_->attributes()->object(2);
		attr2->setFormat(MTL::VertexFormatFloat2);
		attr2->setOffset(24);  // 6 floats * 4 bytes
		attr2->setBufferIndex(0);

		// Attribute 3: position (new frame) - float3
		MTL::VertexAttributeDescriptor* attr3 = modelVertexDescriptor_->attributes()->object(3);
		attr3->setFormat(MTL::VertexFormatFloat3);
		attr3->setOffset(32);  // 8 floats * 4 bytes
		attr3->setBufferIndex(0);

		// Attribute 4: normal (new frame) - float3
		MTL::VertexAttributeDescriptor* attr4 = modelVertexDescriptor_->attributes()->object(4);
		attr4->setFormat(MTL::VertexFormatFloat3);
		attr4->setOffset(44);  // 11 floats * 4 bytes
		attr4->setBufferIndex(0);

		// Layout 0: vertex buffer with stride = 14 floats * 4 bytes = 56 bytes
		MTL::VertexBufferLayoutDescriptor* layout0 = modelVertexDescriptor_->layouts()->object(0);
		layout0->setStride(56);
		layout0->setStepRate(1);
		layout0->setStepFunction(MTL::VertexStepFunctionPerVertex);
	}

	// Create pipeline state
	NS::Error* error = nullptr;
	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();

	pd->setVertexFunction(modelVertexFunction_.get());
	pd->setFragmentFunction(modelFragmentFunction_.get());
	pd->setVertexDescriptor(modelVertexDescriptor_.get());

	// Color attachment - enable alpha blending for transparent model surfaces
	MTL::RenderPipelineColorAttachmentDescriptor* colorAttachment = pd->colorAttachments()->object(0);
	colorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	colorAttachment->setBlendingEnabled(true);
	colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	modelPipeline_.reset(device_->newRenderPipelineState(pd, &error));
	pd->release();

	if (!modelPipeline_) {
		if (error && ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to create model pipeline: %s\n",
				error->localizedDescription()->utf8String());
			error->release();
		}
		return false;
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: Created model rendering pipeline\n");
	}

	// Create model depth stencil state
	if (!modelDepthState_) {
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthWriteEnabled(true);
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
		modelDepthState_.reset(device_->newDepthStencilState(depthDesc));
		depthDesc->release();

		if (ri_.Printf && modelDepthState_) {
			ri_.Printf(PRINT_ALL, "Metal: Created model depth stencil state\n");
		}
	}

	// Create model depth stencil state with no depth write (for additive surfaces)
	if (!modelDepthStateNoWrite_) {
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthWriteEnabled(false);  // Additive surfaces don't write depth
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
		modelDepthStateNoWrite_.reset(device_->newDepthStencilState(depthDesc));
		depthDesc->release();
	}

	// Create additive model pipeline (GL_ONE, GL_ONE blend mode)
	if (!modelPipelineAdditive_) {
		NS::Error* addError = nullptr;
		MTL::RenderPipelineDescriptor* addPd = MTL::RenderPipelineDescriptor::alloc()->init();
		addPd->setVertexFunction(modelVertexFunction_.get());
		addPd->setFragmentFunction(modelFragmentFunction_.get());
		addPd->setVertexDescriptor(modelVertexDescriptor_.get());

		MTL::RenderPipelineColorAttachmentDescriptor* addColorAttachment = addPd->colorAttachments()->object(0);
		addColorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
		addColorAttachment->setBlendingEnabled(true);
		addColorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorOne);
		addColorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
		addColorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
		addColorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
		addColorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
		addColorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

		addPd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

		modelPipelineAdditive_.reset(device_->newRenderPipelineState(addPd, &addError));
		addPd->release();

		if (!modelPipelineAdditive_ && addError && ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to create additive model pipeline: %s\n",
				addError->localizedDescription()->utf8String());
			addError->release();
		} else if (ri_.Printf) {
			ri_.Printf(PRINT_ALL, "Metal: Created additive model rendering pipeline\n");
		}
	}

	return true;
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

MetalRenderer::ModelStagePipelineEntry* MetalRenderer::getModelStagePipeline(const MetalRenderer::MetalShaderResource::MetalPipelineKey& key) {
	// Ensure model pipeline resources are available
	if (!ensureModelPipeline()) {
		return nullptr;
	}

	auto it = modelStagePipelineCache_.find(key);
	if (it != modelStagePipelineCache_.end()) {
		return &it->second;
	}

	ModelStagePipelineEntry entry;
	entry.key = key;

	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexDescriptor(modelVertexDescriptor_.get());
	pd->setVertexFunction(modelVertexFunction_.get());
	pd->setFragmentFunction(modelFragmentFunction_.get());
	
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
			ri_.Printf(PRINT_WARNING, "Metal: failed to build model stage pipeline (%s)\n", msg);
		}
		if (error) {
			error->release();
		}
		return nullptr;
	}
	if (error) {
		error->release();
	}

	// Determine depth write based on blend mode
	// Opaque (One, Zero) writes depth, blended surfaces don't
	bool depthWrite = !enableBlend;
	if (key.depthWriteExplicit) {
		depthWrite = key.depthWrite;
	}

	MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
	depthDesc->setDepthWriteEnabled(depthWrite);
	depthDesc->setDepthCompareFunction(key.depthTest ? MTL::CompareFunctionLessEqual : MTL::CompareFunctionAlways);
	entry.depthState.reset(device_->newDepthStencilState(depthDesc));
	depthDesc->release();
	if (!entry.depthState) {
		return nullptr;
	}

	auto [insertedIt, _] = modelStagePipelineCache_.emplace(key, std::move(entry));
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

	// Setup default entity lighting for world surfaces
	// World surfaces use lightmaps, so entity lighting provides fallback/ambient
	EntityLightingParams defaultLighting{};
	// Use identity lighting (white) - surfaces will be lit by lightmaps
	defaultLighting.ambientLight[0] = 255.0f;
	defaultLighting.ambientLight[1] = 255.0f;
	defaultLighting.ambientLight[2] = 255.0f;
	defaultLighting.directedLight[0] = 0.0f;
	defaultLighting.directedLight[1] = 0.0f;
	defaultLighting.directedLight[2] = 0.0f;
	defaultLighting.lightDir[0] = 0.0f;
	defaultLighting.lightDir[1] = 0.0f;
	defaultLighting.lightDir[2] = 1.0f;
	currentRenderEncoder_->setFragmentBytes(&defaultLighting, sizeof(EntityLightingParams), 2);

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
	auto buildStageParams = [](const MetalShaderStageInfo* stageInfo, float overBrightBits, qhandle_t lightmapHandle) {
		StageFragmentParams params{};
		params.overBrightBits = overBrightBits;
		if (!stageInfo) {
			return params;
		}

		// Set tcGen type for shader
		// 0 = Texture (base texcoords), 1 = Lightmap, 2 = Environment
		switch (stageInfo->tcGen.type) {
			case MetalTCGen::Lightmap:
				params.tcGenType = 1.0f;
				params.texCoordSelector = 1.0f;  // Legacy compatibility
				break;
			case MetalTCGen::Environment:
				params.tcGenType = 2.0f;
				params.texCoordSelector = 0.0f;
				break;
			default:
				params.tcGenType = 0.0f;
				params.texCoordSelector = 0.0f;
				break;
		}

		// Set rgbGen type for shader
		// 0 = Vertex (use vertex colors), 1 = Identity (white), 2 = IdentityLighting, 3 = LightingDiffuse
		// IMPORTANT: For world surfaces with lightmaps, override lightingDiffuse → vertex
		// This matches OpenGL2's behavior where surfaces with lightmaps use CGEN_EXACT_VERTEX
		MetalRGBGen effectiveRgbGen = stageInfo->rgbGen.type;
		if (effectiveRgbGen == MetalRGBGen::LightingDiffuse && lightmapHandle > 0 && !stageInfo->usesLightmap) {
			// Surface has a lightmap AND this stage doesn't use $lightmap texture
			// Use vertex colors (lightmap data) instead of entity lighting
			// Stages using $lightmap texture should keep their rgbGen (usually identity)
			effectiveRgbGen = MetalRGBGen::Vertex;
		}

		switch (effectiveRgbGen) {
			case MetalRGBGen::Identity:
				params.rgbGenType = 1.0f;
				break;
			case MetalRGBGen::IdentityLighting:
				params.rgbGenType = 2.0f;
				break;
			case MetalRGBGen::LightingDiffuse:
				params.rgbGenType = 3.0f;
				break;
			default:
				params.rgbGenType = 0.0f;  // Use vertex color
				break;
		}
		
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
	
	// Helper lambda to draw a single packet
	auto drawPacket = [&](const ScenePolyPacket& packet) {
		if (packet.vertexCount <= 0) {
			return;
		}
		const size_t endVertex = static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount);
		if (endVertex > polyVertexCountGPU_) {
			return;
		}

		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (!shaderResource) {
			shaderResource = getShaderResource(0);
		}
		if (!shaderResource || shaderResource->stageRuntimes.empty()) {
			return;
		}

		const size_t stageCount = shaderResource->stageRuntimes.size();
		for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
			const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
			if (!stageRuntime) {
				continue;
			}
			const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
			
			// Match OpenGL2's ComputeShaderColors (tr_shade.c:572-579)
			// Disable overbright for stages using DST_COLOR or SRC_COLOR blending
			bool isBlend = (stageRuntime->pipelineKey.srcBlend == MetalBlendFactor::DstColor) ||
			               (stageRuntime->pipelineKey.srcBlend == MetalBlendFactor::OneMinusDstColor) ||
			               (stageRuntime->pipelineKey.dstBlend == MetalBlendFactor::SrcColor) ||
			               (stageRuntime->pipelineKey.dstBlend == MetalBlendFactor::OneMinusSrcColor);
			
			float stageOverBright = isBlend ? 0.0f : sceneUniforms_.overBrightBits;

			bindStageParams(buildStageParams(stageInfo, stageOverBright, packet.lightmapHandle));

			// Compute and bind texture coordinate modifications
			TCModParams tcModParams = computeTCModParams(stageInfo, sceneTimeSeconds);
			currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);

			StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
			if (!pipelineEntry || !pipelineEntry->pipeline || !pipelineEntry->depthState) {
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
	};
	
	// Check if shader has fog surface texture stages (not just fogparms)
	auto hasFogSurfaceStages = [this](const ScenePolyPacket& packet) -> bool {
		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (!shaderResource) {
			return false;
		}
		return shaderResource->hasScript && 
		       shaderResource->script.hasFogParms && 
		       !shaderResource->stageRuntimes.empty();
	};
	
	// Check if shader is a sky shader with skybox (cubemap)
	auto isSkyboxPacket = [this](const ScenePolyPacket& packet) -> bool {
		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (!shaderResource) {
			return false;
		}
		return shaderResource->hasScript && 
		       shaderResource->script.isSky && 
		       shaderResource->skyboxLoaded;
	};
	
	// Check if shader is a cloud-only sky (isSky=true but no skybox textures)
	auto isCloudSkyPacket = [this](const ScenePolyPacket& packet) -> bool {
		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (!shaderResource) {
			return false;
		}
		return shaderResource->hasScript && 
		       shaderResource->script.isSky && 
		       !shaderResource->skyboxLoaded;
	};
	
	// Reset skybox rendered flag
	skyboxRenderedThisFrame_ = false;

	// Pass 0: Draw skybox for sky surfaces that have cubemap textures
	for (const ScenePolyPacket& packet : polyPackets_) {
		if (isSkyboxPacket(packet)) {
			if (!skyboxRenderedThisFrame_) {
				drawSkybox(packet.shader);
				// Reset state after skybox rendering - MUST rebind vertex buffer
				// because skybox uses setVertexBytes which overrides the binding
				currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
				currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
				boundPipeline = nullptr;
				boundDepthState = nullptr;
				boundImageHandle = -1;
				boundTexture = nullptr;
				hasFragmentParams = false;
			}
		}
	}
	
	// Pass 0b: Draw cloud sky dome for cloud-only sky surfaces
	for (const ScenePolyPacket& packet : polyPackets_) {
		if (isCloudSkyPacket(packet)) {
			if (!skyboxRenderedThisFrame_) {
				drawCloudSky(packet.shader);
				// Reset state after cloud sky rendering - MUST rebind vertex buffer
				// because cloud sky uses setVertexBytes which overrides the binding
				currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
				currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
				boundPipeline = nullptr;
				boundDepthState = nullptr;
				boundImageHandle = -1;
				boundTexture = nullptr;
				hasFragmentParams = false;
			}
			break;  // Only need one sky shader
		}
	}

	// Pass 1: Draw all non-fog-surface packets
	// Skip sky surfaces entirely - they're now drawn as sky dome/skybox
	for (const ScenePolyPacket& packet : polyPackets_) {
		// Skip fog surfaces (drawn in pass 2)
		if (hasFogSurfaceStages(packet)) {
			continue;
		}
		// Skip skybox surfaces (already drawn as skybox)
		if (isSkyboxPacket(packet)) {
			continue;
		}
		// Skip cloud sky surfaces (already drawn as cloud dome)
		if (isCloudSkyPacket(packet)) {
			continue;
		}
		drawPacket(packet);
	}
	
	// Pass 2: Draw fog surface packets (they use filter blend which multiplies destination)
	// These must be drawn after the geometry behind them for the filter blend to work correctly
	for (const ScenePolyPacket& packet : polyPackets_) {
		if (hasFogSurfaceStages(packet)) {
			drawPacket(packet);
		}
	}

	return true;
}

// ============================================================================
// Skybox Rendering
// ============================================================================

bool MetalRenderer::ensureSkyboxResources() {
	// Create skybox depth state if not exists (always pass, no write)
	if (!skyboxDepthState_) {
		auto depthDesc = MetalPtr<MTL::DepthStencilDescriptor>(MTL::DepthStencilDescriptor::alloc()->init());
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
		depthDesc->setDepthWriteEnabled(false);  // Don't write to depth buffer
		skyboxDepthState_ = MetalPtr<MTL::DepthStencilState>(device_->newDepthStencilState(depthDesc.get()));
		if (!skyboxDepthState_) {
			if (ri_.Printf) ri_.Printf(PRINT_WARNING, "Metal: Failed to create skybox depth state\n");
			return false;
		}
	}
	
	// Build skybox vertex buffer if not exists
	if (!skyboxVertexBuffer_.get()) {
		buildSkyboxVertexBuffer();
	}
	
	return skyboxVertexBuffer_.get() != nullptr;
}

void MetalRenderer::buildSkyboxVertexBuffer() {
	// Create a unit cube centered at origin (-1 to 1 on each axis)
	// We'll scale and translate it to the camera position in the shader
	// Order: rt(+X), lf(-X), ft(+Y), bk(-Y), up(+Z), dn(-Z)
	// Note: Quake uses Z-up coordinate system
	
	struct SkyboxVertex {
		float position[3];
		float texCoord[2];
	};
	
	// Build 6 faces, each with 2 triangles (6 vertices)
	// Each face is a quad from -1 to 1 on two axes, with the third axis at ±1
	std::vector<SkyboxVertex> vertices;
	vertices.reserve(SKYBOX_VERTEX_COUNT);
	
	// Right face (+X) - looking from inside the box toward +X
	// When looking at +X, we see: top=+Z, right=-Y, bottom=-Z, left=+Y
	auto addQuad = [&vertices](
		float p0[3], float p1[3], float p2[3], float p3[3]) {
		// Two triangles: p0-p1-p2 and p0-p2-p3
		// Texture coords: p0=(0,0), p1=(1,0), p2=(1,1), p3=(0,1)
		vertices.push_back({{p0[0], p0[1], p0[2]}, {0.0f, 0.0f}});
		vertices.push_back({{p1[0], p1[1], p1[2]}, {1.0f, 0.0f}});
		vertices.push_back({{p2[0], p2[1], p2[2]}, {1.0f, 1.0f}});
		
		vertices.push_back({{p0[0], p0[1], p0[2]}, {0.0f, 0.0f}});
		vertices.push_back({{p2[0], p2[1], p2[2]}, {1.0f, 1.0f}});
		vertices.push_back({{p3[0], p3[1], p3[2]}, {0.0f, 1.0f}});
	};
	
	const float S = 1.0f;  // Half-size of the skybox cube
	
	// Face 0: Right (+X) - rt
	{
		float p0[] = {S, S, -S};   // top-left (looking from inside)
		float p1[] = {S, -S, -S};  // top-right
		float p2[] = {S, -S, S};   // bottom-right
		float p3[] = {S, S, S};    // bottom-left
		addQuad(p0, p1, p2, p3);
	}
	
	// Face 1: Left (-X) - lf
	{
		float p0[] = {-S, -S, -S};
		float p1[] = {-S, S, -S};
		float p2[] = {-S, S, S};
		float p3[] = {-S, -S, S};
		addQuad(p0, p1, p2, p3);
	}
	
	// Face 2: Front (+Y) - ft
	{
		float p0[] = {S, S, -S};
		float p1[] = {-S, S, -S};
		float p2[] = {-S, S, S};
		float p3[] = {S, S, S};
		addQuad(p0, p1, p2, p3);
	}
	
	// Face 3: Back (-Y) - bk
	{
		float p0[] = {-S, -S, -S};
		float p1[] = {S, -S, -S};
		float p2[] = {S, -S, S};
		float p3[] = {-S, -S, S};
		addQuad(p0, p1, p2, p3);
	}
	
	// Face 4: Up (+Z) - up
	{
		float p0[] = {-S, S, S};
		float p1[] = {-S, -S, S};
		float p2[] = {S, -S, S};
		float p3[] = {S, S, S};
		addQuad(p0, p1, p2, p3);
	}
	
	// Face 5: Down (-Z) - dn
	{
		float p0[] = {-S, -S, -S};
		float p1[] = {-S, S, -S};
		float p2[] = {S, S, -S};
		float p3[] = {S, -S, -S};
		addQuad(p0, p1, p2, p3);
	}
	
	// Create GPU buffer
	size_t bufferSize = vertices.size() * sizeof(SkyboxVertex);
	skyboxVertexBuffer_ = MetalPtr<MTL::Buffer>(
		device_->newBuffer(vertices.data(), bufferSize, MTL::ResourceStorageModeShared));
	
	if (skyboxVertexBuffer_ && ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: Created skybox vertex buffer (%zu vertices, %zu bytes)\n",
			vertices.size(), bufferSize);
	}
}

bool MetalRenderer::drawSkybox(qhandle_t skyShader) {
	if (skyboxRenderedThisFrame_) {
		return true;  // Only render skybox once per frame
	}
	
	MetalShaderResource* resource = getShaderResource(skyShader);
	if (!resource || !resource->hasScript || !resource->script.isSky || !resource->skyboxLoaded) {
		return false;
	}
	
	if (!ensureSkyboxResources()) {
		return false;
	}
	
	if (!currentRenderEncoder_ || !sceneUniformBuffer_) {
		return false;
	}
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) {
		return false;
	}
	
	// Get a simple opaque pipeline for skybox rendering
	// We can reuse the scene pipeline with appropriate settings
	MetalShaderResource::MetalPipelineKey skyKey;
	skyKey.blendMode = MetalShaderBlendMode::Opaque;
	skyKey.srcBlend = MetalBlendFactor::One;
	skyKey.dstBlend = MetalBlendFactor::Zero;
	skyKey.depthWrite = false;  // Don't write to depth
	skyKey.depthTest = true;    // Still test depth (skybox at max depth)
	skyKey.alphaTest = false;
	skyKey.depthWriteExplicit = true;
	
	StagePipelineEntry* pipelineEntry = getStagePipeline(skyKey);
	if (!pipelineEntry || !pipelineEntry->pipeline) {
		if (ri_.Printf) ri_.Printf(PRINT_WARNING, "Metal: No pipeline for skybox\n");
		return false;
	}
	
	// Set pipeline and depth state
	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
	currentRenderEncoder_->setDepthStencilState(skyboxDepthState_.get());
	currentRenderEncoder_->setCullMode(MTL::CullModeBack);  // Cull back faces (we're inside the box)
	
	// Bind skybox vertex buffer - need to use the standard vertex layout
	// The skybox vertices need to be converted to MetalPolyVertex format
	// For now, let's create the vertices on the fly and use the poly vertex buffer
	
	// Actually, let's use a different approach - generate skybox vertices in poly format
	// and add them to the vertex buffer
	
	// For simplicity, let's draw each face separately with its texture
	// This is less efficient but easier to implement correctly
	
	const float boxSize = sceneCamera_.zFar / 1.75f;  // Match OpenGL calculation
	const float* viewOrigin = sceneCamera_.viewOrigin;
	
	// Create temporary vertices for each face
	std::vector<MetalPolyVertex> faceVerts(6);  // 2 triangles = 6 verts per face
	
	auto makeFaceVerts = [&](int faceIndex, 
		const float v0[3], const float v1[3], const float v2[3], const float v3[3]) {
		// v0=top-left, v1=top-right, v2=bottom-right, v3=bottom-left
		// Triangle 1: v0, v1, v2
		// Triangle 2: v0, v2, v3
		(void)faceIndex;  // Not used currently
		auto setVert = [&](int idx, const float pos[3], float s, float t) {
			faceVerts[idx].xyz[0] = viewOrigin[0] + pos[0] * boxSize;
			faceVerts[idx].xyz[1] = viewOrigin[1] + pos[1] * boxSize;
			faceVerts[idx].xyz[2] = viewOrigin[2] + pos[2] * boxSize;
			faceVerts[idx].st[0] = s;
			faceVerts[idx].st[1] = t;
			faceVerts[idx].lightmap[0] = 0;
			faceVerts[idx].lightmap[1] = 0;
			faceVerts[idx].modulate[0] = 255;
			faceVerts[idx].modulate[1] = 255;
			faceVerts[idx].modulate[2] = 255;
			faceVerts[idx].modulate[3] = 255;
			faceVerts[idx].normal[0] = 0;
			faceVerts[idx].normal[1] = 0;
			faceVerts[idx].normal[2] = 1.0f;
		};
		
		setVert(0, v0, 0.0f, 0.0f);
		setVert(1, v1, 1.0f, 0.0f);
		setVert(2, v2, 1.0f, 1.0f);
		setVert(3, v0, 0.0f, 0.0f);
		setVert(4, v2, 1.0f, 1.0f);
		setVert(5, v3, 0.0f, 1.0f);
	};
	
	// Upload face vertices and draw each face
	const float S = 1.0f;
	
	// Face definitions matching the skybox texture order (rt, lf, ft, bk, up, dn)
	struct FaceDef {
		float v0[3], v1[3], v2[3], v3[3];
	};
	
	FaceDef faces[6] = {
		// Right (+X)
		{{S, S, -S}, {S, -S, -S}, {S, -S, S}, {S, S, S}},
		// Left (-X)
		{{-S, -S, -S}, {-S, S, -S}, {-S, S, S}, {-S, -S, S}},
		// Front (+Y)
		{{S, S, -S}, {-S, S, -S}, {-S, S, S}, {S, S, S}},
		// Back (-Y)
		{{-S, -S, -S}, {S, -S, -S}, {S, -S, S}, {-S, -S, S}},
		// Up (+Z)
		{{-S, S, S}, {-S, -S, S}, {S, -S, S}, {S, S, S}},
		// Down (-Z)
		{{-S, -S, -S}, {-S, S, -S}, {S, S, -S}, {S, -S, -S}},
	};
	
	// Set up fragment params for skybox (use vertex colors as-is)
	StageFragmentParams fragParams{};
	fragParams.texCoordSelector = 0.0f;
	fragParams.rgbGenType = 0.0f;  // Use vertex color (white)
	fragParams.overBrightBits = sceneUniforms_.overBrightBits;
	currentRenderEncoder_->setFragmentBytes(&fragParams, sizeof(StageFragmentParams), 0);
	
	// No texture coordinate modifications for skybox (identity matrix)
	TCModParams tcModParams{};  // Default constructor sets identity matrices
	currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
	
	// Bind the scene uniform buffer
	currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
	
	// Draw each face with its texture
	for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
		qhandle_t texHandle = resource->skyboxOuterHandles[faceIdx];
		if (texHandle <= 0) {
			continue;  // Skip faces without textures
		}
		
		MTL::Texture* texture = texManager->getTexture(texHandle);
		if (!texture) {
			continue;
		}
		
		// Generate vertices for this face
		makeFaceVerts(faceIdx, faces[faceIdx].v0, faces[faceIdx].v1, 
			faces[faceIdx].v2, faces[faceIdx].v3);
		
		// Upload vertices directly
		currentRenderEncoder_->setVertexBytes(faceVerts.data(), 
			faceVerts.size() * sizeof(MetalPolyVertex), 0);
		
		// Bind texture
		MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, texture);
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sceneSampler_.get());
		
		// Draw face
		currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangle, 
			NS::UInteger(0), NS::UInteger(6));
	}
	
	skyboxRenderedThisFrame_ = true;
	
	if (ri_.Printf) {
		static int logCounter = 0;
		if (logCounter++ % 300 == 0) {
			ri_.Printf(PRINT_ALL, "Metal: Drew skybox for shader '%s'\n", resource->name.c_str());
		}
	}
	
	return true;
}

// ============================================================================
// Cloud Sky Dome Rendering (for skies without cubemap textures)
// ============================================================================

// Initialize cloud sky texture coordinates based on cloud height
// This is called once when a sky shader is first used
// Adapted from OpenGL's R_InitSkyTexCoords
void MetalRenderer::initCloudSkyTexCoords(float cloudHeight) {
	if (cloudTexCoordsInitialized_) {
		return;
	}
	
	constexpr float radiusWorld = 4096.0f;
	constexpr float zFarInit = 1024.0f;  // Initial zFar for MakeSkyVec-like calculation
	const float boxSize = zFarInit / 1.75f;
	
	// st_to_vec mapping from OpenGL (axis -> vector component mapping)
	static const int st_to_vec[6][3] = {
		{3, -1, 2},
		{-3, 1, 2},
		{1, 3, 2},
		{-1, -3, 2},
		{-2, -1, 3},  // up
		{2, -1, -3}   // down
	};
	
	for (int i = 0; i < 6; ++i) {
		for (int t = 0; t <= SKY_SUBDIVISIONS; ++t) {
			for (int s = 0; s <= SKY_SUBDIVISIONS; ++s) {
				// Compute normalized sky vector (like MakeSkyVec)
				float sParam = (static_cast<float>(s - HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS);
				float tParam = (static_cast<float>(t - HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS);
				
				float b[3];
				b[0] = sParam * boxSize;
				b[1] = tParam * boxSize;
				b[2] = boxSize;
				
				float skyVec[3];
				for (int j = 0; j < 3; ++j) {
					int k = st_to_vec[i][j];
					if (k < 0) {
						skyVec[j] = -b[-k - 1];
					} else {
						skyVec[j] = b[k - 1];
					}
				}
				
				// Normalize skyVec for cloud intersection calculation
				float len = std::sqrt(skyVec[0]*skyVec[0] + skyVec[1]*skyVec[1] + skyVec[2]*skyVec[2]);
				if (len > 0.001f) {
					skyVec[0] /= len;
					skyVec[1] /= len;
					skyVec[2] /= len;
				}
				
				// Compute parametric value 'p' that intersects with cloud layer
				// This is a simplified version of OpenGL's calculation
				float dot = skyVec[0]*skyVec[0] + skyVec[1]*skyVec[1] + skyVec[2]*skyVec[2];
				float p = (1.0f / (2.0f * dot)) *
					(-2.0f * skyVec[2] * radiusWorld +
					 2.0f * std::sqrt(
						skyVec[2] * skyVec[2] * radiusWorld * radiusWorld +
						2.0f * skyVec[0] * skyVec[0] * radiusWorld * cloudHeight +
						skyVec[0] * skyVec[0] * cloudHeight * cloudHeight +
						2.0f * skyVec[1] * skyVec[1] * radiusWorld * cloudHeight +
						skyVec[1] * skyVec[1] * cloudHeight * cloudHeight +
						2.0f * skyVec[2] * skyVec[2] * radiusWorld * cloudHeight +
						skyVec[2] * skyVec[2] * cloudHeight * cloudHeight));
				
				// Compute intersection point based on p
				float v[3];
				v[0] = skyVec[0] * p;
				v[1] = skyVec[1] * p;
				v[2] = skyVec[2] * p + radiusWorld;
				
				// Normalize v for texture coord calculation
				len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
				if (len > 0.001f) {
					v[0] /= len;
					v[1] /= len;
					v[2] /= len;
				}
				
				// acos gives angles for texture coordinates
				cloudTexCoords_[i][t][s][0] = std::acos(v[0]);
				cloudTexCoords_[i][t][s][1] = std::acos(v[1]);
			}
		}
	}
	
	cloudTexCoordsInitialized_ = true;
}

// Build cloud sky dome geometry for a sky shader
// Generates a 5-face box dome (like OpenGL) using precomputed cloud texture coordinates
void MetalRenderer::buildCloudSkyDome(qhandle_t skyShader) {
	MetalShaderResource* resource = getShaderResource(skyShader);
	if (!resource || !resource->hasScript || !resource->script.isSky) {
		return;
	}
	
	// Initialize cloud texture coordinates if not done
	float cloudHeight = resource->script.cloudHeight;
	if (cloudHeight <= 0.0f) {
		cloudHeight = 512.0f;  // Default cloud height
	}
	initCloudSkyTexCoords(cloudHeight);
	
	// Clear previous dome data
	cloudDomeVertices_.clear();
	cloudDomeIndices_.clear();
	
	// Safety check for valid camera state
	if (sceneCamera_.zFar <= 0.0f) {
		return;  // Camera not set up yet
	}
	
	const float boxSize = sceneCamera_.zFar / 1.75f;
	const float* viewOrigin = sceneCamera_.viewOrigin;
	
	// st_to_vec mapping from OpenGL
	static const int st_to_vec[6][3] = {
		{3, -1, 2},
		{-3, 1, 2},
		{1, 3, 2},
		{-1, -3, 2},
		{-2, -1, 3},  // up
		{2, -1, -3}   // down
	};
	
	// Generate vertices for all 6 faces of the sky dome
	// We skip face 5 (down) since we never look straight down at clouds
	for (int face = 0; face < 6; ++face) {
		// Skip down face
		if (face == 5) continue;
		
		uint32_t vertexStart = static_cast<uint32_t>(cloudDomeVertices_.size());
		
		// Generate grid of vertices for this face
		for (int t = 0; t <= SKY_SUBDIVISIONS; ++t) {
			for (int s = 0; s <= SKY_SUBDIVISIONS; ++s) {
				// Compute sky position (like MakeSkyVec)
				float sParam = (static_cast<float>(s - HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS);
				float tParam = (static_cast<float>(t - HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS);
				
				float b[3];
				b[0] = sParam * boxSize;
				b[1] = tParam * boxSize;
				b[2] = boxSize;
				
				float xyz[3];
				for (int j = 0; j < 3; ++j) {
					int k = st_to_vec[face][j];
					if (k < 0) {
						xyz[j] = -b[-k - 1];
					} else {
						xyz[j] = b[k - 1];
					}
				}
				
				MetalPolyVertex vert{};
				vert.xyz[0] = viewOrigin[0] + xyz[0];
				vert.xyz[1] = viewOrigin[1] + xyz[1];
				vert.xyz[2] = viewOrigin[2] + xyz[2];
				vert.st[0] = cloudTexCoords_[face][t][s][0];
				vert.st[1] = cloudTexCoords_[face][t][s][1];
				vert.lightmap[0] = 0.0f;
				vert.lightmap[1] = 0.0f;
				vert.normal[0] = 0.0f;
				vert.normal[1] = 0.0f;
				vert.normal[2] = 1.0f;
				vert.modulate[0] = 255;
				vert.modulate[1] = 255;
				vert.modulate[2] = 255;
				vert.modulate[3] = 255;
				
				cloudDomeVertices_.push_back(vert);
			}
		}
		
		// Generate indices for this face (grid of quads -> triangles)
		int width = SKY_SUBDIVISIONS + 1;
		for (int t = 0; t < SKY_SUBDIVISIONS; ++t) {
			for (int s = 0; s < SKY_SUBDIVISIONS; ++s) {
				uint32_t idx00 = vertexStart + t * width + s;
				uint32_t idx10 = vertexStart + t * width + s + 1;
				uint32_t idx01 = vertexStart + (t + 1) * width + s;
				uint32_t idx11 = vertexStart + (t + 1) * width + s + 1;
				
				// Two triangles per quad
				cloudDomeIndices_.push_back(idx00);
				cloudDomeIndices_.push_back(idx10);
				cloudDomeIndices_.push_back(idx11);
				
				cloudDomeIndices_.push_back(idx00);
				cloudDomeIndices_.push_back(idx11);
				cloudDomeIndices_.push_back(idx01);
			}
		}
	}
	
	lastCloudSkyShader_ = skyShader;
}

// Draw cloud sky dome with shader stages
bool MetalRenderer::drawCloudSky(qhandle_t skyShader) {
	if (skyboxRenderedThisFrame_) {
		return true;  // Already drew sky this frame
	}
	
	MetalShaderResource* resource = getShaderResource(skyShader);
	if (!resource || !resource->hasScript || !resource->script.isSky) {
		return false;
	}
	
	// Build cloud dome if needed - rebuild every frame since view position changes
	// The dome is centered on the viewer, so it must move with the camera
	buildCloudSkyDome(skyShader);
	
	if (cloudDomeVertices_.empty()) {
		return false;
	}
	
	if (!currentRenderEncoder_ || !sceneUniformBuffer_) {
		return false;
	}
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) {
		return false;
	}
	
	// Ensure we have stages to draw
	if (resource->stageRuntimes.empty()) {
		ensureScriptHasStages(*resource);
		loadShaderImages(*resource);
		buildShaderStageRuntime(*resource);
	}
	
	if (resource->stageRuntimes.empty()) {
		return false;  // No stages to draw
	}
	
	// Ensure depth state exists
	if (!skyboxDepthState_) {
		ensureSkyboxResources();
	}
	
	// Draw each stage of the cloud shader
	float timeSeconds = static_cast<float>(ri_.Milliseconds()) / 1000.0f;
	
	for (size_t stageIdx = 0; stageIdx < resource->stageRuntimes.size(); ++stageIdx) {
		const auto& stageRuntime = resource->stageRuntimes[stageIdx];
		
		// Get pipeline for this stage
		StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime.pipelineKey);
		if (!pipelineEntry || !pipelineEntry->pipeline) {
			continue;
		}
		
		// Bind pipeline
		MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
		
		// Use sky-appropriate depth state (test but don't write, always at far plane)
		currentRenderEncoder_->setDepthStencilState(skyboxDepthState_.get());
		currentRenderEncoder_->setCullMode(MTL::CullModeNone);  // Draw both sides
		
		// Get texture for this stage
		qhandle_t imageHandle = selectStageImage(*resource, &stageRuntime, timeSeconds, 0);
		MTL::Texture* texture = nullptr;
		if (imageHandle > 0) {
			texture = texManager->getTexture(imageHandle);
		}
		if (!texture) {
			texture = texManager->getTexture(1);  // Default texture
		}
		if (!texture) {
			continue;
		}
		
		// Set up fragment params
		StageFragmentParams fragParams{};
		fragParams.texCoordSelector = 0.0f;  // Use base texcoords
		fragParams.rgbGenType = 1.0f;  // Identity (white)
		fragParams.alphaTestEnabled = stageRuntime.pipelineKey.alphaTest ? 1.0f : 0.0f;
		
		// Apply overbright unless blending multiplies by destination
		float stageOverBright = sceneUniforms_.overBrightBits;
		if (stageRuntime.pipelineKey.srcBlend == MetalBlendFactor::DstColor ||
			stageRuntime.pipelineKey.dstBlend == MetalBlendFactor::SrcColor) {
			stageOverBright = 0.0f;
		}
		fragParams.overBrightBits = stageOverBright;
		
		currentRenderEncoder_->setFragmentBytes(&fragParams, sizeof(StageFragmentParams), 0);
		
		// Set up TCMod params from stage - use existing helper function
		TCModParams tcModParams = computeTCModParams(stageRuntime.stageInfo, timeSeconds);
		currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
		
		// Bind scene uniforms
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
		
		// Create vertex buffer for dome vertices (can't use setVertexBytes - too large)
		auto vertexBuffer = device_->newBuffer(cloudDomeVertices_.data(), 
			cloudDomeVertices_.size() * sizeof(MetalPolyVertex), MTL::ResourceStorageModeShared);
		if (!vertexBuffer) {
			continue;
		}
		currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
		
		// Bind texture
		MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, texture);
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sceneSampler_.get());
		
		// Draw using indices
		if (cloudDomeIndices_.size() <= 65536) {
			// Upload indices and draw
			auto indexBuffer = device_->newBuffer(cloudDomeIndices_.data(), 
				cloudDomeIndices_.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared);
			if (indexBuffer) {
				currentRenderEncoder_->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
					NS::UInteger(cloudDomeIndices_.size()),
					MTL::IndexTypeUInt32,
					indexBuffer, 0);
				indexBuffer->release();
			}
		}
		vertexBuffer->release();
	}
	
	skyboxRenderedThisFrame_ = true;
	
	return true;
}

bool MetalRenderer::drawDynamicLights() {
	// Check if we have lights to render
	if (lightPackets_.empty()) {
		return true;  // No lights
	}

	if (!currentRenderEncoder_ || !polyVertexBuffer_ || polyVertexCountGPU_ == 0) {
		return true;  // No geometry to light
	}

	if (polyPackets_.empty()) {
		return true;  // No surfaces to light
	}

	// Ensure dlight resources are initialized
	if (!ensureDlightResources() || !dlightPipeline_ || !dlightTexture_) {
		return true;  // Can't render without pipeline
	}

	// Set dlight pipeline and resources
	currentRenderEncoder_->setRenderPipelineState(dlightPipeline_.get());
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	currentRenderEncoder_->setFragmentTexture(dlightTexture_.get(), 0);
	currentRenderEncoder_->setFragmentSamplerState(sampler2D_.get(), 0);

	// Create uniform buffer for dlight uniforms
	DlightUniforms uniforms{};

	// Use scene camera matrices for MVP calculation
	// viewProjection is already computed in sceneUniforms_
	for (int i = 0; i < 16; ++i) {
		uniforms.modelViewProjection[i] = sceneUniforms_.viewProjection[i];
	}

	int lightsRendered = 0;
	int surfacesLit = 0;

	// Render each dynamic light
	for (const SceneLightPacket& lightPacket : lightPackets_) {
		const MetalSceneLight& light = lightPacket.light;

		// Skip if light has no intensity
		if (light.intensity <= 0.0f) {
			continue;
		}

		// Calculate light radius from intensity
		// intensity is already the radius in Quake 3
		const float radius = light.intensity;
		if (radius <= 0.0f) {
			continue;
		}

		// Setup dlight uniforms
		uniforms.dlightInfo[0] = light.origin[0];
		uniforms.dlightInfo[1] = light.origin[1];
		uniforms.dlightInfo[2] = light.origin[2];
		uniforms.dlightInfo[3] = 1.0f / radius;  // Inverse radius for shader

		// Light color
		uniforms.color[0] = light.color[0];
		uniforms.color[1] = light.color[1];
		uniforms.color[2] = light.color[2];
		uniforms.color[3] = 1.0f;  // Alpha

		// No deforms for now
		uniforms.deformGen = 0;
		uniforms.time = sceneUniforms_.timeInfo[0];
		uniforms.vertexLerp = 0.0f;

		// Upload uniforms for this light
		currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(DlightUniforms), 1);

		// Draw all surfaces that are within the light's radius
		for (const ScenePolyPacket& packet : polyPackets_) {
			// Simple culling: check if surface bounding sphere intersects light
			// For now, just draw all surfaces (we'll add proper culling later)

			// Skip if no vertices
			if (packet.vertexCount <= 0) {
				continue;
			}

			// Draw the surface with dlight applied
			currentRenderEncoder_->drawPrimitives(
				packet.primitive,
				NS::UInteger(packet.firstVertex),
				NS::UInteger(packet.vertexCount)
			);

			++surfacesLit;
		}

		++lightsRendered;
	}

	if (ri_.Printf && lightsRendered > 0) {
		static int frameCount = 0;
		if (frameCount++ % 60 == 0) {
			ri_.Printf(PRINT_DEVELOPER, "Metal: drawDynamicLights - %d lights, %d surfaces\n",
			          lightsRendered, surfacesLit);
		}
	}

	return true;
}

bool MetalRenderer::drawFogPasses() {
	// Check if fog is enabled
	if (worldFogs_.empty()) {
		return true;  // No fog to render
	}

	if (!currentRenderEncoder_ || !polyVertexBuffer_ || polyVertexCountGPU_ == 0) {
		return true;  // No geometry to fog
	}

	if (!sceneUniformBuffer_) {
		return false;
	}

	// Ensure fog pipeline and depth state are created
	if (!ensureFogPipeline() || !fogPipeline_ || !fogDepthState_) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Fog pipeline not available\n");
		}
		return false;
	}

	// Set fog pipeline and depth state
	currentRenderEncoder_->setRenderPipelineState(fogPipeline_.get());
	currentRenderEncoder_->setDepthStencilState(fogDepthState_.get());
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);  // Allow viewing from inside fog

	// Bind vertex buffer
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);

	// int totalFoggedPackets = 0;
	const float* viewOrigin = sceneCamera_.viewOrigin;

	// Process each fog volume
	for (size_t fogIdx = 0; fogIdx < worldFogs_.size(); ++fogIdx) {
		const FogVolume& fog = worldFogs_[fogIdx];
		const int fogIndex = static_cast<int>(fogIdx) + 1;  // fogIndex is 1-based

		// Update fog uniforms for this volume
		sceneUniforms_.fogColor[0] = fog.fogColor[0];
		sceneUniforms_.fogColor[1] = fog.fogColor[1];
		sceneUniforms_.fogColor[2] = fog.fogColor[2];
		sceneUniforms_.fogColor[3] = 1.0f;

		// fogDistanceVector using fog surface plane
		sceneUniforms_.fogDistanceVector[0] = fog.surface[0] * fog.tcScale;
		sceneUniforms_.fogDistanceVector[1] = fog.surface[1] * fog.tcScale;
		sceneUniforms_.fogDistanceVector[2] = fog.surface[2] * fog.tcScale;
		sceneUniforms_.fogDistanceVector[3] = -(viewOrigin[0] * fog.surface[0] +
		                                        viewOrigin[1] * fog.surface[1] +
		                                        viewOrigin[2] * fog.surface[2]) * fog.tcScale;

		if (fog.hasSurface) {
			sceneUniforms_.fogDepthVector[0] = fog.surface[0];
			sceneUniforms_.fogDepthVector[1] = fog.surface[1];
			sceneUniforms_.fogDepthVector[2] = fog.surface[2];
			sceneUniforms_.fogDepthVector[3] = -fog.surface[3];

			sceneUniforms_.fogEyeT = viewOrigin[0] * sceneUniforms_.fogDepthVector[0] +
			                         viewOrigin[1] * sceneUniforms_.fogDepthVector[1] +
			                         viewOrigin[2] * sceneUniforms_.fogDepthVector[2] +
			                         sceneUniforms_.fogDepthVector[3];
		} else {
			sceneUniforms_.fogDepthVector[0] = 0.0f;
			sceneUniforms_.fogDepthVector[1] = 0.0f;
			sceneUniforms_.fogDepthVector[2] = 0.0f;
			sceneUniforms_.fogDepthVector[3] = 0.0f;
			sceneUniforms_.fogEyeT = 1.0f;  // Always inside
		}

		sceneUniforms_.fogTcScale = fog.tcScale;
		sceneUniforms_.fogHasSurface = fog.hasSurface ? 1.0f : 0.0f;
		sceneUniforms_.fogEnabled = 1.0f;

		// Upload updated uniforms
		std::memcpy(sceneUniformBuffer_->contents(), &sceneUniforms_, sizeof(SceneUniforms));
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
		currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 1);

		// Draw fog pass for surfaces in this fog volume
		for (const ScenePolyPacket& packet : polyPackets_) {
			if (packet.vertexCount <= 0 || packet.fogIndex != fogIndex) {
				continue;
			}

			const size_t endVertex = static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount);
			if (endVertex > polyVertexCountGPU_) {
				continue;
			}

			currentRenderEncoder_->drawPrimitives(packet.primitive,
			                                       static_cast<NS::UInteger>(packet.firstVertex),
			                                       static_cast<NS::UInteger>(packet.vertexCount));
			// totalFoggedPackets++;
		}
	}

	// Debug logging disabled for performance
	// if (ri_.Printf) {
	// 	static int logCounter = 0;
	// 	if (logCounter++ % 300 == 0) {
	// 		ri_.Printf(PRINT_ALL, "Metal: Drew fog passes for %d packets across %zu fog volumes\n", 
	// 		           totalFoggedPackets, worldFogs_.size());
	// 	}
	// }

	return true;
}

//=============================================================================
// Model Rendering (MD3)
//=============================================================================

void MetalRenderer::calculateEntityTransform(const refEntity_t& ent, float* matrix) {
	// Build a column-major 4x4 transform matrix from entity axis and origin
	// ent.axis is a 3x3 rotation matrix (row-major)
	// ent.origin is the translation
	matrix[0] = ent.axis[0][0];  matrix[4] = ent.axis[1][0];  matrix[8] = ent.axis[2][0];   matrix[12] = ent.origin[0];
	matrix[1] = ent.axis[0][1];  matrix[5] = ent.axis[1][1];  matrix[9] = ent.axis[2][1];   matrix[13] = ent.origin[1];
	matrix[2] = ent.axis[0][2];  matrix[6] = ent.axis[1][2];  matrix[10] = ent.axis[2][2];  matrix[14] = ent.origin[2];
	matrix[3] = 0.0f;            matrix[7] = 0.0f;            matrix[11] = 0.0f;             matrix[15] = 1.0f;
}

void MetalRenderer::setupEntityLighting(const refEntity_t& ent, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
	// Determine the lighting origin
	vec3_t lightOrigin;
	if (ent.renderfx & RF_LIGHTING_ORIGIN) {
		VectorCopy(ent.lightingOrigin, lightOrigin);
	} else {
		VectorCopy(ent.origin, lightOrigin);
	}

	// Sample the light grid at the entity's position
	R_LightForPoint(lightOrigin, ambientLight, directedLight, lightDir);

	// Apply bonus minimum light (matches R_SetupEntityLighting in tr_light.cpp)
	// This ensures all entities have some minimum visibility
	// identityLight = 0.5 for overbrightBits=1, so bonus is 0.5 * 32 = 16
	const float identityLight = R_GetIdentityLight();
	ambientLight[0] += identityLight * 32.0f;
	ambientLight[1] += identityLight * 32.0f;
	ambientLight[2] += identityLight * 32.0f;

	// Clamp ambient light to 0-255 range
	for (int i = 0; i < 3; i++) {
		if (ambientLight[i] > 255.0f) ambientLight[i] = 255.0f;
	}

	// DEBUG: Log lighting values
	static int logCount = 0;
	if (ri_.Printf && logCount++ < 10) {
		ri_.Printf(PRINT_ALL, "^3LIGHTING: ambient=(%.1f,%.1f,%.1f), directed=(%.1f,%.1f,%.1f), dir=(%.2f,%.2f,%.2f)\n",
		          ambientLight[0], ambientLight[1], ambientLight[2],
		          directedLight[0], directedLight[1], directedLight[2],
		          lightDir[0], lightDir[1], lightDir[2]);
	}
}

void MetalRenderer::multiplyMatrices4x4(const float* a, const float* b, const float* c, float* result) {
	// Multiply three 4x4 matrices: result = a * b * c (column-major)
	// Use existing Mat4Multiply from tr_extramath.cpp
	float temp[16];
	Mat4Multiply(a, b, temp);
	Mat4Multiply(temp, c, result);
}

MTL::Buffer* MetalRenderer::createModelVertexBuffer(const refEntity_t& ent, MetalModelSurface& surface) {
	if (!device_) {
		return nullptr;
	}

	// Clamp frame numbers to valid range
	int oldFrame = ent.oldframe;
	int newFrame = ent.frame;
	if (oldFrame < 0) oldFrame = 0;
	if (oldFrame >= surface.numFrames) oldFrame = surface.numFrames - 1;
	if (newFrame < 0) newFrame = 0;
	if (newFrame >= surface.numFrames) newFrame = surface.numFrames - 1;

	// Build interleaved vertex data
	// Layout: position(3), normal(3), texcoord(2), position2(3), normal2(3) = 14 floats per vertex
	struct ModelVertex {
		float position[3];
		float normal[3];
		float texcoord[2];
		float position2[3];
		float normal2[3];
	};

	std::vector<ModelVertex> vertices(surface.numVerts);

	for (int i = 0; i < surface.numVerts; i++) {
		ModelVertex& v = vertices[i];

		// Old frame vertex data
		const MetalModelVertex& oldVert = surface.vertices[oldFrame * surface.numVerts + i];
		VectorCopy(oldVert.xyz, v.position);
		VectorCopy(oldVert.normal, v.normal);

		// Texture coordinates (same for all frames)
		v.texcoord[0] = surface.texCoords[i].st[0];
		v.texcoord[1] = surface.texCoords[i].st[1];

		// New frame vertex data
		const MetalModelVertex& newVert = surface.vertices[newFrame * surface.numVerts + i];
		VectorCopy(newVert.xyz, v.position2);
		VectorCopy(newVert.normal, v.normal2);
	}

	size_t bufferSize = vertices.size() * sizeof(ModelVertex);
	return device_->newBuffer(vertices.data(), bufferSize, MTL::ResourceStorageModeShared);
}

void MetalRenderer::renderModelSurface(
	const refEntity_t& ent,
	MetalModelSurface& surface,
	const float* mvpMatrix,
	const float* modelMatrix,
	float vertexLerp,
	const ModelFogParams& fogParams,
	const vec3_t ambientLight,
	const vec3_t directedLight,
	const vec3_t lightDir)
{
	if (!currentRenderEncoder_ || !modelPipeline_ || !modelDepthState_) {
		return;
	}

	// Get scene time for tcMod animations
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;

	// Determine shader to use (matches OpenGL2 tr_mesh.c:357-383)
	// Priority: customShader > customSkin > surface.shaderIndexes > default
	qhandle_t shaderHandle = 0;

	// 1. Check for customShader first
	if (ent.customShader) {
		shaderHandle = ent.customShader;
	}
	// 2. Check for customSkin with surface name match
	else if (ent.customSkin > 0) {
		MetalSkin* skin = getSkinByHandle(ent.customSkin);
		if (skin) {
			char surfaceName[MAX_QPATH];
			Q_strncpyz(surfaceName, surface.name, sizeof(surfaceName));
			Q_strlwr(surfaceName);

			for (int j = 0; j < skin->numSurfaces; ++j) {
				if (strcmp(skin->surfaces[j].name, surfaceName) == 0) {
					shaderHandle = skin->surfaces[j].shader;
					break;
				}
			}
		}
	}

	// 3. Fall back to surface.shaderIndexes if no customShader/customSkin
	if (shaderHandle == 0 && !surface.shaderIndexes.empty()) {
		shaderHandle = surface.shaderIndexes[0];
	}
	
	// 4. Final fallback: use white shader
	if (shaderHandle == 0) {
		shaderHandle = registerShader("white", true);
	}

	// Get shader resource
	const MetalShaderResource* shaderRes = nullptr;
	if (shaderHandle > 0 && static_cast<size_t>(shaderHandle) < shaderResources_.size()) {
		shaderRes = &shaderResources_[shaderHandle];
	}

	// Build interleaved vertex buffer (shared across all passes)
	MTL::Buffer* vertexBuffer = createModelVertexBuffer(ent, surface);
	if (!vertexBuffer) {
		return;
	}

	// Set up common model uniforms (shared across all passes)
	struct ModelUniforms {
		float mvpMatrix[16];
		float modelMatrix[16];
		float vertexLerp;
		float padding[3];
	};

	ModelUniforms uniforms{};
	std::memcpy(uniforms.mvpMatrix, mvpMatrix, sizeof(float) * 16);
	std::memcpy(uniforms.modelMatrix, modelMatrix, sizeof(float) * 16);
	uniforms.vertexLerp = vertexLerp;
	uniforms.padding[0] = uniforms.padding[1] = uniforms.padding[2] = 0.0f;

	// Set up entity lighting parameters (shared across all passes)
	EntityLightingParams lightingParams{};
	VectorScale(ambientLight, 1.0f / 255.0f, lightingParams.ambientLight);
	VectorScale(directedLight, 1.0f / 255.0f, lightingParams.directedLight);
	vec3_t normalizedLightDir;
	VectorNormalize2(lightDir, normalizedLightDir);
	VectorCopy(normalizedLightDir, lightingParams.lightDir);

	vec3_t axis0, axis1, axis2;
	VectorCopy(ent.axis[0], axis0);
	VectorCopy(ent.axis[1], axis1);
	VectorCopy(ent.axis[2], axis2);
	if (ent.nonNormalizedAxes) {
		const float axisLength = VectorLength(axis0);
		const float invLength = (axisLength > 0.0f) ? (1.0f / axisLength) : 1.0f;
		VectorScale(axis0, invLength, axis0);
		VectorScale(axis1, invLength, axis1);
		VectorScale(axis2, invLength, axis2);
	}
	lightingParams.modelLightDir[0] = DotProduct(normalizedLightDir, axis0);
	lightingParams.modelLightDir[1] = DotProduct(normalizedLightDir, axis1);
	lightingParams.modelLightDir[2] = DotProduct(normalizedLightDir, axis2);
	lightingParams.padding0 = lightingParams.padding1 = lightingParams.padding2 = 0.0f;
	lightingParams.overBrightBits = static_cast<float>(r_overBrightBits_ ? r_overBrightBits_->integer : 1);

	// Q3 models use counter-clockwise winding for front faces (OpenGL convention)
	currentRenderEncoder_->setCullMode(MTL::CullModeFront);

	// Get texture manager
	TextureManager* texMgr = ensureTextureManager();

	// ====== MULTI-PASS MODEL RENDERING ======
	// Render each shader stage as a separate pass
	
	const size_t numStages = (shaderRes && !shaderRes->stageRuntimes.empty()) 
	                          ? shaderRes->stageRuntimes.size() : 0;
	
	if (numStages > 0) {
		// Multi-pass rendering for shader with script stages
		for (size_t stageIndex = 0; stageIndex < numStages; stageIndex++) {
			const auto& stageRuntime = shaderRes->stageRuntimes[stageIndex];
			const auto& pipelineKey = stageRuntime.pipelineKey;
			
			// Get or create pipeline for this stage's blend mode
			ModelStagePipelineEntry* stagePipeline = getModelStagePipeline(pipelineKey);
			if (!stagePipeline || !stagePipeline->pipeline) {
				continue;  // Skip this stage if pipeline creation failed
			}
			
			// Set pipeline and depth state for this stage
			currentRenderEncoder_->setRenderPipelineState(stagePipeline->pipeline.get());
			currentRenderEncoder_->setDepthStencilState(stagePipeline->depthState.get());
			
			// Set vertex buffer and uniforms for each pass
			currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
			currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(ModelUniforms), 1);
			currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);
			
			// Set up per-stage parameters (tcGen type and tcMod transforms)
			ModelStageParams stageParams{};
			if (stageRuntime.stageInfo) {
				switch (stageRuntime.stageInfo->tcGen.type) {
					case MetalTCGen::Lightmap:
						stageParams.tcGenType = 1.0f;
						break;
					case MetalTCGen::Environment:
						stageParams.tcGenType = 2.0f;
						break;
					default:
						stageParams.tcGenType = 0.0f;  // Texture
						break;
				}
				
				// Compute tcMod transforms for this stage
				TCModParams tcModParams = computeTCModParams(stageRuntime.stageInfo, sceneTimeSeconds);
				std::memcpy(stageParams.texMatrix0, tcModParams.texMatrix0, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix1, tcModParams.texMatrix1, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix2, tcModParams.texMatrix2, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix3, tcModParams.texMatrix3, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix4, tcModParams.texMatrix4, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix5, tcModParams.texMatrix5, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix6, tcModParams.texMatrix6, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix7, tcModParams.texMatrix7, sizeof(float) * 4);
			}
			
			// Set fragment uniforms
			currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 0);
			currentRenderEncoder_->setFragmentBytes(&fogParams, sizeof(ModelFogParams), 1);
			currentRenderEncoder_->setFragmentBytes(&stageParams, sizeof(ModelStageParams), 2);
			
			// Bind texture for this stage
			qhandle_t textureHandle = stageRuntime.primaryStageImage;
			if (textureHandle == 0 && shaderRes->primaryImageHandle > 0) {
				textureHandle = shaderRes->primaryImageHandle;
			}
			
			if (textureHandle > 0 && texMgr) {
				MTL::Texture* tex = texMgr->getTexture(textureHandle);
				if (tex) {
					currentRenderEncoder_->setFragmentTexture(tex, 0);
					if (sceneSampler_) {
						currentRenderEncoder_->setFragmentSamplerState(sceneSampler_.get(), 0);
					}
				}
			}
			
			// Draw for this stage
			if (surface.indexBuffer) {
				MTL::Buffer* indexBuffer = static_cast<MTL::Buffer*>(surface.indexBuffer);
				currentRenderEncoder_->drawIndexedPrimitives(
					MTL::PrimitiveTypeTriangle,
					surface.numIndexes,
					MTL::IndexTypeUInt32,
					indexBuffer,
					0
				);
			}
		}
	} else {
		// Single-pass fallback for shaders without script stages
		// Use the default model pipeline with alpha blending
		currentRenderEncoder_->setRenderPipelineState(modelPipeline_.get());
		currentRenderEncoder_->setDepthStencilState(modelDepthState_.get());
		
		currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
		currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(ModelUniforms), 1);
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);
		
		// Default stage params (tcGen texture)
		ModelStageParams stageParams{};
		
		currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 0);
		currentRenderEncoder_->setFragmentBytes(&fogParams, sizeof(ModelFogParams), 1);
		currentRenderEncoder_->setFragmentBytes(&stageParams, sizeof(ModelStageParams), 2);
		
		// Bind primary texture
		qhandle_t textureHandle = shaderRes ? shaderRes->primaryImageHandle : 0;
		if (textureHandle > 0 && texMgr) {
			MTL::Texture* tex = texMgr->getTexture(textureHandle);
			if (tex) {
				currentRenderEncoder_->setFragmentTexture(tex, 0);
				if (sceneSampler_) {
					currentRenderEncoder_->setFragmentSamplerState(sceneSampler_.get(), 0);
				}
			}
		}
		
		// Draw
		if (surface.indexBuffer) {
			MTL::Buffer* indexBuffer = static_cast<MTL::Buffer*>(surface.indexBuffer);
			currentRenderEncoder_->drawIndexedPrimitives(
				MTL::PrimitiveTypeTriangle,
				surface.numIndexes,
				MTL::IndexTypeUInt32,
				indexBuffer,
				0
			);
		}
	}

	vertexBuffer->release();
}

void MetalRenderer::renderModel(const refEntity_t& ent, MetalModel& model, MetalModelLOD& lodData,
	int fogIndex, CullResult cullState) {
	if (cullState == CullResult::Out) {
		return;
	}

	const bool fullyVisible = (cullState == CullResult::In);
	(void)fullyVisible; // Placeholder until clip handling is wired in

	// Debug logging for model rendering (commented out - enable for troubleshooting)
	// static int frameCount = 0;
	// static int lastFrameLogged = -1;
	// if (frameCount != lastFrameLogged && frameCount++ < 5) {
	// 	lastFrameLogged = frameCount;
	// 	if (ri_.Printf) {
	// 		ri_.Printf(PRINT_ALL, "^2MODEL_RENDER: '%s' hModel=%d customSkin=%d customShader=%d numSurfaces=%d renderfx=0x%x\n",
	// 		          model.name, ent.hModel, ent.customSkin, ent.customShader,
	// 		          lodData.numSurfaces, ent.renderfx);
	// 	}
	// }

	// Calculate entity transform matrix
	float modelMatrix[16];
	calculateEntityTransform(ent, modelMatrix);

	// Calculate MVP matrix (projection * view * model)
	float mvpMatrix[16];
	multiplyMatrices4x4(sceneCamera_.projectionMatrix, sceneCamera_.viewMatrix, modelMatrix, mvpMatrix);

	const ModelFogParams fogParams = buildModelFogParams(fogIndex);

	// Set up entity lighting
	vec3_t ambientLight, directedLight, lightDir;
	setupEntityLighting(ent, ambientLight, directedLight, lightDir);

	// Calculate vertex interpolation factor
	float vertexLerp = 1.0f - ent.backlerp;

	// Render each surface
	for (int i = 0; i < lodData.numSurfaces; i++) {
		// if (ri_.Printf) {
		// 	ri_.Printf(PRINT_ALL, "DEBUG: renderModel - rendering surface %d/%d\n", i, lodData->numSurfaces);
		// }
		renderModelSurface(ent, lodData.surfaces[i], mvpMatrix, modelMatrix, vertexLerp,
		                   fogParams, ambientLight, directedLight, lightDir);
	}
}

void MetalRenderer::renderBrushModel(const refEntity_t& ent, MetalBrushModel& bmodel) {
	// Brush models render world surfaces with the entity's transform applied
	// They use the same rendering path as world surfaces but with a model matrix
	
	if (!currentRenderEncoder_ || !sceneCamera_.valid) {
		return;
	}
	
	if (!ensureSceneShaderResources()) {
		return;
	}
	
	// Calculate the entity transform matrix
	float modelMatrix[16];
	calculateEntityTransform(ent, modelMatrix);
	
	// Create a modified uniform buffer with the model matrix applied to view-projection
	float brushViewProjection[16];
	Mat4Multiply(sceneCamera_.viewProjectionMatrix, modelMatrix, brushViewProjection);
	
	// Build modified uniforms for brush model rendering
	SceneUniforms brushUniforms;
	std::memcpy(&brushUniforms, sceneUniformBuffer_->contents(), sizeof(SceneUniforms));
	std::memcpy(brushUniforms.viewProjection, brushViewProjection, sizeof(brushViewProjection));
	
	// Create temporary uniform buffer for this brush model
	MTL::Buffer* brushUniformBuffer = device_->newBuffer(&brushUniforms, sizeof(SceneUniforms), MTL::ResourceStorageModeShared);
	if (!brushUniformBuffer) {
		return;
	}
	
	// Find packet indices for this brush model's surfaces
	std::vector<int> packetIndices;
	for (int i = 0; i < bmodel.numSurfaces; i++) {
		int bspSurfaceIndex = bmodel.firstSurface + i;
		if (bspSurfaceIndex >= 0 && bspSurfaceIndex < static_cast<int>(worldSurfaceToPacket_.size())) {
			int packetIndex = worldSurfaceToPacket_[bspSurfaceIndex];
			if (packetIndex >= 0) {
				packetIndices.push_back(packetIndex);
			}
		}
	}
	
	if (packetIndices.empty()) {
		brushUniformBuffer->release();
		return;
	}
	
	// Set up vertex buffer (same as world rendering)
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	currentRenderEncoder_->setVertexBuffer(brushUniformBuffer, 0, 1);
	
	// Setup entity lighting for brush model
	EntityLightingParams lightingParams{};
	vec3_t ambientLight, directedLight, lightDir;
	setupEntityLighting(ent, ambientLight, directedLight, lightDir);
	lightingParams.ambientLight[0] = ambientLight[0];
	lightingParams.ambientLight[1] = ambientLight[1];
	lightingParams.ambientLight[2] = ambientLight[2];
	lightingParams.directedLight[0] = directedLight[0];
	lightingParams.directedLight[1] = directedLight[1];
	lightingParams.directedLight[2] = directedLight[2];
	lightingParams.lightDir[0] = lightDir[0];
	lightingParams.lightDir[1] = lightDir[1];
	lightingParams.lightDir[2] = lightDir[2];
	currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 2);
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) {
		brushUniformBuffer->release();
		return;
	}
	MTL::Texture* defaultTexture = texManager->getTexture(0);
	if (!defaultTexture) {
		brushUniformBuffer->release();
		return;
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
	
	auto buildStageParamsLocal = [&](const MetalShaderStageInfo* stageInfo, float overBrightBits, qhandle_t lightmapHandle) {
		StageFragmentParams params{};
		params.overBrightBits = overBrightBits;
		if (!stageInfo) {
			return params;
		}

		// Set tcGen type
		switch (stageInfo->tcGen.type) {
			case MetalTCGen::Lightmap:
				params.tcGenType = 1.0f;
				params.texCoordSelector = 1.0f;
				break;
			case MetalTCGen::Environment:
				params.tcGenType = 2.0f;
				params.texCoordSelector = 0.0f;
				break;
			default:
				params.tcGenType = 0.0f;
				params.texCoordSelector = 0.0f;
				break;
		}

		// Set rgbGen type - use vertex colors for lightmapped surfaces
		MetalRGBGen effectiveRgbGen = stageInfo->rgbGen.type;
		if (effectiveRgbGen == MetalRGBGen::LightingDiffuse && lightmapHandle > 0 && !stageInfo->usesLightmap) {
			effectiveRgbGen = MetalRGBGen::Vertex;
		}
		
		switch (effectiveRgbGen) {
			case MetalRGBGen::Identity:
				params.rgbGenType = 1.0f;
				break;
			case MetalRGBGen::IdentityLighting:
				params.rgbGenType = 2.0f;
				break;
			case MetalRGBGen::LightingDiffuse:
				params.rgbGenType = 3.0f;
				break;
			default:
				params.rgbGenType = 0.0f;
				break;
		}
		
		return params;
	};
	
	// Render each brush model surface (matching drawPolyPackets logic)
	for (int packetIndex : packetIndices) {
		const ScenePolyPacket& packet = worldPacketTemplate_[packetIndex];
		
		if (packet.vertexCount <= 0) {
			continue;
		}
		
		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (!shaderResource) {
			shaderResource = getShaderResource(0);
		}
		if (!shaderResource || shaderResource->stageRuntimes.empty()) {
			continue;
		}
		
		// Skip sky surfaces on brush models
		if (shaderResource->hasScript && shaderResource->script.isSky) {
			continue;
		}
		
		// Render each stage using stageRuntimes (not stageInfo directly)
		const size_t stageCount = shaderResource->stageRuntimes.size();
		for (size_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
			const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
			if (!stageRuntime) {
				continue;
			}
			const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
			
			// Match OpenGL2's ComputeShaderColors - disable overbright for blend stages
			bool isBlend = (stageRuntime->pipelineKey.srcBlend == MetalBlendFactor::DstColor) ||
			               (stageRuntime->pipelineKey.srcBlend == MetalBlendFactor::OneMinusDstColor) ||
			               (stageRuntime->pipelineKey.dstBlend == MetalBlendFactor::SrcColor) ||
			               (stageRuntime->pipelineKey.dstBlend == MetalBlendFactor::OneMinusSrcColor);
			
			float stageOverBright = isBlend ? 0.0f : (r_overBrightBits_ ? static_cast<float>(r_overBrightBits_->integer) : 1.0f);

			bindStageParams(buildStageParamsLocal(stageInfo, stageOverBright, packet.lightmapHandle));

			// Compute and bind texture coordinate modifications
			TCModParams tcModParams = computeTCModParams(stageInfo, sceneTimeSeconds);
			currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);

			StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
			if (!pipelineEntry || !pipelineEntry->pipeline || !pipelineEntry->depthState) {
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
			
			currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangle,
				static_cast<NS::UInteger>(packet.firstVertex),
				static_cast<NS::UInteger>(packet.vertexCount));
		}
	}
	
	brushUniformBuffer->release();
}

bool MetalRenderer::drawModelEntities() {
	// DEBUG: Log entry to drawModelEntities
	// if (ri_.Printf) {
	// 	ri_.Printf(PRINT_ALL, "DEBUG: drawModelEntities called - drawPackets size=%zu\n", drawPackets_.size());
	// }

	if (drawPackets_.empty()) {
		// if (ri_.Printf) {
		// 	ri_.Printf(PRINT_ALL, "DEBUG: drawModelEntities - no draw packets, returning true\n");
		// }
		return true;
	}

	if (!currentRenderEncoder_ || !sceneCamera_.valid) {
		// if (ri_.Printf) {
		// 	ri_.Printf(PRINT_ALL, "DEBUG: drawModelEntities - encoder or camera invalid (encoder=%p, camera.valid=%d)\n",
		// 	          currentRenderEncoder_, sceneCamera_.valid);
		// }
		return false;
	}

	// Ensure model pipeline is created
	if (!ensureModelPipeline() || !modelPipeline_) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Model pipeline not available\n");
		}
		return false;
	}

	// Render all model entities
	for (const SceneDrawPacket& packet : drawPackets_) {
		const refEntity_t& ent = packet.entity;
		if (ent.reType != RT_MODEL) {
			continue;
		}

		// Don't render third-person models in first-person view
		bool isThirdPerson = (ent.renderfx & RF_THIRD_PERSON) != 0;
		bool personalModel = isThirdPerson; // TODO: exclude portal views when implemented
		if (personalModel) {
			continue;
		}

		// Resolve model handle
		if (ent.hModel <= 0 || static_cast<size_t>(ent.hModel) >= models_.size()) {
			continue;
		}
		MetalModel* model = models_[ent.hModel];
		if (!model || model->type == MetalModelType::BAD) {
			continue;
		}

		// Handle brush models (doors, platforms, movers)
		if (model->type == MetalModelType::BRUSH) {
			if (model->bmodel) {
				renderBrushModel(ent, *model->bmodel);
			}
			continue;
		}

		// Handle regular models (MD3, etc.)
		int lod = computeModelLod(*model, ent);
		if (lod < 0) {
			lod = 0;
		}
		if (lod >= model->numLods || !model->lods[lod]) {
			continue;
		}
		MetalModelLOD* lodData = model->lods[lod];
		if (!lodData) {
			continue;
		}

		const CullResult cullState = cullModel(*lodData, ent);
		if (cullState == CullResult::Out) {
			continue;
		}

		const int fogIndex = computeModelFogIndex(*lodData, ent);
		renderModel(ent, *model, *lodData, fogIndex, cullState);
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
	sceneCamera_.frustumValid = false;
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
	buildFrustumPlanes(sceneCamera_);
	
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

void MetalRenderer::buildFrustumPlanes(SceneCamera& camera) {
	// Extract frustum planes from combined view-projection matrix (column-major order)
	const float* m = camera.viewProjectionMatrix;
	auto setPlane = [&](int index, float a, float b, float c, float d) {
		float length = std::sqrt(a * a + b * b + c * c);
		if (length <= 0.0f) {
			camera.frustumPlanes[index][0] = camera.frustumPlanes[index][1] =
			camera.frustumPlanes[index][2] = camera.frustumPlanes[index][3] = 0.0f;
			return;
		}
		float invLen = 1.0f / length;
		camera.frustumPlanes[index][0] = a * invLen;
		camera.frustumPlanes[index][1] = b * invLen;
		camera.frustumPlanes[index][2] = c * invLen;
		camera.frustumPlanes[index][3] = d * invLen;
	};

	setPlane(0, m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]); // Left
	setPlane(1, m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]); // Right
	setPlane(2, m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]); // Bottom
	setPlane(3, m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]); // Top
	setPlane(4, m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]); // Near
	setPlane(5, m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]); // Far

	camera.frustumValid = true;
}

float MetalRenderer::projectRadius(float radius, const vec3_t location) const {
	if (!sceneCamera_.valid) {
		return 0.0f;
	}

	const vec3_t& axis = sceneCamera_.viewAxis[0];
	const vec3_t& origin = sceneCamera_.viewOrigin;
	const float c = DotProduct(axis, origin);
	const float dist = DotProduct(axis, location) - c;
	if (dist <= 0.0f) {
		return 0.0f;
	}

	vec3_t p;
	p[0] = 0.0f;
	p[1] = std::fabs(radius);
	p[2] = -dist;

	const float* proj = sceneCamera_.projectionMatrix;
	float projected[4];
	projected[0] = p[0] * proj[0]  + p[1] * proj[4]  + p[2] * proj[8]  + proj[12];
	projected[1] = p[0] * proj[1]  + p[1] * proj[5]  + p[2] * proj[9]  + proj[13];
	projected[2] = p[0] * proj[2]  + p[1] * proj[6]  + p[2] * proj[10] + proj[14];
	projected[3] = p[0] * proj[3]  + p[1] * proj[7]  + p[2] * proj[11] + proj[15];

	if (projected[3] == 0.0f) {
		return 0.0f;
	}

	float pr = projected[1] / projected[3];
	if (pr > 1.0f) {
		pr = 1.0f;
	}
	if (pr < 0.0f) {
		pr = 0.0f;
	}
	return pr;
}

MetalRenderer::CullResult MetalRenderer::cullBoundingSphere(const vec3_t center, float radius) const {
	if (!sceneCamera_.frustumValid) {
		return CullResult::In;
	}

	bool clipped = false;
	const bool useFarPlane = !(sceneCamera_.refdef.rdflags & RDF_NOWORLDMODEL);
	const int numPlanes = useFarPlane ? 6 : 5;

	for (int i = 0; i < numPlanes; ++i) {
		const float* plane = sceneCamera_.frustumPlanes[i];
		const float dist = plane[0] * center[0] + plane[1] * center[1] + plane[2] * center[2] + plane[3];
		if (dist <= -radius) {
			return CullResult::Out;
		}
		if (dist < radius) {
			clipped = true;
		}
	}

	return clipped ? CullResult::Clip : CullResult::In;
}

void MetalRenderer::transformModelPoint(const refEntity_t& ent, const float point[3], vec3_t out) const {
	out[0] = point[0] * ent.axis[0][0] + point[1] * ent.axis[1][0] + point[2] * ent.axis[2][0] + ent.origin[0];
	out[1] = point[0] * ent.axis[0][1] + point[1] * ent.axis[1][1] + point[2] * ent.axis[2][1] + ent.origin[1];
	out[2] = point[0] * ent.axis[0][2] + point[1] * ent.axis[1][2] + point[2] * ent.axis[2][2] + ent.origin[2];
}

MetalRenderer::CullResult MetalRenderer::cullBoundingBox(const float mins[3], const float maxs[3], const refEntity_t& ent) const {
	if (!sceneCamera_.frustumValid) {
		return CullResult::In;
	}

	vec3_t worldMins;
	vec3_t worldMaxs;
	ClearBounds(worldMins, worldMaxs);

	for (int i = 0; i < 8; ++i) {
		vec3_t local;
		local[0] = (i & 1) ? maxs[0] : mins[0];
		local[1] = (i & 2) ? maxs[1] : mins[1];
		local[2] = (i & 4) ? maxs[2] : mins[2];
		vec3_t world;
		transformModelPoint(ent, local, world);
		AddPointToBounds(world, worldMins, worldMaxs);
	}

	bool clipped = false;
	const bool useFarPlane = !(sceneCamera_.refdef.rdflags & RDF_NOWORLDMODEL);
	const int numPlanes = useFarPlane ? 6 : 5;

	for (int i = 0; i < numPlanes; ++i) {
		const float* plane = sceneCamera_.frustumPlanes[i];
		vec3_t positive;
		positive[0] = (plane[0] >= 0.0f) ? worldMaxs[0] : worldMins[0];
		positive[1] = (plane[1] >= 0.0f) ? worldMaxs[1] : worldMins[1];
		positive[2] = (plane[2] >= 0.0f) ? worldMaxs[2] : worldMins[2];
		const float dist = plane[0] * positive[0] + plane[1] * positive[1] + plane[2] * positive[2] + plane[3];
		if (dist < 0.0f) {
			return CullResult::Out;
		}

		vec3_t negative;
		negative[0] = (plane[0] >= 0.0f) ? worldMins[0] : worldMaxs[0];
		negative[1] = (plane[1] >= 0.0f) ? worldMins[1] : worldMaxs[1];
		negative[2] = (plane[2] >= 0.0f) ? worldMins[2] : worldMaxs[2];
		const float negDist = plane[0] * negative[0] + plane[1] * negative[1] + plane[2] * negative[2] + plane[3];
		if (negDist < 0.0f) {
			clipped = true;
		}
	}

	return clipped ? CullResult::Clip : CullResult::In;
}

MetalRenderer::CullResult MetalRenderer::cullModel(const MetalModelLOD& lod, const refEntity_t& ent) const {
	if (lod.numFrames <= 0 || lod.frames.empty()) {
		return CullResult::In;
	}

	const int numFrames = lod.numFrames;
	const int newFrameIndex = std::clamp(ent.frame, 0, numFrames - 1);
	const int oldFrameIndex = std::clamp(ent.oldframe, 0, numFrames - 1);
	const MetalModelFrame& newFrame = lod.frames[newFrameIndex];
	const MetalModelFrame& oldFrame = lod.frames[oldFrameIndex];

	if (!ent.nonNormalizedAxes) {
		vec3_t newCenter;
		transformModelPoint(ent, newFrame.localOrigin, newCenter);
		if (ent.frame == ent.oldframe) {
			const CullResult sphereCull = cullBoundingSphere(newCenter, newFrame.radius);
			if (sphereCull != CullResult::Clip) {
				return sphereCull;
			}
		} else {
			const CullResult sphereCullNew = cullBoundingSphere(newCenter, newFrame.radius);
			CullResult sphereCullOld;
			if (newFrameIndex == oldFrameIndex) {
				sphereCullOld = sphereCullNew;
			} else {
				vec3_t oldCenter;
				transformModelPoint(ent, oldFrame.localOrigin, oldCenter);
				sphereCullOld = cullBoundingSphere(oldCenter, oldFrame.radius);
			}

			if (sphereCullNew == sphereCullOld) {
				if (sphereCullNew == CullResult::Out) {
					return CullResult::Out;
				}
				if (sphereCullNew == CullResult::In) {
					return CullResult::In;
				}
			}
		}
	}

	vec3_t mergedMins;
	vec3_t mergedMaxs;
	for (int i = 0; i < 3; ++i) {
		mergedMins[i] = std::min(oldFrame.bounds[0][i], newFrame.bounds[0][i]);
		mergedMaxs[i] = std::max(oldFrame.bounds[1][i], newFrame.bounds[1][i]);
	}

	return cullBoundingBox(mergedMins, mergedMaxs, ent);
}

int MetalRenderer::computeModelLod(const MetalModel& model, const refEntity_t& ent) const {
	if (model.numLods <= 1 || !model.lods[0]) {
		return 0;
	}

	const MetalModelLOD* baseLod = model.lods[0];
	if (!baseLod || baseLod->numFrames <= 0 || baseLod->frames.empty()) {
		return 0;
	}

	const int numFrames = baseLod->numFrames;
	int frameIndex = ent.frame;
	if (frameIndex < 0 || frameIndex >= numFrames) {
		frameIndex = 0;
	}
	const MetalModelFrame& frame = baseLod->frames[frameIndex];

	const float radius = frame.radius;
	float projectedRadius = projectRadius(radius, ent.origin);
	float flod;
	if (projectedRadius != 0.0f) {
		float lodscale = r_lodScale_ ? r_lodScale_->value : 1.0f;
		if (lodscale > 20.0f) lodscale = 20.0f;
		if (lodscale < 0.0f) lodscale = 0.0f;
		flod = 1.0f - projectedRadius * lodscale;
	} else {
		flod = 0.0f;
	}

	flod *= static_cast<float>(model.numLods);
	int lod = static_cast<int>(flod);
	if (lod < 0) {
		lod = 0;
	} else if (lod >= model.numLods) {
		lod = model.numLods - 1;
	}

	if (r_lodBias_) {
		lod += r_lodBias_->integer;
		if (lod < 0) {
			lod = 0;
		} else if (lod >= model.numLods) {
			lod = model.numLods - 1;
		}
	}

	return lod;
}

int MetalRenderer::computeModelFogIndex(const MetalModelLOD& lod, const refEntity_t& ent) const {
	if (sceneCamera_.refdef.rdflags & RDF_NOWORLDMODEL) {
		return 0;
	}
	if (worldFogs_.empty() || lod.frames.empty() || lod.numFrames <= 0) {
		return 0;
	}

	const int frameIndex = std::clamp(ent.frame, 0, lod.numFrames - 1);
	const MetalModelFrame& frame = lod.frames[frameIndex];

	vec3_t worldOrigin;
	transformModelPoint(ent, frame.localOrigin, worldOrigin);

	for (size_t i = 0; i < worldFogs_.size(); ++i) {
		const FogVolume& fog = worldFogs_[i];
		bool inside = true;
		for (int axis = 0; axis < 3; ++axis) {
			if (worldOrigin[axis] - frame.radius >= fog.bounds[1][axis]) {
				inside = false;
				break;
			}
			if (worldOrigin[axis] + frame.radius <= fog.bounds[0][axis]) {
				inside = false;
				break;
			}
		}
		if (inside) {
			return static_cast<int>(i) + 1; // Fog indices are 1-based
		}
	}

	return 0;
}

MetalRenderer::ModelFogParams MetalRenderer::buildModelFogParams(int fogIndex) const {
	ModelFogParams params;
	params.fogColor[0] = 0.0f;
	params.fogColor[1] = 0.0f;
	params.fogColor[2] = 0.0f;
	params.fogColor[3] = 1.0f;
	params.fogDistanceVector[0] = 0.0f;
	params.fogDistanceVector[1] = 0.0f;
	params.fogDistanceVector[2] = 0.0f;
	params.fogDistanceVector[3] = 0.0f;
	params.fogDepthVector[0] = 0.0f;
	params.fogDepthVector[1] = 0.0f;
	params.fogDepthVector[2] = 0.0f;
	params.fogDepthVector[3] = 0.0f;
	params.fogEyeT = 0.0f;
	params.fogTcScale = 0.0f;
	params.fogEnabled = 0.0f;
	params.fogHasSurface = 0.0f;

	if (fogIndex <= 0 || !sceneCamera_.valid) {
		return params;
	}

	const size_t fogIdx = static_cast<size_t>(fogIndex - 1);
	if (fogIdx >= worldFogs_.size()) {
		return params;
	}

	const FogVolume& fog = worldFogs_[fogIdx];
	params.fogColor[0] = std::clamp(fog.fogColor[0], 0.0f, 1.0f);
	params.fogColor[1] = std::clamp(fog.fogColor[1], 0.0f, 1.0f);
	params.fogColor[2] = std::clamp(fog.fogColor[2], 0.0f, 1.0f);
	params.fogColor[3] = 1.0f;

	params.fogDistanceVector[0] = fog.surface[0] * fog.tcScale;
	params.fogDistanceVector[1] = fog.surface[1] * fog.tcScale;
	params.fogDistanceVector[2] = fog.surface[2] * fog.tcScale;
	const float viewDot = sceneCamera_.viewOrigin[0] * fog.surface[0] +
	                      sceneCamera_.viewOrigin[1] * fog.surface[1] +
	                      sceneCamera_.viewOrigin[2] * fog.surface[2];
	params.fogDistanceVector[3] = -viewDot * fog.tcScale;
	params.fogDistanceVector[3] += 1.0f / 512.0f;

	params.fogTcScale = fog.tcScale;
	params.fogHasSurface = fog.hasSurface ? 1.0f : 0.0f;

	if (fog.hasSurface) {
		params.fogDepthVector[0] = fog.surface[0];
		params.fogDepthVector[1] = fog.surface[1];
		params.fogDepthVector[2] = fog.surface[2];
		params.fogDepthVector[3] = -fog.surface[3];
		params.fogEyeT = sceneCamera_.viewOrigin[0] * params.fogDepthVector[0] +
		                 sceneCamera_.viewOrigin[1] * params.fogDepthVector[1] +
		                 sceneCamera_.viewOrigin[2] * params.fogDepthVector[2] +
		                 params.fogDepthVector[3];
	} else {
		params.fogDepthVector[0] = 0.0f;
		params.fogDepthVector[1] = 0.0f;
		params.fogDepthVector[2] = 0.0f;
		params.fogDepthVector[3] = 0.0f;
		params.fogEyeT = 1.0f;
	}
	params.fogEnabled = 1.0f;

	return params;
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

	// Initialize model system - allocate BAD model at index 0
	if (models_.empty()) {
		MetalModel* badModel = MetalModel_Alloc(0);
		badModel->type = MetalModelType::BAD;
		models_.push_back(badModel);
	}

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

//=============================================================================
// DYNAMIC LIGHTING
//=============================================================================

void MetalRenderer::createDlightTexture() {
	if (!device_) return;

	// Create radial falloff texture for dynamic lights
	// Matches tr_image.c R_CreateDlightImage() from OpenGL renderer
	constexpr int DLIGHT_SIZE = 16;
	byte data[DLIGHT_SIZE][DLIGHT_SIZE][4];

	for (int x = 0; x < DLIGHT_SIZE; x++) {
		for (int y = 0; y < DLIGHT_SIZE; y++) {
			// Calculate distance from center
			float dx = (x - DLIGHT_SIZE/2 + 0.5f) / (DLIGHT_SIZE/2.0f);
			float dy = (y - DLIGHT_SIZE/2 + 0.5f) / (DLIGHT_SIZE/2.0f);
			float d = sqrt(dx*dx + dy*dy);

			// Radial falloff
			float intensity = 1.0f - d;
			if (intensity < 0.0f) intensity = 0.0f;
			intensity = intensity * intensity;  // Quadratic falloff

			byte b = static_cast<byte>(intensity * 255.0f);
			data[y][x][0] = b;
			data[y][x][1] = b;
			data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}

	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
	desc->setWidth(DLIGHT_SIZE);
	desc->setHeight(DLIGHT_SIZE);
	desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	desc->setTextureType(MTL::TextureType2D);
	desc->setStorageMode(MTL::StorageModeShared);
	desc->setUsage(MTL::TextureUsageShaderRead);

	dlightTexture_.reset(device_->newTexture(desc));
	desc->release();

	if (dlightTexture_) {
		MTL::Region region(0, 0, DLIGHT_SIZE, DLIGHT_SIZE);
		dlightTexture_->replaceRegion(region, 0, &data[0][0][0], DLIGHT_SIZE * 4);
	}
}

bool MetalRenderer::ensureDlightResources() {
	if (dlightPipeline_) {
		return true; // Already initialized
	}

	if (!device_ || !sceneLibrary_) {
		return false;
	}

	// Load dlight shader functions
	NS::String* dlightVertName = NS::String::string("vertex_dlight", NS::UTF8StringEncoding);
	NS::String* dlightFragName = NS::String::string("fragment_dlight", NS::UTF8StringEncoding);

	dlightVertexFunction_.reset(sceneLibrary_->newFunction(dlightVertName));
	dlightFragmentFunction_.reset(sceneLibrary_->newFunction(dlightFragName));

	if (!dlightVertexFunction_ || !dlightFragmentFunction_) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to load dlight shader functions\n");
		}
		return false;
	}

	// Create dlight pipeline
	MTL::RenderPipelineDescriptor* pipelineDesc = MTL::RenderPipelineDescriptor::alloc()->init();
	pipelineDesc->setVertexFunction(dlightVertexFunction_.get());
	pipelineDesc->setFragmentFunction(dlightFragmentFunction_.get());

	// Use the same vertex descriptor as scene rendering
	if (sceneVertexDescriptor_) {
		pipelineDesc->setVertexDescriptor(sceneVertexDescriptor_.get());
	}

	// Color attachment
	MTL::RenderPipelineColorAttachmentDescriptor* colorAttachment = pipelineDesc->colorAttachments()->object(0);
	colorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

	// Blend mode for additive or multiplicative dlights
	// Additive: ONE + ONE, Multiplicative: DST_COLOR + ONE
	colorAttachment->setBlendingEnabled(true);
	colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
	colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

	// Depth attachment
	pipelineDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	NS::Error* error = nullptr;
	dlightPipeline_.reset(device_->newRenderPipelineState(pipelineDesc, &error));
	pipelineDesc->release();

	if (!dlightPipeline_) {
		if (ri_.Printf && error) {
			NS::String* errDesc = error->localizedDescription();
			ri_.Printf(PRINT_WARNING, "Metal: Failed to create dlight pipeline: %s\n",
			           errDesc ? errDesc->utf8String() : "unknown error");
		}
		return false;
	}

	// Create dlight texture
	createDlightTexture();

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: Dlight resources initialized\n");
	}

	return true;
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

int MetalBackend_LerpTag(orientation_t* tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char* tagName) {
	// Metal_LogRendererCall("re.LerpTag");
	return g_renderer.lerpTag(tag, handle, startFrame, endFrame, frac, tagName);
}

void MetalBackend_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs) {
	// Metal_LogRendererCall("re.ModelBounds");
	g_renderer.modelBounds(handle, mins, maxs);
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

bool MetalRenderer::initialize(refimport_t imports) {
	ri_ = imports;
	ri = imports;
	registerConsoleCommands();
	r_lodBias_ = ri_.Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
	r_lodScale_ = ri_.Cvar_Get("r_lodscale", "5", CVAR_CHEAT);
	r_znear_ = ri_.Cvar_Get("r_znear", "4", CVAR_CHEAT);
	if (ri_.Cvar_CheckRange) {
		ri_.Cvar_CheckRange(r_znear_, 0.001f, 200.0f, qfalse);
	}
	r_zproj_ = ri_.Cvar_Get("r_zproj", "64", CVAR_ARCHIVE);
	r_stereoSeparation_ = ri_.Cvar_Get("r_stereoSeparation", "64", CVAR_ARCHIVE);
	r_metalLogCalls_ = ri_.Cvar_Get("r_metalLogCalls", "0", CVAR_TEMP);
	r_swapInterval_ = ri_.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mapOverBrightBits_ = ri_.Cvar_Get("r_mapOverBrightBits", "2", CVAR_ARCHIVE | CVAR_LATCH);
	r_overBrightBits_ = ri_.Cvar_Get("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH);

	// Lighting console variables (matching OpenGL2 renderer)
	r_ambientScale = ri_.Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
	r_directedScale = ri_.Cvar_Get("r_directedScale", "1", CVAR_CHEAT);
	r_debugLight = ri_.Cvar_Get("r_debugLight", "0", CVAR_TEMP);
	r_dlightMode = ri_.Cvar_Get("r_dlightMode", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// Initialize lighting system
	R_InitLightingSystem();

	// Set identity light based on overbright bits
	// Matches OpenGL2: tr.identityLight = 1.0f / ( 1 << tr.overbrightBits )
	{
		int overbrightBits = r_overBrightBits_ ? r_overBrightBits_->integer : 1;
		int mapOverbrightBits = r_mapOverBrightBits_ ? r_mapOverBrightBits_->integer : 2;
		
		// Clamp overbright bits like OpenGL2 does
		if (overbrightBits > 2) overbrightBits = 2;
		if (overbrightBits < 0) overbrightBits = 0;
		if (overbrightBits > mapOverbrightBits) overbrightBits = mapOverbrightBits;
		
		R_SetIdentityLight(overbrightBits);
	}

	resetShaderCaches();
	// TextureManager will be created when device is available
	return true;
}
