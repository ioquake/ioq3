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
#include "../renderercommon/iqm.h"
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
	// --- 8 scalars (offsets 0-28, 4 bytes each) ---
	float alphaRef = 0.0f;
	float alphaFunc = 0.0f;
	float alphaTestEnabled = 0.0f;
	float texCoordSelector = 0.0f;  // Legacy - now use tcGenType instead
	// rgbGenType: 0=Vertex, 1=Identity, 2=IdentityLighting, 3=LightingDiffuse, 4=Wave,
	//             5=Const, 6=Entity, 7=OneMinusEntity, 8=OneMinusVertex
	float rgbGenType = 0.0f;
	// tcGenType: 0=Texture, 1=Lightmap, 2=Environment, 3=Vector, 4=Fog
	float tcGenType = 0.0f;
	float overBrightBits = 0.0f;
	float waveColorScale = 1.0f;  // For rgbGen wave - computed wave value
	// --- 4 scalars (offsets 32-44) then float4s at 16-byte-aligned offsets ---
	// alphaGenType: 0=Identity/default, 1=Entity, 2=OneMinusEntity, 3=Wave, 4=Specular, 5=Portal
	float alphaGenType = 0.0f;
	float alphaWaveValue = 1.0f;  // Precomputed clamped wave value for AGEN_WAVEFORM
	float portalRange = 256.0f;   // View-distance threshold for AGEN_PORTAL
	float _pad0 = 0.0f;           // Align next float4 to offset 48 (16-byte boundary)
	// --- float4 members (16-byte aligned, offsets 48-111) ---
	float entityColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // Entity shaderRGBA / 255
	float constColor[4]  = {1.0f, 1.0f, 1.0f, 1.0f};  // CGEN_CONST constant color
	float tcGenSVector[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // TCGEN_VECTOR s-axis (w unused)
	float tcGenTVector[4] = {0.0f, 1.0f, 0.0f, 0.0f}; // TCGEN_VECTOR t-axis (w unused)
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

// PShadow uniform structs — layout must exactly match scene.metal structs of the same name.
struct alignas(16) PShadowCasterUniforms {
	float lightOrigin[4];   // xyz = light origin, w = viewRadius
	float lightForward[4];  // xyz = axis[0] (toward scene), w = lightRadius
	float lightRight[4];    // xyz = axis[1], w = unused
	float lightUp[4];       // xyz = axis[2], w = unused
};
static_assert(sizeof(PShadowCasterUniforms) == 64, "PShadowCasterUniforms size mismatch");

struct alignas(16) PShadowReceiverUniforms {
	float lightOrigin[4];   // xyz = light origin, w = lightRadius
	float lightForward[4];  // xyz = axis[0] (unscaled)
	float lightRight[4];    // xyz = axis[1] / viewRadius (SCALED)
	float lightUp[4];       // xyz = axis[2] / viewRadius (SCALED)
};
static_assert(sizeof(PShadowReceiverUniforms) == 64, "PShadowReceiverUniforms size mismatch");

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
// Bezier Patch Tessellation — adaptive subdivision + T-junction stitching
// Matches GL2's R_SubdividePatchToGrid / R_StitchPatches algorithm
//=============================================================================

namespace {

// Maximum tessellated grid dimensions, matching GL2's MAX_GRID_SIZE
constexpr int METAL_MAX_GRID_SIZE = 65;

// Tessellated patch grid: holds the subdivided control mesh before triangle conversion.
// Large struct (~182 KB); always heap-allocate (e.g. via std::unique_ptr or std::vector).
struct PatchGrid {
	int   width  = 0;
	int   height = 0;
	MetalPolyVertex ctrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE];
	float widthLodError[METAL_MAX_GRID_SIZE]  = {};
	float heightLodError[METAL_MAX_GRID_SIZE] = {};
};

// ---------------------------------------------------------------------------
// Vertex helpers
// ---------------------------------------------------------------------------

inline MetalPolyVertex LerpVertex(const MetalPolyVertex& a, const MetalPolyVertex& b, float t) {
	MetalPolyVertex out{};
	const float omt = 1.0f - t;
	out.xyz[0]      = a.xyz[0]      * omt + b.xyz[0]      * t;
	out.xyz[1]      = a.xyz[1]      * omt + b.xyz[1]      * t;
	out.xyz[2]      = a.xyz[2]      * omt + b.xyz[2]      * t;
	out.st[0]       = a.st[0]       * omt + b.st[0]       * t;
	out.st[1]       = a.st[1]       * omt + b.st[1]       * t;
	out.lightmap[0] = a.lightmap[0] * omt + b.lightmap[0] * t;
	out.lightmap[1] = a.lightmap[1] * omt + b.lightmap[1] * t;
	out.normal[0]   = a.normal[0]   * omt + b.normal[0]   * t;
	out.normal[1]   = a.normal[1]   * omt + b.normal[1]   * t;
	out.normal[2]   = a.normal[2]   * omt + b.normal[2]   * t;
	out.modulate[0] = static_cast<byte>(a.modulate[0] * omt + b.modulate[0] * t);
	out.modulate[1] = static_cast<byte>(a.modulate[1] * omt + b.modulate[1] * t);
	out.modulate[2] = static_cast<byte>(a.modulate[2] * omt + b.modulate[2] * t);
	out.modulate[3] = static_cast<byte>(a.modulate[3] * omt + b.modulate[3] * t);
	return out;
}

inline MetalPolyVertex MidVert(const MetalPolyVertex& a, const MetalPolyVertex& b) {
	return LerpVertex(a, b, 0.5f);
}

// ---------------------------------------------------------------------------
// Grid helpers — direct ports of GL2's tr_curve.c static functions
// ---------------------------------------------------------------------------

// Rearranges ctrl data in place; caller must swap width/height afterwards.
// Matches GL2's Transpose().
static void GridTranspose(int width, int height,
                          MetalPolyVertex ctrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE]) {
	if (width > height) {
		for (int i = 0; i < height; i++) {
			for (int j = i + 1; j < width; j++) {
				if (j < height)
					std::swap(ctrl[j][i], ctrl[i][j]);
				else
					ctrl[j][i] = ctrl[i][j];
			}
		}
	} else {
		for (int i = 0; i < width; i++) {
			for (int j = i + 1; j < height; j++) {
				if (j < width)
					std::swap(ctrl[i][j], ctrl[j][i]);
				else
					ctrl[i][j] = ctrl[j][i];
			}
		}
	}
}

static void GridInvertCtrl(int width, int height,
                           MetalPolyVertex ctrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE]) {
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width / 2; j++)
			std::swap(ctrl[i][j], ctrl[i][width - 1 - j]);
}

// Matches GL2's InvertErrorTable().
static void GridInvertErrorTable(float errorTable[2][METAL_MAX_GRID_SIZE],
                                 int width, int height) {
	float copy[2][METAL_MAX_GRID_SIZE];
	Com_Memcpy(copy, errorTable, sizeof(copy));
	for (int i = 0; i < width;  i++) errorTable[1][i] = copy[0][i];
	for (int i = 0; i < height; i++) errorTable[0][i] = copy[1][height - 1 - i];
}

// Snap odd-indexed approximation control points onto the actual Bezier curve.
// Matches GL2's PutPointsOnCurve().
static void GridPutPointsOnCurve(MetalPolyVertex ctrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE],
                                 int width, int height) {
	for (int i = 0; i < width; i++)
		for (int j = 1; j < height; j += 2)
			ctrl[j][i] = MidVert(MidVert(ctrl[j][i], ctrl[j+1][i]),
			                     MidVert(ctrl[j][i], ctrl[j-1][i]));
	for (int j = 0; j < height; j++)
		for (int i = 1; i < width; i += 2)
			ctrl[j][i] = MidVert(MidVert(ctrl[j][i], ctrl[j][i+1]),
			                     MidVert(ctrl[j][i], ctrl[j][i-1]));
}

// Compute smooth per-vertex normals via averaged cross-products of adjacent edge pairs.
static void GridMakeNormals(MetalPolyVertex ctrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE],
                            int width, int height) {
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			float sum[3] = { 0.f, 0.f, 0.f };
			const float* base = ctrl[j][i].xyz;

			auto accum = [&](int di0, int dj0, int di1, int dj1) {
				int i0 = i + di0, j0 = j + dj0;
				int i1 = i + di1, j1 = j + dj1;
				if (i0 < 0 || i0 >= width  || j0 < 0 || j0 >= height) return;
				if (i1 < 0 || i1 >= width  || j1 < 0 || j1 >= height) return;
				const float* p0 = ctrl[j0][i0].xyz;
				const float* p1 = ctrl[j1][i1].xyz;
				float e0[3] = { p0[0]-base[0], p0[1]-base[1], p0[2]-base[2] };
				float e1[3] = { p1[0]-base[0], p1[1]-base[1], p1[2]-base[2] };
				float nx = e0[1]*e1[2] - e0[2]*e1[1];
				float ny = e0[2]*e1[0] - e0[0]*e1[2];
				float nz = e0[0]*e1[1] - e0[1]*e1[0];
				float len = sqrtf(nx*nx + ny*ny + nz*nz);
				if (len > 0.001f) { sum[0] += nx/len; sum[1] += ny/len; sum[2] += nz/len; }
			};

			accum( 1, 0,  0,  1);
			accum( 0, 1, -1,  0);
			accum(-1, 0,  0, -1);
			accum( 0,-1,  1,  0);

			float len = sqrtf(sum[0]*sum[0] + sum[1]*sum[1] + sum[2]*sum[2]);
			if (len > 0.001f) {
				ctrl[j][i].normal[0] = sum[0] / len;
				ctrl[j][i].normal[1] = sum[1] / len;
				ctrl[j][i].normal[2] = sum[2] / len;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Adaptive tessellation — direct port of GL2's R_SubdividePatchToGrid()
//
// subdivLevel: world-space flatness tolerance in game units.
//   Keep subdividing while the perpendicular deviation of a Bezier midpoint
//   from its chord exceeds this value.  Matches r_subdivisions cvar semantics.
// ---------------------------------------------------------------------------
static PatchGrid AdaptiveTessellate(const drawVert_t* controlPoints,
                                    int patchWidth, int patchHeight,
                                    float subdivLevel,
                                    int mapOverBrightBits, int overBrightBits) {
	PatchGrid g;
	int width  = patchWidth;
	int height = patchHeight;

	float errorTable[2][METAL_MAX_GRID_SIZE];
	Com_Memset(errorTable, 0, sizeof(errorTable));

	for (int j = 0; j < height; j++)
		for (int i = 0; i < width; i++)
			g.ctrl[j][i] = ConvertDrawVert(controlPoints[j * patchWidth + i],
			                               mapOverBrightBits, overBrightBits);

	// Two passes: horizontal subdivision, then vertical (via transpose).
	for (int dir = 0; dir < 2; dir++) {
		for (int j2 = 0; j2 < METAL_MAX_GRID_SIZE; j2++) errorTable[dir][j2] = 0.f;

		int consecutiveComplete = 0;

		// Iterate over even-indexed columns of the current control grid.
		for (int j = 0; ; j = (j + 2) % (width - 1)) {
			// Find max perpendicular deviation of the Bezier midpoint from its chord.
			float maxLen = 0.f;
			for (int i = 0; i < height; i++) {
				float midxyz[3];
				for (int l = 0; l < 3; l++)
					midxyz[l] = (g.ctrl[i][j].xyz[l]
					             + g.ctrl[i][j+1].xyz[l] * 2.f
					             + g.ctrl[i][j+2].xyz[l]) * 0.25f;

				float d[3]     = { midxyz[0] - g.ctrl[i][j].xyz[0],
				                   midxyz[1] - g.ctrl[i][j].xyz[1],
				                   midxyz[2] - g.ctrl[i][j].xyz[2] };
				float chord[3] = { g.ctrl[i][j+2].xyz[0] - g.ctrl[i][j].xyz[0],
				                   g.ctrl[i][j+2].xyz[1] - g.ctrl[i][j].xyz[1],
				                   g.ctrl[i][j+2].xyz[2] - g.ctrl[i][j].xyz[2] };
				float clen = sqrtf(chord[0]*chord[0] + chord[1]*chord[1] + chord[2]*chord[2]);
				if (clen > 0.001f) { chord[0]/=clen; chord[1]/=clen; chord[2]/=clen; }
				float dot = d[0]*chord[0] + d[1]*chord[1] + d[2]*chord[2];
				float perp[3] = { d[0]-dot*chord[0], d[1]-dot*chord[1], d[2]-dot*chord[2] };
				float len2 = perp[0]*perp[0] + perp[1]*perp[1] + perp[2]*perp[2];
				if (len2 > maxLen) maxLen = len2;
			}
			maxLen = sqrtf(maxLen);

			if (maxLen < 0.1f) {
				errorTable[dir][j+1] = 999.f;
				if (++consecutiveComplete >= width) break;
				continue;
			}
			if (width + 2 > METAL_MAX_GRID_SIZE) {
				errorTable[dir][j+1] = 1.f / maxLen;
				break;
			}
			if (maxLen <= subdivLevel) {
				errorTable[dir][j+1] = 1.f / maxLen;
				if (++consecutiveComplete >= width) break;
				continue;
			}

			errorTable[dir][j+2] = 1.f / maxLen;
			consecutiveComplete = 0;

			// Insert two new columns replacing the approximation peak.
			width += 2;
			for (int i = 0; i < height; i++) {
				MetalPolyVertex prev = MidVert(g.ctrl[i][j],   g.ctrl[i][j+1]);
				MetalPolyVertex next = MidVert(g.ctrl[i][j+1], g.ctrl[i][j+2]);
				MetalPolyVertex mid  = MidVert(prev, next);
				for (int k = width - 1; k > j + 3; k--)
					g.ctrl[i][k] = g.ctrl[i][k-2];
				g.ctrl[i][j+1] = prev;
				g.ctrl[i][j+2] = mid;
				g.ctrl[i][j+3] = next;
			}
			j += 2;
		}

		// Transpose for the vertical pass (matching GL2: Transpose() then swap dims).
		GridTranspose(width, height, g.ctrl);
		int t = width; width = height; height = t;
	}

	GridPutPointsOnCurve(g.ctrl, width, height);

	// Cull colinear columns (matching GL2: no i-- after deletion).
	for (int i = 1; i < width - 1; i++) {
		if (errorTable[0][i] != 999.f) continue;
		for (int j = i + 1; j < width; j++) {
			for (int k = 0; k < height; k++) g.ctrl[k][j-1] = g.ctrl[k][j];
			errorTable[0][j-1] = errorTable[0][j];
		}
		width--;
	}

	// Cull colinear rows.
	for (int i = 1; i < height - 1; i++) {
		if (errorTable[1][i] != 999.f) continue;
		for (int j = i + 1; j < height; j++) {
			for (int k = 0; k < width; k++) g.ctrl[j-1][k] = g.ctrl[j][k];
			errorTable[1][j-1] = errorTable[1][j];
		}
		height--;
	}

	// Flip for longest tristrips (matching GL2).
	if (height > width) {
		GridTranspose(width, height, g.ctrl);
		GridInvertErrorTable(errorTable, width, height);
		int t = width; width = height; height = t;
		GridInvertCtrl(width, height, g.ctrl);
	}

	GridMakeNormals(g.ctrl, width, height);

	g.width  = width;
	g.height = height;
	Com_Memcpy(g.widthLodError,  errorTable[0], width  * sizeof(float));
	Com_Memcpy(g.heightLodError, errorTable[1], height * sizeof(float));
	return g;
}

// ---------------------------------------------------------------------------
// T-junction stitching — ports of GL2's R_GridInsertColumn / R_GridInsertRow
// ---------------------------------------------------------------------------

// Insert a new column at position `column` into grid g.
// The new column's vertices are interpolated from the neighbours; the vertex at
// row `row` is overridden with the exact xyz from `point`.
// Matches GL2's R_GridInsertColumn().
static void GridInsertColumn(PatchGrid& g, int column, int row,
                             const float point[3], float loderror) {
	if (g.width + 1 > METAL_MAX_GRID_SIZE) return;

	const int newWidth = g.width + 1;
	int oldwidth = 0;
	float newLodError[METAL_MAX_GRID_SIZE];
	MetalPolyVertex newCtrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE];

	for (int i = 0; i < newWidth; i++) {
		if (i == column) {
			// column >= 1 guaranteed by all callers (always l+1 where l >= 0)
			for (int j = 0; j < g.height; j++) {
				newCtrl[j][i] = LerpVertex(g.ctrl[j][oldwidth-1], g.ctrl[j][oldwidth], 0.5f);
				if (j == row) {
					newCtrl[j][i].xyz[0] = point[0];
					newCtrl[j][i].xyz[1] = point[1];
					newCtrl[j][i].xyz[2] = point[2];
				}
			}
			newLodError[i] = loderror;
		} else {
			for (int j = 0; j < g.height; j++)
				newCtrl[j][i] = g.ctrl[j][oldwidth];
			newLodError[i] = g.widthLodError[oldwidth];
			oldwidth++;
		}
	}

	for (int j = 0; j < g.height; j++)
		for (int i = 0; i < newWidth; i++)
			g.ctrl[j][i] = newCtrl[j][i];
	Com_Memcpy(g.widthLodError, newLodError, newWidth * sizeof(float));
	g.width = newWidth;
	GridMakeNormals(g.ctrl, g.width, g.height);
}

// Insert a new row at position `row` into grid g.
// Matches GL2's R_GridInsertRow().
static void GridInsertRow(PatchGrid& g, int row, int column,
                          const float point[3], float loderror) {
	if (g.height + 1 > METAL_MAX_GRID_SIZE) return;

	const int newHeight = g.height + 1;
	int oldheight = 0;
	float newLodError[METAL_MAX_GRID_SIZE];
	MetalPolyVertex newCtrl[METAL_MAX_GRID_SIZE][METAL_MAX_GRID_SIZE];

	for (int i = 0; i < newHeight; i++) {
		if (i == row) {
			// row >= 1 guaranteed by all callers
			for (int j = 0; j < g.width; j++) {
				newCtrl[i][j] = LerpVertex(g.ctrl[oldheight-1][j], g.ctrl[oldheight][j], 0.5f);
				if (j == column) {
					newCtrl[i][j].xyz[0] = point[0];
					newCtrl[i][j].xyz[1] = point[1];
					newCtrl[i][j].xyz[2] = point[2];
				}
			}
			newLodError[i] = loderror;
		} else {
			for (int j = 0; j < g.width; j++)
				newCtrl[i][j] = g.ctrl[oldheight][j];
			newLodError[i] = g.heightLodError[oldheight];
			oldheight++;
		}
	}

	for (int i = 0; i < newHeight; i++)
		for (int j = 0; j < g.width; j++)
			g.ctrl[i][j] = newCtrl[i][j];
	Com_Memcpy(g.heightLodError, newLodError, newHeight * sizeof(float));
	g.height = newHeight;
	GridMakeNormals(g.ctrl, g.width, g.height);
}

// Stitch one T-junction crack between g1 and g2.
// Returns true if a column/row was inserted into g2 (caller should retry).
// Direct port of GL2's R_StitchPatches() — all four directional scan passes.
static bool StitchPatchGrids(PatchGrid& g1, PatchGrid& g2) {
	// Tolerances matching GL2's R_StitchPatches
	auto near01 = [](const float* a, const float* b) {
		return fabsf(a[0]-b[0]) <= 0.1f && fabsf(a[1]-b[1]) <= 0.1f && fabsf(a[2]-b[2]) <= 0.1f;
	};
	auto near001 = [](const float* a, const float* b) {
		return fabsf(a[0]-b[0]) < 0.01f && fabsf(a[1]-b[1]) < 0.01f && fabsf(a[2]-b[2]) < 0.01f;
	};

	// Pass 1: g1 top/bottom width-edges, forward k (g1 has more subdivisions than g2)
	for (int n = 0; n < 2; n++) {
		const int row1 = (n == 1) ? (g1.height - 1) : 0;
		for (int k = 0; k < g1.width - 2; k += 2) {
			for (int m = 0; m < 2; m++) {
				if (g2.width >= METAL_MAX_GRID_SIZE) break;
				const int row2 = (m == 1) ? (g2.height - 1) : 0;
				for (int l = 0; l < g2.width - 1; l++) {
					if (!near01(g1.ctrl[row1][k].xyz,   g2.ctrl[row2][l].xyz))   continue;
					if (!near01(g1.ctrl[row1][k+2].xyz, g2.ctrl[row2][l+1].xyz)) continue;
					if (near001(g2.ctrl[row2][l].xyz,   g2.ctrl[row2][l+1].xyz)) continue;
					GridInsertColumn(g2, l+1, row2,
					                 g1.ctrl[row1][k+1].xyz,
					                 g1.widthLodError[std::min(k+1, g1.width-1)]);
					return true;
				}
			}
			for (int m = 0; m < 2; m++) {
				if (g2.height >= METAL_MAX_GRID_SIZE) break;
				const int col2 = (m == 1) ? (g2.width - 1) : 0;
				for (int l = 0; l < g2.height - 1; l++) {
					if (!near01(g1.ctrl[row1][k].xyz,   g2.ctrl[l][col2].xyz))   continue;
					if (!near01(g1.ctrl[row1][k+2].xyz, g2.ctrl[l+1][col2].xyz)) continue;
					if (near001(g2.ctrl[l][col2].xyz,   g2.ctrl[l+1][col2].xyz)) continue;
					GridInsertRow(g2, l+1, col2,
					              g1.ctrl[row1][k+1].xyz,
					              g1.widthLodError[std::min(k+1, g1.width-1)]);
					return true;
				}
			}
		}
	}

	// Pass 2: g1 left/right height-edges, forward k
	for (int n = 0; n < 2; n++) {
		const int col1 = (n == 1) ? (g1.width - 1) : 0;
		for (int k = 0; k < g1.height - 2; k += 2) {
			for (int m = 0; m < 2; m++) {
				if (g2.width >= METAL_MAX_GRID_SIZE) break;
				const int row2 = (m == 1) ? (g2.height - 1) : 0;
				for (int l = 0; l < g2.width - 1; l++) {
					if (!near01(g1.ctrl[k][col1].xyz,   g2.ctrl[row2][l].xyz))   continue;
					if (!near01(g1.ctrl[k+2][col1].xyz, g2.ctrl[row2][l+1].xyz)) continue;
					if (near001(g2.ctrl[row2][l].xyz,   g2.ctrl[row2][l+1].xyz)) continue;
					GridInsertColumn(g2, l+1, row2,
					                 g1.ctrl[k+1][col1].xyz,
					                 g1.heightLodError[std::min(k+1, g1.height-1)]);
					return true;
				}
			}
			for (int m = 0; m < 2; m++) {
				if (g2.height >= METAL_MAX_GRID_SIZE) break;
				const int col2 = (m == 1) ? (g2.width - 1) : 0;
				for (int l = 0; l < g2.height - 1; l++) {
					if (!near01(g1.ctrl[k][col1].xyz,   g2.ctrl[l][col2].xyz))   continue;
					if (!near01(g1.ctrl[k+2][col1].xyz, g2.ctrl[l+1][col2].xyz)) continue;
					if (near001(g2.ctrl[l][col2].xyz,   g2.ctrl[l+1][col2].xyz)) continue;
					GridInsertRow(g2, l+1, col2,
					              g1.ctrl[k+1][col1].xyz,
					              g1.heightLodError[std::min(k+1, g1.height-1)]);
					return true;
				}
			}
		}
	}

	// Pass 3: g1 top/bottom width-edges, backward k (g1 has fewer subdivisions than g2)
	for (int n = 0; n < 2; n++) {
		const int row1 = (n == 1) ? (g1.height - 1) : 0;
		for (int k = g1.width - 1; k > 1; k -= 2) {
			for (int m = 0; m < 2; m++) {
				if (g2.width >= METAL_MAX_GRID_SIZE) break;
				const int row2 = (m == 1) ? (g2.height - 1) : 0;
				for (int l = 0; l < g2.width - 1; l++) {
					if (!near01(g1.ctrl[row1][k].xyz,   g2.ctrl[row2][l].xyz))   continue;
					if (!near01(g1.ctrl[row1][k-2].xyz, g2.ctrl[row2][l+1].xyz)) continue;
					if (near001(g2.ctrl[row2][l].xyz,   g2.ctrl[row2][l+1].xyz)) continue;
					GridInsertColumn(g2, l+1, row2,
					                 g1.ctrl[row1][k-1].xyz,
					                 g1.widthLodError[std::min(k+1, g1.width-1)]);
					return true;
				}
			}
			for (int m = 0; m < 2; m++) {
				if (g2.height >= METAL_MAX_GRID_SIZE) break;
				const int col2 = (m == 1) ? (g2.width - 1) : 0;
				for (int l = 0; l < g2.height - 1; l++) {
					if (!near01(g1.ctrl[row1][k].xyz,   g2.ctrl[l][col2].xyz))   continue;
					if (!near01(g1.ctrl[row1][k-2].xyz, g2.ctrl[l+1][col2].xyz)) continue;
					if (near001(g2.ctrl[l][col2].xyz,   g2.ctrl[l+1][col2].xyz)) continue;
					GridInsertRow(g2, l+1, col2,
					              g1.ctrl[row1][k-1].xyz,
					              g1.widthLodError[std::min(k+1, g1.width-1)]);
					return true;
				}
			}
		}
	}

	// Pass 4: g1 left/right height-edges, backward k
	for (int n = 0; n < 2; n++) {
		const int col1 = (n == 1) ? (g1.width - 1) : 0;
		for (int k = g1.height - 1; k > 1; k -= 2) {
			for (int m = 0; m < 2; m++) {
				if (g2.width >= METAL_MAX_GRID_SIZE) break;
				const int row2 = (m == 1) ? (g2.height - 1) : 0;
				for (int l = 0; l < g2.width - 1; l++) {
					if (!near01(g1.ctrl[k][col1].xyz,   g2.ctrl[row2][l].xyz))   continue;
					if (!near01(g1.ctrl[k-2][col1].xyz, g2.ctrl[row2][l+1].xyz)) continue;
					if (near001(g2.ctrl[row2][l].xyz,   g2.ctrl[row2][l+1].xyz)) continue;
					GridInsertColumn(g2, l+1, row2,
					                 g1.ctrl[k-1][col1].xyz,
					                 g1.heightLodError[std::min(k+1, g1.height-1)]);
					return true;
				}
			}
			for (int m = 0; m < 2; m++) {
				if (g2.height >= METAL_MAX_GRID_SIZE) break;
				const int col2 = (m == 1) ? (g2.width - 1) : 0;
				for (int l = 0; l < g2.height - 1; l++) {
					if (!near01(g1.ctrl[k][col1].xyz,   g2.ctrl[l][col2].xyz))   continue;
					if (!near01(g1.ctrl[k-2][col1].xyz, g2.ctrl[l+1][col2].xyz)) continue;
					if (near001(g2.ctrl[l][col2].xyz,   g2.ctrl[l+1][col2].xyz)) continue;
					GridInsertRow(g2, l+1, col2,
					              g1.ctrl[k-1][col1].xyz,
					              g1.heightLodError[std::min(k+1, g1.height-1)]);
					return true;
				}
			}
		}
	}

	return false;
}

// ---------------------------------------------------------------------------
// Grid → triangle list conversion
// Triangle ordering matches GL2's MakeMeshIndexes() for tristrip efficiency.
// ---------------------------------------------------------------------------
static void PatchGridToTriangles(const PatchGrid& g, std::vector<MetalPolyVertex>& out) {
	for (int j = 0; j < g.height - 1; j++) {
		for (int i = 0; i < g.width - 1; i++) {
			// v2=ctrl[j][i], v1=ctrl[j][i+1], v3=ctrl[j+1][i], v4=ctrl[j+1][i+1]
			const MetalPolyVertex& v2 = g.ctrl[j][i];
			const MetalPolyVertex& v1 = g.ctrl[j][i+1];
			const MetalPolyVertex& v3 = g.ctrl[j+1][i];
			const MetalPolyVertex& v4 = g.ctrl[j+1][i+1];
			out.push_back(v2); out.push_back(v3); out.push_back(v1);
			out.push_back(v1); out.push_back(v3); out.push_back(v4);
		}
	}
}

// ---------------------------------------------------------------------------
// TessellateBezierPatch — adaptive, used directly by the mark-fragments path.
// subdivLevel: world-space flatness tolerance clamped to 1..64 (GL2 range).
// ---------------------------------------------------------------------------
inline void TessellateBezierPatch(const drawVert_t* controlPoints, int width, int height,
                                  std::vector<MetalPolyVertex>& outVerts, float subdivLevel,
                                  int mapOverBrightBits, int overBrightBits) {
	PatchGrid g = AdaptiveTessellate(controlPoints, width, height, subdivLevel,
	                                 mapOverBrightBits, overBrightBits);
	PatchGridToTriangles(g, outVerts);
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
// entity parameter is optional, only needed for EntityTranslate tcMod
inline void ComputeTCModMatrix(const MetalTCMod& mod, float time, float outMatrix[6], float& turbAmp, float& turbPhase, const refEntity_t* entity = nullptr) {
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
			// Entity-based translation using entity's shaderTexCoord
			if (entity) {
				outMatrix[4] = entity->shaderTexCoord[0];
				outMatrix[5] = entity->shaderTexCoord[1];
			}
			// If no entity, remains identity
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

// Portal/Mirror orientation structure - used for GL2-style portal transformations
// Surface orientation: origin is on the mirror plane, axis defines the coordinate system
// Camera orientation: origin is where the reflected camera goes, axis is the reflected view
struct PortalOrientation {
	float origin[3] = {};
	float axis[3][3] = {};  // axis[0]=forward/normal, axis[1]=right, axis[2]=up
};

} // end anonymous namespace for deformVertexes helpers

//=============================================================================
// BSP surface types used by MarkFragments (file-scope so helpers can access them)
//=============================================================================
namespace {

struct BspMarkVert {
	float xyz[3];
	float normal[3];
};

enum class BspMarkSurfType : int { Skip = 0, Face, Patch, TriSoup };

struct BspMarkSurface {
	BspMarkSurfType type          = BspMarkSurfType::Skip;
	int             shaderSurfFlags = 0;
	int             shaderContFlags = 0;
	// Cull plane for MST_PLANAR surfaces
	float           cullNormal[3] = {};
	float           cullDist      = 0.0f;
	byte            cullSignbits  = 0;
	byte            cullType      = 0;
	byte            pad[2]        = {};
	// Flat triangle list: 3 consecutive BspMarkVerts per triangle (no separate index array)
	std::vector<BspMarkVert> verts;
};

} // anonymous namespace (BSP mark types)

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
	void drawRotatePic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float degrees, qhandle_t shader);
	void drawRotatePic2(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float degrees, qhandle_t shader);
	
	int lerpTag(orientation_t* tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char* tagName);
	void modelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);

	const glconfig_t& config() const { return config_; }
	bool callLoggingEnabled() const;
	void logRendererCall(const char* name) const;

	friend void Metal_GfxInfo_f();
	friend void Metal_ImageList_f();
	friend void Metal_DebugPoly_f();
	friend void MetalBackend_LoadWorld(const char* name);
	friend qboolean MetalBackend_GetEntityToken(char* buffer, int size);
	friend qboolean MetalBackend_inPVS(const vec3_t p1, const vec3_t p2);
	friend int MetalBackend_MarkFragments(int, const vec3_t*, const vec3_t, int, vec3_t, int, markFragment_t*);
	friend qhandle_t MetalBackend_GetFlareShader();
	friend void MetalBackend_DrawFlareQuad(float, float, float, float, float, float, float, qhandle_t);
	friend void MetalBackend_GetSceneCameraForFlares(float[16], float[16], float[3], int*, int*, int*, int*);

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
		int cubemapIndex = 0; // 0 = no probe, 1+ = probe index (1-based into worldCubemapTextures_)
		bool isPortal = false;      // True if this surface is a portal/mirror
		bool isStaticWorld = false; // True if vertices live in staticWorldVertexBuffer_
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
		float greyscale = 0.0f;           // r_greyscale: 0.0 = colour, 1.0 = full greyscale
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
	// Note: Metal's float3 is 16-byte aligned, so we use float4 for proper alignment
	struct alignas(16) EntityLightingParams {
		float ambientLight[4] = {150.0f, 150.0f, 150.0f, 0.0f};  // xyz = color, w = padding
		float directedLight[4] = {150.0f, 150.0f, 150.0f, 0.0f}; // xyz = color, w = padding
		float lightDir[4] = {0.0f, 0.0f, 1.0f, 0.0f};            // xyz = dir, w = padding
		float modelLightDir[4] = {0.0f, 0.0f, 1.0f, 1.0f};       // xyz = dir, w = overBrightBits
	};

	// Normal-map and specular-map parameters — matches scene.metal NormalSpecularParams
	struct alignas(16) NormalSpecularParams {
		float useNormalMap    = 0.0f;
		float useSpecularMap  = 0.0f;
		float normalScale     = 1.0f;
		float specularPower   = 32.0f;
		// Cubemap reflection (NEW) — must match scene.metal NormalSpecularParams layout
		float useCubemap      = 0.0f;  // 1.0 = sample cubemap, 0.0 = disabled
		float cubemapStrength = 0.0f;  // Reflection blend strength (0–1)
		float _pad0           = 0.0f;
		float _pad1           = 0.0f;
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
			qhandle_t normalMapHandle   = 0;  // Detected normal map for this stage (0 = none)
			qhandle_t specularMapHandle = 0;  // Detected specular map for this stage (0 = none)
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
	cvar_t* r_greyscale_ = nullptr;
	cvar_t* r_shadows_ = nullptr;  // 0=off, 1+=blob/projection shadows
	cvar_t* r_subdivisions_ = nullptr;  // world-space flatness tolerance for patch subdivision
	cvar_t* r_pshadowDist_ = nullptr;   // max distance for per-object shadow consideration

	// Projection (blob) shadow pipeline resources
	MetalPtr<MTL::RenderPipelineState> shadowPipeline_;
	MetalPtr<MTL::DepthStencilState>   shadowDepthState_;
	MetalPtr<MTL::VertexDescriptor>    shadowVertexDescriptor_;
	MetalPtr<MTL::Function>            shadowVertexFunction_;
	MetalPtr<MTL::Function>            shadowFragmentFunction_;
	MetalPtr<MTL::Buffer> sceneUniformBuffer_;
	size_t sceneUniformBufferSize_ = 0;
	MetalPtr<MTL::Buffer> polyVertexBuffer_;
	size_t polyVertexBufferSize_ = 0;
	size_t polyVertexCountGPU_ = 0;
	// Static world vertex buffer — uploaded once at map load, never modified per-frame.
	// Stores worldVertexTemplate_ on the GPU for zero-copy world rendering.
	MetalPtr<MTL::Buffer> staticWorldVertexBuffer_;
	size_t staticWorldVertexBufferSize_ = 0;  // Total bytes in staticWorldVertexBuffer_
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


	MetalPtr<MTL::SamplerState> sceneSampler_;       // Repeat mode
	MetalPtr<MTL::SamplerState> sceneClampSampler_;  // Clamp-to-edge mode for clampmap
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
	MetalPtr<MTL::DepthStencilState> dlightDepthState_;  // Depth-test-only (no write) for dlight pass

	// Animated-model dlight pipeline (uses model vertex descriptor with position2/normal2)
	MetalPtr<MTL::Function> dlightAnimatedVertexFunction_;
	MetalPtr<MTL::RenderPipelineState> dlightAnimatedPipeline_;

	// Per-object shadow maps (pshadow) — per-frame state
	pshadow_t pshadows_[MAX_DRAWN_PSHADOWS];
	int       numPshadows_ = 0;

	// PShadow GPU resources
	MetalPtr<MTL::Texture>             pshadowMaps_[MAX_DRAWN_PSHADOWS];
	MetalPtr<MTL::Function>            pshadowCasterVFn_;
	MetalPtr<MTL::Function>            pshadowRecvWorldVFn_;
	MetalPtr<MTL::Function>            pshadowRecvModelVFn_;
	MetalPtr<MTL::Function>            pshadowRecvFFn_;
	MetalPtr<MTL::RenderPipelineState> pshadowCasterPipeline_;
	MetalPtr<MTL::RenderPipelineState> pshadowRecvWorldPipeline_;
	MetalPtr<MTL::RenderPipelineState> pshadowRecvModelPipeline_;
	MetalPtr<MTL::VertexDescriptor>    pshadowCasterVD_;
	MetalPtr<MTL::DepthStencilState>   pshadowCasterDepthState_;
	MetalPtr<MTL::DepthStencilState>   pshadowRecvDepthState_;

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

	// ---------- MarkFragments BSP data ----------
	std::vector<int>            bspLeafSurfaces_;   // LUMP_LEAFSURFACES
	std::vector<cplane_t>       bspCPlanes_;         // BSP planes with type/signbits
	std::vector<BspMarkSurface> bspMarkSurfData_;    // per BSP surface geometry
	std::vector<int>            bspSurfViewCounts_;  // per surface viewCount for dedup
	int                         bspViewCount_ = 0;
	cvar_t*                     r_marksOnTriangleMeshes_ = nullptr;
	MetalPtr<MTL::Function> fogVertexFunction_;
	MetalPtr<MTL::Function> fogFragmentFunction_;
	MetalPtr<MTL::DepthStencilState> fogDepthState_;

	// Portal/Mirror rendering - following GL2's approach exactly
	struct PortalSurface {
		int packetIndex = -1;       // Index into polyPackets_
		float plane[4] = {};        // Plane equation (normal.xyz, dist)
		float center[3] = {};       // Center of portal surface
		bool isMirror = true;       // True if mirror (no camera entity), false if portal
		PortalOrientation surface;  // Surface coordinate system (on the mirror)
		PortalOrientation camera;   // Camera coordinate system (reflected)
	};
	std::vector<PortalSurface> portalSurfaces_;  // Detected portal surfaces this frame
	MetalPtr<MTL::Texture> portalTexture_;       // Render target for portal view
	MetalPtr<MTL::Texture> portalDepthTexture_;  // Depth buffer for portal rendering
	int portalTextureWidth_ = 0;
	int portalTextureHeight_ = 0;
	bool isRenderingPortal_ = false;             // True if currently rendering portal view

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

	// Entity lump data for GetEntityToken
	std::vector<char> entityStringBuf_;   // null-terminated entity string from BSP
	char* entityParsePoint_ = nullptr;    // cursor into entityStringBuf_

	// Sky portal: if the BSP entity lump contains a "skyboxportal" or "misc_skyportal"
	// entity (as used in some custom Q3 maps), the sky is rendered from its origin rather
	// than the player's eye position, producing the portal-sky visual effect.
	bool    hasSkyPortal_ = false;
	vec3_t  skyPortalOrigin_ = {0.0f, 0.0f, 0.0f};

	// BSP spatial data for inPVS / R_PointInLeaf
	std::vector<dnode_t>  bspNodes_;
	std::vector<dleaf_t>  bspLeafs_;
	std::vector<dplane_t> bspPlanesForPVS_;  // copy of plane lump for node traversal

	std::vector<MetalPolyVertex> worldVertexTemplate_;
	std::vector<ScenePolyPacket> worldPacketTemplate_;
	std::vector<qhandle_t> worldLightmapHandles_;
	std::vector<int> worldSurfaceToPacket_;  // Maps BSP surface index to packet index (-1 if skipped)
	std::vector<MetalBrushModel> brushModels_;  // Brush models (inline BSP models)
	int numWorldSurfaces_ = 0;  // Number of surfaces in model 0 (the world itself)
	int numWorldPackets_ = 0;   // Number of packets that belong to the world (vs brush models)

	// Cubemap probe textures (TextureTypeCube) for environment reflections.
	// Index 0 = fallback (1×1 black), indices 1..N = probe textures (1-based,
	// matching the convention returned by R_CubemapForPoint).
	std::vector<MetalPtr<MTL::Texture>> worldCubemapTextures_;
	// Permanent 1×1 black cubemap — bound to texture slot 3 when no world
	// cubemaps are loaded so the shader slot is always valid.
	MetalPtr<MTL::Texture> fallbackCubemapTexture_;

	// PVS visibility culling
	std::vector<byte> visData_;       // Raw PVS bit arrays from LUMP_VISIBILITY (after 8-byte header)
	int visNumClusters_ = 0;          // Number of clusters in the PVS
	int visClusterBytes_ = 0;         // Bytes per cluster row in the PVS bit array
	std::vector<int> surfaceVisFrame_; // Per-packet visibility frame (indexed by packet index)
	int visFrame_ = 0;                // Incremented each frame; a packet is visible iff surfaceVisFrame_[i]==visFrame_
	std::vector<MetalModel*> models_;  // Model storage (index 0 is reserved for BAD model)
	std::unordered_map<std::string, qhandle_t> modelLookup_;
	std::unordered_map<uintptr_t, MetalPtr<MTL::Buffer>> mdrIndexBuffers_; // Cached MDR triangle index buffers
	std::unordered_map<uintptr_t, MetalPtr<MTL::Buffer>> iqmIndexBuffers_; // Cached IQM triangle index buffers
	std::vector<MetalSkin> registeredSkins_;  // Skin storage (index 0 is reserved)
	std::unordered_map<std::string, qhandle_t> skinLookup_;
	std::vector<MetalShaderResource> shaderResources_;
	std::unordered_map<std::string, qhandle_t> shaderLookup_;

	// Lens flare rendering (see tr_flares.cpp)
	MetalPtr<MTL::RenderPipelineState> pipelineFlare_;  // additive One/One blend
	qhandle_t flareShaderHandle_ = 0;
	cvar_t* r_flares_   = nullptr;
	cvar_t* r_flareSize_ = nullptr;
	cvar_t* r_flareFade_ = nullptr;
	cvar_t* r_flareCoeff_ = nullptr;

	// -------------------------------------------------------------------------
	// Post-processing pipeline
	// -------------------------------------------------------------------------

	// HDR render target — scene renders here instead of directly to drawable.
	MetalPtr<MTL::Texture> hdrColorTarget_;
	int hdrTargetWidth_  = 0;
	int hdrTargetHeight_ = 0;

	// Half-resolution ping-pong targets for bloom and sun-ray intermediates.
	MetalPtr<MTL::Texture> bloomTarget_[2];
	int bloomTargetWidth_  = 0;
	int bloomTargetHeight_ = 0;

	// SSAO occlusion map (full-res, r=AO).
	MetalPtr<MTL::Texture> ssaoTarget_;
	int ssaoTargetWidth_  = 0;
	int ssaoTargetHeight_ = 0;

	// Luminance targets — one raw (current frame) and two for ping-pong
	// temporal accumulation (matches GL2's calcLevelsFbo + targetLevelsFbo).
	MetalPtr<MTL::Texture> lumRawTarget_;        // 1×1 log-lum this frame
	MetalPtr<MTL::Texture> lumAccumTarget_[2];   // ping-pong smoothed lum
	int lumAccumIdx_ = 0;                        // which accum is the "current"
	bool lumAccumInitialized_ = false;

	// Intermediate scratch textures for the luminance downsample chain.
	// Created at up to 256×256; reused across frames.
	MetalPtr<MTL::Texture> lumScratch_[2];
	int lumScratchWidth_  = 0;
	int lumScratchHeight_ = 0;

	// Post-processing render pipeline states.
	MetalPtr<MTL::RenderPipelineState> ppPassthroughPso_;       // simple blit
	MetalPtr<MTL::RenderPipelineState> ppDownsample4xPso_;      // 4× box downsample
	MetalPtr<MTL::RenderPipelineState> ppCalcLevelsFirstPso_;   // lum first pass
	MetalPtr<MTL::RenderPipelineState> ppCalcLevelsPso_;        // lum subsequent
	MetalPtr<MTL::RenderPipelineState> ppLumBlendPso_;          // temporal lum blend
	MetalPtr<MTL::RenderPipelineState> ppTonemapPso_;           // HDR→LDR tonemap+SSAO
	MetalPtr<MTL::RenderPipelineState> ppBloomExtractPso_;      // bright extraction
	MetalPtr<MTL::RenderPipelineState> ppGaussianBlurHPso_;     // Gaussian H
	MetalPtr<MTL::RenderPipelineState> ppGaussianBlurVPso_;     // Gaussian V
	MetalPtr<MTL::RenderPipelineState> ppSSAOPso_;              // SSAO occlusion
	MetalPtr<MTL::RenderPipelineState> ppDepthBlurPso_;         // SSAO depth-aware blur
	MetalPtr<MTL::RenderPipelineState> ppSunRaysPso_;           // sun-ray radial blur
	MetalPtr<MTL::RenderPipelineState> ppDofBlurPso_;           // DOF bokeh
	// Additive blend pipeline (for bloom composite and sun-ray composite).
	MetalPtr<MTL::RenderPipelineState> ppAdditivePso_;

	// Shared linear-clamp sampler for all post-processing passes.
	MetalPtr<MTL::SamplerState> ppLinearSampler_;

	// Post-processing cvars.
	cvar_t* r_hdr_               = nullptr;   // 0 = off, 1 = HDR path
	cvar_t* r_bloom_             = nullptr;   // 0 = off, 1 = bloom enabled
	cvar_t* r_bloomThreshold_    = nullptr;   // luminance threshold (default 1.0)
	cvar_t* r_bloomStrength_     = nullptr;   // bloom output multiplier
	cvar_t* r_ssao_              = nullptr;   // 0 = off, 1 = SSAO
	cvar_t* r_dof_               = nullptr;   // 0 = off, blurFactor from refdef
	cvar_t* r_sunlightMode_      = nullptr;   // 0 = off, 1 = sun rays
	cvar_t* r_autoExposure_      = nullptr;   // 0 = manual, 1 = auto
	cvar_t* r_autoExposureMinValue_ = nullptr;
	cvar_t* r_autoExposureMaxValue_ = nullptr;
	cvar_t* r_cameraExposure_    = nullptr;   // manual exposure bias
	cvar_t* r_tonemapExposure_   = nullptr;   // target average scene luminance

	// Per-frame post-processing state.
	bool usingHDRRenderPath_  = false;   // beginFrame set this when r_hdr=1
	bool postProcessingDone_  = false;   // set after runPostProcessing()

	bool initializeWindow(int& width, int& height, qboolean& fullscreen);
	bool createPipeline(MTL::Texture* drawableTexture);
	bool create2DPipeline();
	bool createFlarePipeline();
	void drawFlareQuad(float x, float y, float w, float h,
	                   float r, float g, float b, qhandle_t shader);
	void getSceneCameraForFlares(float viewMat[16], float projMat[16], float viewOrg[3],
	                             int* vpX, int* vpY, int* vpW, int* vpH) const;
	qhandle_t getFlareShader();
	bool ensureDepthTexture(int width, int height);
	bool ensureHDRTargets(int width, int height);
	bool createPostProcessPipelines();
	void ensurePostProcessed();
	void runPostProcessing();
	void doRenderPass(MTL::RenderPipelineState* pso,
	                  MTL::Texture* src0, MTL::Texture* src1, MTL::Texture* src2,
	                  MTL::Texture* dst,
	                  MTL::LoadAction loadAction,
	                  const void* uniforms, size_t uniformSize);
	MTL::RenderPipelineState* makePPPso(MTL::Library* lib,
	                                    const char* vfnName,
	                                    const char* ffnName,
	                                    MTL::PixelFormat colorFmt,
	                                    bool additiveBlend = false);
	bool ensureDlightResources();
	void createDlightTexture();
	void processPendingUploads();
	void fillConfigDefaults(int width, int height, qboolean fullscreen);
	void publishConfig();
	void registerConsoleCommands();
	void drawRotatePicImpl(float x, float y, float w, float h, float s1, float t1, float s2, float t2,
	                       float degrees, qhandle_t shader, float pivotNdcX, float pivotNdcY);
	void unregisterConsoleCommands();
	void configureDebugPoly(bool enable, const char* shaderName, float size, float distance);
	void appendDebugPoly(const MetalSceneState& scene);
	qhandle_t ensureDebugPolyShaderHandle();
	bool loadWorldMap(const char* name);
	void unloadWorldMap();
	int  markFragments(int numPoints, const vec3_t* points, const vec3_t projection,
	                   int maxPoints, vec3_t pointBuffer, int maxFragments,
	                   markFragment_t* fragmentBuffer);
	qboolean getEntityToken(char* buffer, int size);
	void parseSkyPortalEntity();  // Scan entity lump for a sky portal origin
	qboolean inPVS(const vec3_t p1, const vec3_t p2) const;
	const dleaf_t* pointInLeaf(const vec3_t p) const;
	const byte* getClusterPVS(int cluster) const;
	bool cullWorldBox(const int mins[3], const int maxs[3], uint32_t& planeBits) const;
	void recursiveWorldNode(int nodeIndex, uint32_t planeBits, const byte* pvs);
	void markWorldSurfaces();
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
	bool ensureShadowPipeline();
	// PShadow (per-object shadow maps)
	void computePshadows();
	bool ensurePshadowResources();
	void ensurePshadowTexture(int i);
	void renderPshadowCasterPass(int shadowIdx);
	void renderPshadowReceiverPasses();
	enum class CullResult { Out, In, Clip };
	void renderModel(const refEntity_t& ent, MetalModel& model, MetalModelLOD& lodData,
	                int fogIndex, CullResult cullState);
	void renderBrushModel(const refEntity_t& ent, MetalBrushModel& bmodel);
	// Procedural entity rendering (sprites, beams, rails, lightning)
	void renderSprite(const refEntity_t& ent);
	void renderBeam(const refEntity_t& ent);
	void renderRailCore(const refEntity_t& ent);
	void renderRailRings(const refEntity_t& ent);
	void renderLightning(const refEntity_t& ent);
	void renderRailRibbonSegment(size_t baseVertex, int vertexCount, qhandle_t shaderHandle);
	void addQuadToPolyBuffer(const vec3_t origin, const vec3_t left, const vec3_t up, 
	                         float s1, float t1, float s2, float t2, const byte* color,
	                         qhandle_t shader, int& firstVertex, int& vertexCount);
	void renderModelSurface(const refEntity_t& ent, MetalModelSurface& surface,
	                        const float* mvpMatrix, const float* modelMatrix, float vertexLerp,
	                        const ModelFogParams& fogParams,
	                        const vec3_t ambientLight, const vec3_t directedLight, const vec3_t lightDir,
	                        uint32_t dlightBits);
	void renderModelShadow(const refEntity_t& ent, MetalModelSurface& surface,
	                       const float* mvpMatrix, const vec3_t modelLightDir);
	MTL::Buffer* createModelVertexBuffer(const refEntity_t& ent, MetalModelSurface& surface);
	// MDR skeletal animation rendering
	void renderMDRModel(const refEntity_t& ent, MetalModel& model);
	void renderMDRSurface(const refEntity_t& ent, const mdrHeader_t* header,
	                      const mdrSurface_t* surf, const mdrBone_t* bonePtr,
	                      const float* mvpMatrix, const float* modelMatrix,
	                      const ModelFogParams& fogParams,
	                      const vec3_t ambientLight, const vec3_t directedLight,
	                      const vec3_t lightDir);
	void registerMDRShaders(MetalModel* model);
	// IQM skeletal animation rendering
	void renderIQMModel(const refEntity_t& ent, MetalModel& model);
	void renderIQMSurface(const refEntity_t& ent, MetalIQMData* data,
	                      const MetalIQMSurface* surf,
	                      int frame, int oldframe, float backlerp,
	                      const float* mvpMatrix, const float* modelMatrix,
	                      const ModelFogParams& fogParams,
	                      const vec3_t ambientLight, const vec3_t directedLight,
	                      const vec3_t lightDir);
	void registerIQMShaders(MetalModel* model);
	void calculateEntityTransform(const refEntity_t& ent, float* matrix);
	void setupEntityLighting(const refEntity_t& ent, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
	void multiplyMatrices4x4(const float* a, const float* b, const float* c, float* result);
	StagePipelineEntry* getStagePipeline(const MetalShaderResource::MetalPipelineKey& key);
	ModelStagePipelineEntry* getModelStagePipeline(const MetalShaderResource::MetalPipelineKey& key);
	void resetStagePipelineCache();
	bool drawPolyPackets();
	bool drawPolyPacketsForPortal(int excludePacketIndex);  // Draw world, skip portal surface
	bool drawModelEntities();
	bool drawDynamicLights();
	bool drawFogPasses();
	bool ensureDlightAnimatedResources();
	uint32_t computeModelDlightBits(const refEntity_t& ent, const MetalModelLOD& lodData) const;
	// Portal/mirror rendering
	void detectPortalSurfaces();
	bool ensurePortalTexture(int width, int height);
	void calculateMirrorMatrix(const PortalSurface& portal, float* viewMatrix, float* projMatrix);
	void calculatePortalMatrix(const PortalSurface& portal, const refEntity_t& cameraEntity,
	                           float* viewMatrix, float* projMatrix);
	bool renderPortalView(const PortalSurface& portal);
	bool renderPortalViews();
	void resumeMainEncoder(MTL::RenderCommandEncoder* oldEncoder);
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
	TCModParams computeTCModParams(const MetalShaderStageInfo* stageInfo, float timeSeconds, const refEntity_t* entity = nullptr) const;

	// -------------------------------------------------------------------------
	// Cubemap helpers
	// -------------------------------------------------------------------------
	// Create a Metal TextureTypeCube texture with all 6 faces set to a solid colour.
	MTL::Texture* createSolidColorCubemap(float r, float g, float b);
	// Attempt to load a pre-baked DDS cubemap file.  Returns nullptr on failure.
	MTL::Texture* loadCubemapDDS(const char* filename);
	// Load/generate cubemap textures for all probes of the current world and
	// assign cubemapIndex to every packet in worldPacketTemplate_.
	void loadWorldCubemaps(const std::string& requestedName);
	// Return the permanent fallback cubemap, creating it on first use.
	MTL::Texture* getOrCreateFallbackCubemap();
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
	dlightDepthState_.reset();
	dlightAnimatedVertexFunction_.reset();
	dlightAnimatedPipeline_.reset();
	depthTexture_.reset();
	depthTextureWidth_ = depthTextureHeight_ = 0;
	stagePipelineCache_.clear();

	// Post-processing resources
	hdrColorTarget_.reset();
	hdrTargetWidth_ = hdrTargetHeight_ = 0;
	for (int i = 0; i < 2; ++i) {
		bloomTarget_[i].reset();
		lumAccumTarget_[i].reset();
		lumScratch_[i].reset();
	}
	ssaoTarget_.reset();
	ssaoTargetWidth_ = ssaoTargetHeight_ = 0;
	bloomTargetWidth_ = bloomTargetHeight_ = 0;
	lumScratchWidth_ = lumScratchHeight_ = 0;
	lumRawTarget_.reset();
	lumAccumInitialized_ = false;
	ppPassthroughPso_.reset();
	ppDownsample4xPso_.reset();
	ppCalcLevelsFirstPso_.reset();
	ppCalcLevelsPso_.reset();
	ppLumBlendPso_.reset();
	ppTonemapPso_.reset();
	ppBloomExtractPso_.reset();
	ppGaussianBlurHPso_.reset();
	ppGaussianBlurVPso_.reset();
	ppSSAOPso_.reset();
	ppDepthBlurPso_.reset();
	ppSunRaysPso_.reset();
	ppDofBlurPso_.reset();
	ppAdditivePso_.reset();
	ppLinearSampler_.reset();
	
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
	if (!worldLoaded_ || worldPacketTemplate_.empty() || !staticWorldVertexBuffer_) {
		return;
	}

	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;

	// Only append world packets (model 0), not brush model packets.
	// Brush model surfaces are rendered via renderBrushModel with entity transforms.
	const int packetLimit = (numWorldPackets_ > 0) ? numWorldPackets_ : static_cast<int>(worldPacketTemplate_.size());

	for (int i = 0; i < packetLimit; ++i) {
		// PVS + frustum culling: skip surfaces not marked visible this frame.
		// surfaceVisFrame_[i] == visFrame_ means markWorldSurfaces() saw this surface.
		if (!surfaceVisFrame_.empty() && visFrame_ > 0 && surfaceVisFrame_[i] != visFrame_) {
			continue;
		}

		const ScenePolyPacket& templatePacket = worldPacketTemplate_[i];
		ScenePolyPacket packet = templatePacket;

		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (shaderResource && shaderResource->hasScript && shaderResource->script.isPortal) {
			packet.isPortal = true;
		}

		// Determine whether this surface needs per-frame CPU deformation.
		bool hasActiveDeform = false;
		if (shaderResource && shaderResource->hasScript && shaderResource->script.hasDeform) {
			const MetalDeformInfo& deform = shaderResource->script.deform;
			if (deform.type == MetalDeformType::Wave || deform.type == MetalDeformType::Bulge) {
				hasActiveDeform = true;
			}
		}

		if (!hasActiveDeform) {
			// Static surface: reference geometry directly in the GPU-resident static buffer.
			// No per-frame vertex copy required.
			packet.isStaticWorld = true;
			polyPackets_.push_back(packet);
		} else {
			// Deformed surface: copy template vertices into the per-frame dynamic buffer
			// and apply the CPU-side deformation there.
			const size_t srcStart = static_cast<size_t>(templatePacket.firstVertex);
			const size_t srcEnd   = srcStart + static_cast<size_t>(templatePacket.vertexCount);
			if (srcEnd > worldVertexTemplate_.size()) {
				continue;
			}
			const size_t baseVertex = polyVertices_.size();
			polyVertices_.insert(polyVertices_.end(),
			                     worldVertexTemplate_.begin() + srcStart,
			                     worldVertexTemplate_.begin() + srcEnd);

			packet.firstVertex   = static_cast<int>(baseVertex);
			packet.isStaticWorld = false;

			MetalPolyVertex* packetVerts = &polyVertices_[packet.firstVertex];
			const MetalDeformInfo& deform = shaderResource->script.deform;
			if (deform.type == MetalDeformType::Wave) {
				ApplyDeformVertexesWave(packetVerts, packet.vertexCount, deform, sceneTimeSeconds);
			} else if (deform.type == MetalDeformType::Bulge) {
				ApplyDeformVertexesBulge(packetVerts, packet.vertexCount, deform, sceneTimeSeconds);
			}
			polyPackets_.push_back(packet);
		}
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

	// Load nodes and leaves for inPVS / R_PointInLeaf
	int nodeCount = 0;
	const dnode_t* bspNodes = reinterpret_cast<const dnode_t*>(getLumpRange(LUMP_NODES, nodeCount, sizeof(dnode_t)));
	int leafCount = 0;
	const dleaf_t* bspLeafs = reinterpret_cast<const dleaf_t*>(getLumpRange(LUMP_LEAFS, leafCount, sizeof(dleaf_t)));

	// Load entities lump for GetEntityToken
	int entitiesLen = 0;
	const char* entitiesData = reinterpret_cast<const char*>(getLumpRange(LUMP_ENTITIES, entitiesLen, 1));

	// Load cubemap probe origins from entity string early so they are available
	// when we assign indices to world packets later in this function.
	{
		const int numProbes = R_LoadCubemapProbeOrigins(entitiesData, entitiesLen);
		if (numProbes > 0 && ri_.Printf) {
			ri_.Printf(PRINT_ALL, "Metal: Found %d cubemap probe(s) in '%s'\n",
			           numProbes, requestedName.c_str());
		}
	}

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
	numWorldPackets_ = 0;
	
	// Load brush models FIRST to determine numWorldSurfaces_
	// This tells us which surfaces belong to the static world vs movers
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
					// Only surfaces 0 to numWorldSurfaces_-1 should be rendered as static world
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
					
					if (ri_.Printf) {
						ri_.Printf(PRINT_ALL, "Metal: Registered brush model '%s' handle=%d firstSurf=%d numSurf=%d\n",
						          modelName, handle, bm.firstSurface, bm.numSurfaces);
					}
				}
			}
			
			if (ri_.Printf && modelsLen > 1) {
				ri_.Printf(PRINT_ALL, "Metal: Loaded %d brush models (doors, platforms, movers)\n", modelsLen - 1);
			}
		}
	}
	
	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: World has %d surfaces, model 0 has %d surfaces (brush models use surfaces %d+)\n",
		          surfaceCount, numWorldSurfaces_, numWorldSurfaces_);
	}
	
	worldVertexTemplate_.reserve(static_cast<size_t>(vertexCount) * 2);
	worldPacketTemplate_.reserve(static_cast<size_t>(surfaceCount));
	worldSurfaceToPacket_.resize(surfaceCount, -1);  // -1 means surface was skipped

	// Adaptive subdivision level: world-space flatness tolerance in game units.
	// Clamp to 1..64 matching GL2.  Default 4 (GL2 default).
	const float subdivLevel = r_subdivisions_
	    ? static_cast<float>(std::max(1, std::min(64, r_subdivisions_->integer)))
	    : 4.f;

	// Pre-compute tessellated PatchGrids for ALL MST_PATCH surfaces,
	// then stitch adjacent grids to eliminate T-junction seams.
	// Uses unique_ptr because PatchGrid is large (~182 KB each).
	std::vector<std::unique_ptr<PatchGrid>> patchGrids;
	std::unordered_map<int, int> surfToPatchGrid;  // BSP surface index → patchGrids index

	for (int si = 0; si < surfaceCount; ++si) {
		const dsurface_t& ds2 = surfaces[si];
		if (LittleLong(ds2.surfaceType) != MST_PATCH) continue;

		const int pw = LittleLong(ds2.patchWidth);
		const int ph = LittleLong(ds2.patchHeight);
		const int fv = LittleLong(ds2.firstVert);
		const int nv = LittleLong(ds2.numVerts);

		if (pw < 3 || ph < 3 || (pw & 1) == 0 || (ph & 1) == 0) continue;
		if (fv < 0 || nv < pw * ph || fv + nv > vertexCount) continue;

		auto g = std::make_unique<PatchGrid>(
		    AdaptiveTessellate(&drawVerts[fv], pw, ph, subdivLevel,
		                       mapOverBrightBits, overBrightBits));
		surfToPatchGrid[si] = static_cast<int>(patchGrids.size());
		patchGrids.push_back(std::move(g));
	}

	// Stitch all patch pairs, matching GL2's R_StitchAllPatches:
	// for each patch, keep stitching against all others until stable.
	for (size_t pi = 0; pi < patchGrids.size(); pi++) {
		bool anyStitch = true;
		while (anyStitch) {
			anyStitch = false;
			for (size_t pj = 0; pj < patchGrids.size(); pj++) {
				if (pi == pj) continue;
				while (StitchPatchGrids(*patchGrids[pi], *patchGrids[pj]))
					anyStitch = true;
			}
		}
	}

	if (ri_.Printf && !patchGrids.empty()) {
		ri_.Printf(PRINT_ALL, "Metal: tessellated %zu patch surfaces (r_subdivisions=%.0f)\n",
		           patchGrids.size(), subdivLevel);
	}

	// Load ALL surfaces into the template (world + brush models)
	// Brush model surfaces need to be in the template so renderBrushModel can find them
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
			// Use the pre-computed, T-junction-stitched PatchGrid.
			auto patchIt = surfToPatchGrid.find(surfaceIndex);
			if (patchIt == surfToPatchGrid.end()) continue;  // invalid dimensions — skipped in pre-pass
			PatchGridToTriangles(*patchGrids[static_cast<size_t>(patchIt->second)], worldVertexTemplate_);
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
		
		// Track how many packets belong to the static world (model 0)
		// World surfaces are 0 to numWorldSurfaces_-1
		if (surfaceIndex < numWorldSurfaces_) {
			numWorldPackets_ = static_cast<int>(worldPacketTemplate_.size());
		}
	}
	
	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: Created %d world packets, %zu total packets (brush models use packets %d+)\n",
		          numWorldPackets_, worldPacketTemplate_.size(), numWorldPackets_);
	}

	// Initialize per-packet visibility frame array.
	// Sized to the full worldPacketTemplate_ (world + brush model packets).
	// Only entries 0..numWorldPackets_-1 are used for PVS culling.
	surfaceVisFrame_.assign(worldPacketTemplate_.size(), -1);
	visFrame_ = 0;

	// Load lightGrid for entity lighting
	// First parse entities to get gridSize, then load the grid data
	{
		vec3_t gridSize = {64.0f, 64.0f, 128.0f}; // Default grid size
		vec3_t gridOrigin{};
		int gridBounds[3] = {0, 0, 0};

		// Parse entities lump to find worldspawn gridsize
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

	// Load cubemap textures and assign probe indices to world surface packets.
	// This must happen after the light grid is loaded (for fallback solid-colour
	// cubemaps sampled from the light grid at each probe position).
	if (R_GetNumCubemapProbes() > 0) {
		loadWorldCubemaps(requestedName);
	} else {
		worldCubemapTextures_.clear();
	}

	// Store entity string so GetEntityToken can walk it token-by-token.
	// Match GL2's R_LoadEntities: copy the lump, reset the parse cursor.
	if (entitiesData && entitiesLen > 0) {
		entityStringBuf_.assign(entitiesData, entitiesData + entitiesLen);
		entityStringBuf_.push_back('\0');
	} else {
		entityStringBuf_.assign(1, '\0');
	}
	entityParsePoint_ = entityStringBuf_.data();

	// Scan entity lump for a sky portal entity so sky rendering knows to offset
	// the view origin to the portal position (GL2-compatible sky portal behaviour).
	parseSkyPortalEntity();

	// Store BSP spatial data for inPVS / R_PointInLeaf.
	if (bspNodes && nodeCount > 0) {
		bspNodes_.assign(bspNodes, bspNodes + nodeCount);
	}
	if (bspLeafs && leafCount > 0) {
		bspLeafs_.assign(bspLeafs, bspLeafs + leafCount);
	}
	if (planes && planeCount > 0) {
		bspPlanesForPVS_.assign(planes, planes + planeCount);
	}

	// Load LUMP_VISIBILITY for PVS-based world culling.
	// Format: int numClusters, int clusterBytes, then numClusters*clusterBytes bytes of PVS bits.
	{
		visNumClusters_ = 0;
		visClusterBytes_ = 0;
		visData_.clear();
		const lump_t& visLump = header->lumps[LUMP_VISIBILITY];
		const int visOfs = LittleLong(visLump.fileofs);
		const int visLen = LittleLong(visLump.filelen);
		if (visLen >= 8 && visOfs >= 0 && visOfs + visLen <= fileLen) {
			const byte* visRaw = reinterpret_cast<const byte*>(buffer) + visOfs;
			visNumClusters_ = LittleLong(reinterpret_cast<const int*>(visRaw)[0]);
			visClusterBytes_ = LittleLong(reinterpret_cast<const int*>(visRaw)[1]);
			const int dataBytes = visLen - 8;
			if (dataBytes > 0 && visNumClusters_ > 0 && visClusterBytes_ > 0) {
				visData_.assign(visRaw + 8, visRaw + 8 + dataBytes);
				if (ri_.Printf) {
					ri_.Printf(PRINT_ALL, "Metal: PVS loaded: %d clusters, %d bytes/cluster\n",
					           visNumClusters_, visClusterBytes_);
				}
			}
		}
	}

	// ---- Build MarkFragments BSP data ----
	// 1. BSP planes with signbits for BoxOnPlaneSide
	bspCPlanes_.resize(planeCount);
	for (int i = 0; i < planeCount; ++i) {
		cplane_t& cp  = bspCPlanes_[i];
		cp.normal[0]  = LittleFloat(planes[i].normal[0]);
		cp.normal[1]  = LittleFloat(planes[i].normal[1]);
		cp.normal[2]  = LittleFloat(planes[i].normal[2]);
		cp.dist       = LittleFloat(planes[i].dist);
		cp.type       = PlaneTypeForNormal(cp.normal);
		SetPlaneSignbits(&cp);
	}

	// 2. Leaf-surface index mapping (LUMP_LEAFSURFACES)
	{
		int leafSurfCount = 0;
		const int* leafSurfData = reinterpret_cast<const int*>(
		    getLumpRange(LUMP_LEAFSURFACES, leafSurfCount, sizeof(int)));
		bspLeafSurfaces_.resize(leafSurfCount);
		if (leafSurfData) {
			for (int i = 0; i < leafSurfCount; ++i)
				bspLeafSurfaces_[i] = LittleLong(leafSurfData[i]);
		}
	}

	// 3. Per-surface geometry for MarkFragments
	bspMarkSurfData_.clear();
	bspMarkSurfData_.resize(surfaceCount);
	bspSurfViewCounts_.assign(surfaceCount, -1);
	bspViewCount_ = 0;

	for (int si = 0; si < surfaceCount; ++si) {
		const dsurface_t& ds  = surfaces[si];
		const int surfType    = LittleLong(ds.surfaceType);
		BspMarkSurface&   ms  = bspMarkSurfData_[si];

		// Shader flags for culling in the box-surface traversal
		const int shNum = LittleLong(ds.shaderNum);
		if (shNum >= 0 && shNum < shaderCount) {
			ms.shaderSurfFlags = LittleLong(shaderTable[shNum].surfaceFlags);
			ms.shaderContFlags = LittleLong(shaderTable[shNum].contentFlags);
		}

		if (surfType == MST_PLANAR) {
			ms.type = BspMarkSurfType::Face;
			const int fv = LittleLong(ds.firstVert);
			const int nv = LittleLong(ds.numVerts);
			const int fi = LittleLong(ds.firstIndex);
			const int ni = LittleLong(ds.numIndexes);

			// Face plane from lightmapVecs[2] (the surface normal stored in the BSP)
			ms.cullNormal[0] = ds.lightmapVecs[2][0];
			ms.cullNormal[1] = ds.lightmapVecs[2][1];
			ms.cullNormal[2] = ds.lightmapVecs[2][2];
			if (fv >= 0 && fv < vertexCount) {
				ms.cullDist = ms.cullNormal[0] * drawVerts[fv].xyz[0]
				            + ms.cullNormal[1] * drawVerts[fv].xyz[1]
				            + ms.cullNormal[2] * drawVerts[fv].xyz[2];
			}
			ms.cullSignbits = 0;
			if (ms.cullNormal[0] < 0) ms.cullSignbits |= 1;
			if (ms.cullNormal[1] < 0) ms.cullSignbits |= 2;
			if (ms.cullNormal[2] < 0) ms.cullSignbits |= 4;
			ms.cullType = PlaneTypeForNormal(ms.cullNormal);

			// Build flat triangle list (3 BspMarkVerts per triangle)
			if (fv >= 0 && nv > 0 && fi >= 0 && ni >= 3
			    && fv + nv <= vertexCount && fi + ni <= indexCount) {
				ms.verts.reserve(ni);
				bool ok = true;
				for (int k = 0; k + 2 < ni && ok; k += 3) {
					for (int j = 0; j < 3; ++j) {
						const int idx = fv + LittleLong(drawIndexes[fi + k + j]);
						if (idx < 0 || idx >= vertexCount) { ms.verts.clear(); ok = false; break; }
						BspMarkVert v;
						v.xyz[0]    = drawVerts[idx].xyz[0];
						v.xyz[1]    = drawVerts[idx].xyz[1];
						v.xyz[2]    = drawVerts[idx].xyz[2];
						v.normal[0] = drawVerts[idx].normal[0];
						v.normal[1] = drawVerts[idx].normal[1];
						v.normal[2] = drawVerts[idx].normal[2];
						ms.verts.push_back(v);
					}
				}
			}

		} else if (surfType == MST_PATCH) {
			ms.type = BspMarkSurfType::Patch;
			const int pw = LittleLong(ds.patchWidth);
			const int ph = LittleLong(ds.patchHeight);
			const int fv = LittleLong(ds.firstVert);
			const int nv = LittleLong(ds.numVerts);
			if (pw >= 3 && ph >= 3 && (pw & 1) == 1 && (ph & 1) == 1
			    && fv >= 0 && nv >= pw * ph && fv + nv <= vertexCount) {
				// Tessellate the patch and store as a flat triangle list
				std::vector<MetalPolyVertex> tessVerts;
				TessellateBezierPatch(&drawVerts[fv], pw, ph, tessVerts,
				                      subdivLevel, mapOverBrightBits, overBrightBits);
				ms.verts.reserve(tessVerts.size());
				for (const auto& tv : tessVerts) {
					BspMarkVert v;
					v.xyz[0]    = tv.xyz[0]; v.xyz[1]    = tv.xyz[1]; v.xyz[2]    = tv.xyz[2];
					v.normal[0] = tv.normal[0]; v.normal[1] = tv.normal[1]; v.normal[2] = tv.normal[2];
					ms.verts.push_back(v);
				}
			}

		} else if (surfType == MST_TRIANGLE_SOUP) {
			ms.type = BspMarkSurfType::TriSoup;
			const int fv = LittleLong(ds.firstVert);
			const int nv = LittleLong(ds.numVerts);
			const int fi = LittleLong(ds.firstIndex);
			const int ni = LittleLong(ds.numIndexes);
			if (fv >= 0 && nv > 0 && fi >= 0 && ni >= 3
			    && fv + nv <= vertexCount && fi + ni <= indexCount) {
				ms.verts.reserve(ni);
				bool ok = true;
				for (int k = 0; k + 2 < ni && ok; k += 3) {
					for (int j = 0; j < 3; ++j) {
						const int idx = fv + LittleLong(drawIndexes[fi + k + j]);
						if (idx < 0 || idx >= vertexCount) { ms.verts.clear(); ok = false; break; }
						BspMarkVert v;
						v.xyz[0]    = drawVerts[idx].xyz[0];
						v.xyz[1]    = drawVerts[idx].xyz[1];
						v.xyz[2]    = drawVerts[idx].xyz[2];
						v.normal[0] = drawVerts[idx].normal[0];
						v.normal[1] = drawVerts[idx].normal[1];
						v.normal[2] = drawVerts[idx].normal[2];
						ms.verts.push_back(v);
					}
				}
			}
		}
	}
	// ---- End MarkFragments BSP data ----

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

		// Upload world geometry to a permanent GPU buffer once at map load.
		// drawPolyPackets() and renderBrushModel() draw directly from this
		// buffer with zero per-frame CPU→GPU copy for static surfaces.
		staticWorldVertexBuffer_.reset();
		staticWorldVertexBufferSize_ = 0;
		if (!worldVertexTemplate_.empty() && device_) {
			const size_t sz = worldVertexTemplate_.size() * sizeof(MetalPolyVertex);
			staticWorldVertexBuffer_.reset(device_->newBuffer(
			    worldVertexTemplate_.data(), sz, MTL::ResourceStorageModeShared));
			if (staticWorldVertexBuffer_) {
				staticWorldVertexBufferSize_ = sz;
				if (ri_.Printf) {
					ri_.Printf(PRINT_ALL,
					           "Metal: static world VBO: %zu verts, %.2f MB\n",
					           worldVertexTemplate_.size(),
					           static_cast<float>(sz) / (1024.0f * 1024.0f));
				}
			} else if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: failed to create static world vertex buffer\n");
			}
		}
	} else if (ri_.Printf) {
		ri_.Printf(PRINT_WARNING, "Metal: no supported surfaces found in '%s'\n", requestedName.c_str());
	}
	return worldLoaded_;
}

void MetalRenderer::unloadWorldMap() {
	worldLoaded_ = false;
	worldName_.clear();
	entityStringBuf_.clear();
	entityParsePoint_ = nullptr;
	hasSkyPortal_ = false;
	VectorClear(skyPortalOrigin_);
	bspNodes_.clear();
	bspLeafs_.clear();
	bspPlanesForPVS_.clear();
	bspLeafSurfaces_.clear();
	visData_.clear();
	visNumClusters_ = 0;
	visClusterBytes_ = 0;
	surfaceVisFrame_.clear();
	visFrame_ = 0;
	bspCPlanes_.clear();
	bspMarkSurfData_.clear();
	bspSurfViewCounts_.clear();
	bspViewCount_ = 0;
	staticWorldVertexBuffer_.reset();
	staticWorldVertexBufferSize_ = 0;
	worldVertexTemplate_.clear();
	worldPacketTemplate_.clear();
	worldLightmapHandles_.clear();
	worldCubemapTextures_.clear();
	worldSurfaceToPacket_.clear();
	brushModels_.clear();
	numWorldSurfaces_ = 0;
	numWorldPackets_ = 0;
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

/*
================
MetalRenderer::getEntityToken

Walk the entity string token by token, matching GL2's R_GetEntityToken exactly.
On end-of-string, reset the cursor and return qfalse.
================
*/
qboolean MetalRenderer::getEntityToken(char* buffer, int size) {
	const char* s = COM_Parse(&entityParsePoint_);
	Q_strncpyz(buffer, s, size);
	if (!entityParsePoint_ && !s[0]) {
		// End of string — reset cursor for the next call, return false (GL2 behaviour).
		entityParsePoint_ = entityStringBuf_.empty() ? nullptr : entityStringBuf_.data();
		return qfalse;
	}
	return qtrue;
}

/*
================
MetalRenderer::parseSkyPortalEntity

Scan the BSP entity lump for a sky portal entity.  Two conventions are
recognised (matching the skyboxportal feature referenced in GL2-based forks):

  1. An entity whose "classname" is "skyboxportal" or "misc_skyportal".
  2. Any entity that carries a key literally named "skyboxportal" (value
     ignored; its presence is the flag).

In either case the entity's "origin" key is parsed and the world-space
position is stored in skyPortalOrigin_.  The sky rendering passes
(drawSkybox / buildCloudSkyDome) then translate the sky to this position
instead of the player's eye origin, implementing the portal-sky effect.

Uses a local parse pointer so entityParsePoint_ (used by GetEntityToken)
is left undisturbed.
================
*/
void MetalRenderer::parseSkyPortalEntity() {
	hasSkyPortal_ = false;
	VectorClear(skyPortalOrigin_);

	if (entityStringBuf_.empty()) {
		return;
	}

	// COM_ParseExt requires a non-const char** — use a mutable pointer into our buffer.
	char* p = entityStringBuf_.data();

	while (p && *p) {
		// Find the opening brace of the next entity block.
		const char* tok = COM_ParseExt(&p, qtrue);
		if (!tok || !tok[0]) {
			break;
		}
		if (tok[0] != '{') {
			continue;
		}

		// Parse all key-value pairs inside this entity block.
		char classname[MAX_TOKEN_CHARS]  = {};
		char origin[MAX_TOKEN_CHARS]     = {};
		bool hasSkyboxportalKey          = false;

		while (p && *p) {
			// Read the key token.
			const char* key = COM_ParseExt(&p, qtrue);
			if (!key || !key[0] || key[0] == '}') {
				break;
			}

			// Read the value token.
			const char* value = COM_ParseExt(&p, qtrue);
			if (!value || !value[0] || value[0] == '}') {
				break;
			}

			if (Q_stricmp(key, "classname") == 0) {
				Q_strncpyz(classname, value, sizeof(classname));
			} else if (Q_stricmp(key, "origin") == 0) {
				Q_strncpyz(origin, value, sizeof(origin));
			} else if (Q_stricmp(key, "skyboxportal") == 0) {
				hasSkyboxportalKey = true;
			}
		}

		// Qualify as a sky portal entity by classname or explicit key.
		bool isSkyPortal = (Q_stricmp(classname, "skyboxportal")  == 0) ||
		                   (Q_stricmp(classname, "misc_skyportal") == 0) ||
		                   hasSkyboxportalKey;

		if (isSkyPortal && origin[0]) {
			float x = 0.0f, y = 0.0f, z = 0.0f;
			sscanf(origin, "%f %f %f", &x, &y, &z);
			skyPortalOrigin_[0] = x;
			skyPortalOrigin_[1] = y;
			skyPortalOrigin_[2] = z;
			hasSkyPortal_ = true;

			if (ri_.Printf) {
				ri_.Printf(PRINT_ALL,
				           "Metal: Sky portal entity '%s' at (%.1f, %.1f, %.1f)\n",
				           classname, x, y, z);
			}
			break;  // Only the first qualifying entity is used.
		}
	}
}

/*
================
MetalRenderer::pointInLeaf

Traverse the BSP tree and return the leaf that contains point p.
Mirrors GL2's R_PointInLeaf but works on raw dnode_t / dleaf_t data.
================
*/
const dleaf_t* MetalRenderer::pointInLeaf(const vec3_t p) const {
	if (bspNodes_.empty() || bspLeafs_.empty() || bspPlanesForPVS_.empty()) {
		return nullptr;
	}

	int nodeIndex = 0;  // root
	while (nodeIndex >= 0) {
		const dnode_t& node = bspNodes_[nodeIndex];
		const dplane_t& plane = bspPlanesForPVS_[node.planeNum];
		const float d = DotProduct(p, plane.normal) - plane.dist;
		nodeIndex = node.children[d <= 0 ? 1 : 0];
	}

	// nodeIndex encodes a leaf as -(leafIndex + 1)
	const int leafIndex = -(nodeIndex + 1);
	if (leafIndex < 0 || leafIndex >= static_cast<int>(bspLeafs_.size())) {
		return nullptr;
	}
	return &bspLeafs_[leafIndex];
}

/*
================
MetalRenderer::inPVS

Return qtrue if points p1 and p2 are in the same PVS cluster.
Matches GL2's R_inPVS (uses ri.CM_ClusterPVS for the visibility bit-array).
================
*/
qboolean MetalRenderer::inPVS(const vec3_t p1, const vec3_t p2) const {
	const dleaf_t* leaf1 = pointInLeaf(p1);
	if (!leaf1) {
		return qfalse;
	}
	const byte* vis = ri_.CM_ClusterPVS(leaf1->cluster);
	if (!vis) {
		return qfalse;
	}
	const dleaf_t* leaf2 = pointInLeaf(p2);
	if (!leaf2) {
		return qfalse;
	}
	if (!(vis[leaf2->cluster >> 3] & (1 << (leaf2->cluster & 7)))) {
		return qfalse;
	}
	return qtrue;
}

/*
================
MetalRenderer::getClusterPVS

Return the PVS byte row for a given cluster, or nullptr if unavailable.
A bit set at (pvs[cluster>>3] & (1<<(cluster&7))) means that cluster is visible.
================
*/
const byte* MetalRenderer::getClusterPVS(int cluster) const {
	if (visData_.empty() || cluster < 0 || cluster >= visNumClusters_) {
		return nullptr;
	}
	return visData_.data() + static_cast<size_t>(cluster) * static_cast<size_t>(visClusterBytes_);
}

/*
================
MetalRenderer::cullWorldBox

Test a world-space AABB (integer mins/maxs from BSP) against the current view frustum.
planeBits: bitmask of which of the 4 side planes to test (bit i = test plane i).
           If a box is fully inside a plane, that plane's bit is cleared so descendants
           can skip testing it (GL2's optimisation).
Returns true if the box is fully OUTSIDE the frustum (should be culled).
================
*/
bool MetalRenderer::cullWorldBox(const int imins[3], const int imaxs[3], uint32_t& planeBits) const {
	if (!sceneCamera_.frustumValid || planeBits == 0) {
		return false;
	}

	const float mins[3] = { static_cast<float>(imins[0]), static_cast<float>(imins[1]), static_cast<float>(imins[2]) };
	const float maxs[3] = { static_cast<float>(imaxs[0]), static_cast<float>(imaxs[1]), static_cast<float>(imaxs[2]) };

	// Test only the 4 side planes (skip near/far — indices 4 and 5) for BSP node traversal.
	// planeBits bit 0..3 correspond to frustumPlanes 0..3.
	for (int i = 0; i < 4; ++i) {
		if (!(planeBits & (1u << i))) {
			continue;
		}
		const float* p = sceneCamera_.frustumPlanes[i];

		// Positive vertex: the corner of the AABB that is most in the direction of the plane normal.
		// If this most-inside point is still outside, the whole box is culled.
		const float px = p[0] >= 0.0f ? maxs[0] : mins[0];
		const float py = p[1] >= 0.0f ? maxs[1] : mins[1];
		const float pz = p[2] >= 0.0f ? maxs[2] : mins[2];
		if (p[0] * px + p[1] * py + p[2] * pz + p[3] < 0.0f) {
			return true;  // Fully outside this plane — cull the subtree
		}

		// Negative vertex: the corner least in the plane's direction.
		// If this least-inside point is still inside, all descendants are also inside
		// this plane and we don't need to test it further.
		const float nx = p[0] >= 0.0f ? mins[0] : maxs[0];
		const float ny = p[1] >= 0.0f ? mins[1] : maxs[1];
		const float nz = p[2] >= 0.0f ? mins[2] : maxs[2];
		if (p[0] * nx + p[1] * ny + p[2] * nz + p[3] >= 0.0f) {
			planeBits &= ~(1u << i);
		}
	}
	return false;
}

/*
================
MetalRenderer::recursiveWorldNode

Recursive BSP traversal combining PVS and frustum culling, mirroring GL2's
R_RecursiveWorldNode.  For each leaf that passes both tests, the BSP surfaces
it references are marked visible for the current frame (surfaceVisFrame_).

nodeIndex: BSP node index (>= 0) or encoded leaf (< 0, decoded as -(idx+1)).
planeBits: bitmask of frustum planes still requiring testing.
pvs:       PVS byte-row for the camera's cluster (nullptr = all clusters visible).
================
*/
void MetalRenderer::recursiveWorldNode(int nodeIndex, uint32_t planeBits, const byte* pvs) {
	while (nodeIndex >= 0) {
		if (static_cast<size_t>(nodeIndex) >= bspNodes_.size()) {
			return;
		}
		const dnode_t& node = bspNodes_[nodeIndex];

		// Frustum-cull this node's AABB; clear planeBits for planes the box is fully inside.
		if (planeBits && cullWorldBox(node.mins, node.maxs, planeBits)) {
			return;
		}

		// Recurse into the front child, then tail-recurse into the back child.
		recursiveWorldNode(node.children[0], planeBits, pvs);
		nodeIndex = node.children[1];
	}

	// nodeIndex < 0: it's a leaf encoded as -(leafIndex + 1).
	const int leafIndex = -(nodeIndex + 1);
	if (leafIndex < 0 || leafIndex >= static_cast<int>(bspLeafs_.size())) {
		return;
	}
	const dleaf_t& leaf = bspLeafs_[leafIndex];

	// PVS check: skip this leaf if its cluster is not visible from the camera cluster.
	const int cluster = leaf.cluster;
	if (pvs && cluster >= 0 && cluster < visNumClusters_) {
		if (!(pvs[cluster >> 3] & (1 << (cluster & 7)))) {
			return;
		}
	}

	// Mark all BSP surfaces in this leaf as visible this frame.
	const int first = leaf.firstLeafSurface;
	const int count = leaf.numLeafSurfaces;
	if (first < 0 || count <= 0 || first + count > static_cast<int>(bspLeafSurfaces_.size())) {
		return;
	}
	for (int i = 0; i < count; ++i) {
		const int bspSurf = bspLeafSurfaces_[first + i];
		// Skip BSP surface indices belonging to brush models (>= numWorldSurfaces_).
		if (bspSurf < 0 || bspSurf >= numWorldSurfaces_) {
			continue;
		}
		const int packetIdx = worldSurfaceToPacket_[bspSurf];
		if (packetIdx < 0 || packetIdx >= static_cast<int>(surfaceVisFrame_.size())) {
			continue;
		}
		surfaceVisFrame_[packetIdx] = visFrame_;
	}
}

/*
================
MetalRenderer::markWorldSurfaces

Top-level PVS + frustum traversal called once per frame before appendWorldGeometry().
Increments visFrame_ and marks only the surfaces visible from the camera position.
If no PVS data is available, all world surfaces are marked (safe fallback).
================
*/
void MetalRenderer::markWorldSurfaces() {
	if (!worldLoaded_ || bspNodes_.empty() || bspLeafs_.empty() || surfaceVisFrame_.empty()) {
		return;
	}

	// Advance the per-frame visibility counter.
	++visFrame_;

	// Find the BSP leaf containing the camera.
	const dleaf_t* cameraLeaf = pointInLeaf(sceneCamera_.viewOrigin);
	if (!cameraLeaf) {
		// Cannot locate camera in tree — mark everything visible as a safe fallback.
		const int n = std::min(static_cast<int>(surfaceVisFrame_.size()), numWorldPackets_);
		for (int i = 0; i < n; ++i) {
			surfaceVisFrame_[i] = visFrame_;
		}
		return;
	}

	// Get the PVS row for the camera's cluster.
	const int cameraCluster = cameraLeaf->cluster;
	const byte* pvs = (cameraCluster >= 0) ? getClusterPVS(cameraCluster) : nullptr;

	// If no PVS data was loaded, fall back to drawing everything (frustum culling still applies).
	if (visData_.empty()) {
		pvs = nullptr;
	}

	// Traverse the BSP tree from the root, testing 4 frustum side planes.
	const uint32_t planeBits = sceneCamera_.frustumValid ? 15u : 0u;
	recursiveWorldNode(0, planeBits, pvs);
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
	// No depth attachment — cinematics render in the 2D encoder which has no depth.

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

// Create an additive (GL_ONE / GL_ONE) 2D pipeline for lens flare rendering.
bool MetalRenderer::createFlarePipeline() {
	if (pipelineFlare_) {
		return true;
	}
	if (!device_) {
		return false;
	}

	NS::Error* error = nullptr;
	MTL::Library* lib = device_->newDefaultLibrary();
	if (!lib) {
		ri_.Printf(PRINT_WARNING, "Metal: createFlarePipeline - failed to load shader library\n");
		return false;
	}

	NS::String* vertexName   = NS::String::string("vertex_ui_2d",   NS::ASCIIStringEncoding);
	NS::String* fragmentName = NS::String::string("fragment_ui_2d", NS::ASCIIStringEncoding);
	MTL::Function* vfn = lib->newFunction(vertexName);
	MTL::Function* ffn = lib->newFunction(fragmentName);
	vertexName->release();
	fragmentName->release();

	if (!vfn || !ffn) {
		ri_.Printf(PRINT_WARNING, "Metal: createFlarePipeline - 2D shader functions not found\n");
		if (vfn) vfn->release();
		if (ffn) ffn->release();
		lib->release();
		return false;
	}

	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexFunction(vfn);
	pd->setFragmentFunction(ffn);
	pd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	// Additive blend: src=One, dst=One (GL_ONE, GL_ONE)
	auto* ca = pd->colorAttachments()->object(0);
	ca->setBlendingEnabled(true);
	ca->setSourceRGBBlendFactor(MTL::BlendFactorOne);
	ca->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
	ca->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	ca->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);

	pipelineFlare_.reset(device_->newRenderPipelineState(pd, &error));
	pd->release();
	vfn->release();
	ffn->release();
	lib->release();

	if (!pipelineFlare_) {
		if (error) {
			ri_.Printf(PRINT_WARNING, "Metal: flare pipeline creation failed: %s\n",
			           error->localizedDescription()->utf8String());
			error->release();
		}
		return false;
	}

	return true;
}

// Draw a screen-space quad with additive blending for lens flares.
// (x, y, w, h) are in screen pixels; y=0 is the top of the window.
// (r, g, b) are linear colour values in [0, 1], pre-multiplied by intensity.
void MetalRenderer::drawFlareQuad(float x, float y, float w, float h,
                                  float r, float g, float b, qhandle_t shader) {
	if (!device_ || !currentRenderEncoder_) {
		return;
	}

	if (!createFlarePipeline()) {
		return;
	}

	// Ensure the 2D sampler exists.
	if (!sampler2D_) {
		create2DPipeline();
	}
	if (!sampler2D_) {
		return;
	}

	TextureManager* tm = ensureTextureManager();
	if (!tm) {
		return;
	}

	MetalShaderResource* shaderRes = getShaderResource(shader);
	if (!shaderRes) {
		shaderRes = getShaderResource(0);
	}
	if (!shaderRes) {
		return;
	}

	const auto* stageRuntime = getPrimaryStageRuntime(*shaderRes);
	float timeSeconds = ri_.Milliseconds ? static_cast<float>(ri_.Milliseconds()) * 0.001f : 0.0f;
	qhandle_t imageHandle = selectStageImage(*shaderRes, stageRuntime, timeSeconds);
	MTL::Texture* texture = tm->getTexture(imageHandle);
	if (!texture) {
		texture = tm->getTexture(0);
		if (!texture) return;
	}

	// Convert screen pixel coords (y=0 at top) to Metal NDC (y=+1 at top).
	float ndcX =  (x     * 2.0f / config_.vidWidth)  - 1.0f;
	float ndcY =  1.0f - (y     * 2.0f / config_.vidHeight);
	float ndcW =   w    * 2.0f / config_.vidWidth;
	float ndcH =   h    * 2.0f / config_.vidHeight;

	// Must match ui_2d.metal QuadInstance layout exactly.
	struct QuadInstance {
		float rect[4];
		float texCoords[4];
		float color[4];
		float pivot[2];
		float rotation;
		float _pad;
	};

	QuadInstance instance;
	instance.rect[0] = ndcX;
	instance.rect[1] = ndcY;
	instance.rect[2] = ndcW;
	instance.rect[3] = -ndcH;  // negative height: draw downward from top
	instance.texCoords[0] = 0.0f;
	instance.texCoords[1] = 0.0f;
	instance.texCoords[2] = 1.0f;
	instance.texCoords[3] = 1.0f;
	instance.color[0] = r;
	instance.color[1] = g;
	instance.color[2] = b;
	instance.color[3] = 1.0f;
	instance.pivot[0] = 0.0f;
	instance.pivot[1] = 0.0f;
	instance.rotation = 0.0f;
	instance._pad     = 0.0f;

	// Full-window viewport (same as drawStretchPic).
	MTL::Viewport vp;
	vp.originX = 0.0;
	vp.originY = 0.0;
	vp.width   = static_cast<double>(config_.vidWidth);
	vp.height  = static_cast<double>(config_.vidHeight);
	vp.znear   = 0.0;
	vp.zfar    = 1.0;
	currentRenderEncoder_->setViewport(vp);

	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineFlare_.get());
	if (depthState2D_) {
		currentRenderEncoder_->setDepthStencilState(depthState2D_.get());
	}
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);
	currentRenderEncoder_->setVertexBytes(&instance, sizeof(instance), 0);
	MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, texture);
	MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler2D_.get());
	currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangleStrip,
	                                      NS::UInteger(0), NS::UInteger(4), NS::UInteger(1));
}

// Register (lazily) and return the flare shader handle.
qhandle_t MetalRenderer::getFlareShader() {
	if (!flareShaderHandle_) {
		flareShaderHandle_ = registerShader("flareShader", false);
		if (!flareShaderHandle_) {
			// Fall back to the white texture if no flare shader is defined in game data.
			flareShaderHandle_ = registerShader("white", false);
		}
	}
	return flareShaderHandle_;
}

// Fill out the current scene camera matrices and viewport for tr_flares.cpp.
void MetalRenderer::getSceneCameraForFlares(float viewMat[16], float projMat[16],
                                            float viewOrg[3],
                                            int* vpX, int* vpY, int* vpW, int* vpH) const {
	if (!sceneCamera_.valid) {
		for (int i = 0; i < 16; i++) { viewMat[i] = projMat[i] = 0.0f; }
		viewOrg[0] = viewOrg[1] = viewOrg[2] = 0.0f;
		if (vpX) *vpX = 0;
		if (vpY) *vpY = 0;
		if (vpW) *vpW = 0;
		if (vpH) *vpH = 0;
		return;
	}

	Mat4Copy(sceneCamera_.viewMatrix,       viewMat);
	Mat4Copy(sceneCamera_.projectionMatrix, projMat);
	VectorCopy(sceneCamera_.viewOrigin, viewOrg);

	const refdef_t& rd = sceneCamera_.refdef;
	if (vpX) *vpX = rd.x;
	if (vpY) *vpY = rd.y;
	if (vpW) *vpW = rd.width;
	if (vpH) *vpH = rd.height;
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
	// Always add ShaderRead so the SSAO pass can sample depth.
	desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

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

// ---------------------------------------------------------------------------
// Post-processing infrastructure
// ---------------------------------------------------------------------------

// Helper: allocate a 2-D render target or return the existing one if size unchanged.
static MetalPtr<MTL::Texture> makeRenderTarget(MTL::Device* dev,
                                               MTL::PixelFormat fmt,
                                               int w, int h,
                                               const char* label = nullptr) {
	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(fmt, w, h, false);
	desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
	desc->setStorageMode(MTL::StorageModePrivate);
	MTL::Texture* tex = dev->newTexture(desc);
	desc->release();
	if (tex && label) {
		tex->setLabel(NS::String::string(label, NS::UTF8StringEncoding));
	}
	MetalPtr<MTL::Texture> result;
	result.reset(tex);
	return result;
}

bool MetalRenderer::ensureHDRTargets(int width, int height) {
	if (!device_ || width <= 0 || height <= 0) {
		return false;
	}

	const int halfW = std::max(1, width / 2);
	const int halfH = std::max(1, height / 2);
	const int lumW  = std::max(1, std::min(256, width  / 4));
	const int lumH  = std::max(1, std::min(256, height / 4));

	// Recreate HDR target on resize.
	if (!hdrColorTarget_ || hdrTargetWidth_ != width || hdrTargetHeight_ != height) {
		hdrColorTarget_ = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, width, height, "hdrColorTarget");
		if (!hdrColorTarget_) { return false; }
		hdrTargetWidth_  = width;
		hdrTargetHeight_ = height;
	}

	// Bloom / sun-ray scratch at half resolution.
	if (!bloomTarget_[0] || bloomTargetWidth_ != halfW || bloomTargetHeight_ != halfH) {
		bloomTarget_[0] = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, halfW, halfH, "bloom0");
		bloomTarget_[1] = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, halfW, halfH, "bloom1");
		if (!bloomTarget_[0] || !bloomTarget_[1]) { return false; }
		bloomTargetWidth_  = halfW;
		bloomTargetHeight_ = halfH;
	}

	// SSAO target full-res (single-channel packed as R in RGBA8).
	if (!ssaoTarget_ || ssaoTargetWidth_ != width || ssaoTargetHeight_ != height) {
		ssaoTarget_ = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA8Unorm, width, height, "ssao");
		if (!ssaoTarget_) { return false; }
		ssaoTargetWidth_  = width;
		ssaoTargetHeight_ = height;
	}

	// Luminance downsample scratch.
	if (!lumScratch_[0] || lumScratchWidth_ != lumW || lumScratchHeight_ != lumH) {
		lumScratch_[0] = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, lumW, lumH, "lumScratch0");
		lumScratch_[1] = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, lumW, lumH, "lumScratch1");
		if (!lumScratch_[0] || !lumScratch_[1]) { return false; }
		lumScratchWidth_  = lumW;
		lumScratchHeight_ = lumH;
	}

	// Raw 1×1 luminance target.
	if (!lumRawTarget_) {
		lumRawTarget_ = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, 1, 1, "lumRaw");
		if (!lumRawTarget_) { return false; }
	}

	// Temporal accumulation targets.
	if (!lumAccumTarget_[0]) {
		lumAccumTarget_[0] = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, 1, 1, "lumAccum0");
		lumAccumTarget_[1] = makeRenderTarget(device_.get(), MTL::PixelFormatRGBA16Float, 1, 1, "lumAccum1");
		if (!lumAccumTarget_[0] || !lumAccumTarget_[1]) { return false; }
		lumAccumInitialized_ = false;
	}

	return true;
}

// Build a single post-process PSO.
MTL::RenderPipelineState* MetalRenderer::makePPPso(MTL::Library* lib,
                                                   const char* vfnName,
                                                   const char* ffnName,
                                                   MTL::PixelFormat colorFmt,
                                                   bool additiveBlend) {
	NS::Error* err = nullptr;
	auto vfn = NS::TransferPtr(lib->newFunction(
		NS::String::string(vfnName, NS::UTF8StringEncoding)));
	auto ffn = NS::TransferPtr(lib->newFunction(
		NS::String::string(ffnName, NS::UTF8StringEncoding)));
	if (!vfn || !ffn) {
		if (ri_.Printf) ri_.Printf(PRINT_WARNING,
			"Metal post-process: missing shader %s / %s\n", vfnName, ffnName);
		return nullptr;
	}

	auto* rpdesc = MTL::RenderPipelineDescriptor::alloc()->init();
	rpdesc->setVertexFunction(vfn.get());
	rpdesc->setFragmentFunction(ffn.get());
	rpdesc->colorAttachments()->object(0)->setPixelFormat(colorFmt);

	if (additiveBlend) {
		rpdesc->colorAttachments()->object(0)->setBlendingEnabled(true);
		rpdesc->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorOne);
		rpdesc->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
		rpdesc->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
		rpdesc->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
	}

	MTL::RenderPipelineState* pso = device_->newRenderPipelineState(rpdesc, &err);
	rpdesc->release();
	if (!pso && ri_.Printf) {
		ri_.Printf(PRINT_WARNING, "Metal post-process PSO compile error: %s\n",
		           err ? err->localizedDescription()->utf8String() : "unknown");
	}
	return pso;
}

bool MetalRenderer::createPostProcessPipelines() {
	if (ppTonemapPso_) {
		return true; // already created
	}
	if (!device_) return false;

	auto* lib = device_->newDefaultLibrary();
	if (!lib) {
		if (ri_.Printf) ri_.Printf(PRINT_WARNING,
			"Metal post-process: could not load default library\n");
		return false;
	}

	auto fmt16  = MTL::PixelFormatRGBA16Float;
	auto fmt8   = MTL::PixelFormatRGBA8Unorm;
	auto fmtDraw = currentDrawable_ ? currentDrawable_->texture()->pixelFormat()
	                                : MTL::PixelFormatBGRA8Unorm;

	ppPassthroughPso_     .reset(makePPPso(lib, "vertex_fullscreen", "fragment_passthrough",          fmt16));
	ppDownsample4xPso_    .reset(makePPPso(lib, "vertex_fullscreen", "fragment_downsample4x",         fmt16));
	ppCalcLevelsFirstPso_ .reset(makePPPso(lib, "vertex_fullscreen", "fragment_calc_levels_first",    fmt16));
	ppCalcLevelsPso_      .reset(makePPPso(lib, "vertex_fullscreen", "fragment_calc_levels",          fmt16));
	ppLumBlendPso_        .reset(makePPPso(lib, "vertex_fullscreen", "fragment_lum_blend",            fmt16));
	ppTonemapPso_         .reset(makePPPso(lib, "vertex_fullscreen", "fragment_tonemap",              fmtDraw));
	ppBloomExtractPso_    .reset(makePPPso(lib, "vertex_fullscreen", "fragment_bloom_extract",        fmt16));
	ppGaussianBlurHPso_   .reset(makePPPso(lib, "vertex_fullscreen", "fragment_gaussian_blur_h",      fmt16));
	ppGaussianBlurVPso_   .reset(makePPPso(lib, "vertex_fullscreen", "fragment_gaussian_blur_v",      fmt16));
	ppSSAOPso_            .reset(makePPPso(lib, "vertex_fullscreen", "fragment_ssao",                 fmt8));
	ppDepthBlurPso_       .reset(makePPPso(lib, "vertex_fullscreen", "fragment_depthblur",            fmt8));
	ppSunRaysPso_         .reset(makePPPso(lib, "vertex_fullscreen", "fragment_sun_rays",             fmtDraw, /*additive=*/true));
	ppDofBlurPso_         .reset(makePPPso(lib, "vertex_fullscreen", "fragment_dof_blur",             fmt16));
	ppAdditivePso_        .reset(makePPPso(lib, "vertex_fullscreen", "fragment_passthrough",          fmtDraw, /*additive=*/true));

	lib->release();

	// Build a shared linear-clamp sampler for all PP passes.
	if (!ppLinearSampler_) {
		MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
		sd->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sd->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sd->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sd->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		ppLinearSampler_.reset(device_->newSamplerState(sd));
		sd->release();
	}

	return static_cast<bool>(ppTonemapPso_);
}

// Run a single full-screen post-process pass and end the encoder.
void MetalRenderer::doRenderPass(MTL::RenderPipelineState* pso,
                                 MTL::Texture* src0, MTL::Texture* src1, MTL::Texture* src2,
                                 MTL::Texture* dst,
                                 MTL::LoadAction loadAction,
                                 const void* uniforms, size_t uniformSize) {
	if (!pso || !dst || !currentCommandBuffer_) return;

	MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
	rpd->colorAttachments()->object(0)->setTexture(dst);
	rpd->colorAttachments()->object(0)->setLoadAction(loadAction);
	rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
	rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

	auto* enc = currentCommandBuffer_->renderCommandEncoder(rpd);
	rpd->release();
	if (!enc) return;

	enc->setRenderPipelineState(pso);
	enc->setFragmentSamplerState(ppLinearSampler_.get(), 0);
	if (src0) enc->setFragmentTexture(src0, 0);
	if (src1) enc->setFragmentTexture(src1, 1);
	if (src2) enc->setFragmentTexture(src2, 2);
	if (uniforms && uniformSize > 0) {
		enc->setFragmentBytes(uniforms, uniformSize, 0);
	}
	enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
	enc->endEncoding();
	enc->release();
}

void MetalRenderer::ensurePostProcessed() {
	if (!usingHDRRenderPath_ || postProcessingDone_) {
		return;
	}
	postProcessingDone_ = true;
	runPostProcessing();
}

void MetalRenderer::runPostProcessing() {
	if (!currentCommandBuffer_ || !currentDrawable_) return;

	// End the scene (HDR) encoder before starting compute passes.
	if (currentRenderEncoder_) {
		currentRenderEncoder_->endEncoding();
		currentRenderEncoder_->release();
		MetalStateCache::Instance().resetEncoder(nullptr);
		currentRenderEncoder_ = nullptr;
	}

	if (!createPostProcessPipelines()) {
		// Fall back: blit HDR target to drawable via passthrough.
		goto create_2d_encoder;
	}

	// -----------------------------------------------------------------------
	// 1. Luminance chain: build per-frame log-lum then temporal smooth
	// -----------------------------------------------------------------------
	if (r_autoExposure_ && r_autoExposure_->integer && lumScratch_[0] && lumRawTarget_) {
		// First pass: log-lum from full HDR → lumScratch_[0] (256×256 ish)
		doRenderPass(ppCalcLevelsFirstPso_.get(),
		             hdrColorTarget_.get(), nullptr, nullptr,
		             lumScratch_[0].get(),
		             MTL::LoadActionDontCare,
		             nullptr, 0);

		// Downsample chain: ping-pong until 1×1.
		int src = 0;
		int curW = lumScratchWidth_, curH = lumScratchHeight_;
		while (curW > 1 || curH > 1) {
			int nextW = std::max(1, curW / 2);
			int nextH = std::max(1, curH / 2);
			int dst = src ^ 1;
			// Re-use the scratch textures — they are large enough for the initial
			// pass; for the tiny passes we just render into the corner pixels.
			// When we reach 1×1 write to lumRawTarget_.
			MTL::Texture* dstTex = (nextW == 1 && nextH == 1) ? lumRawTarget_.get() : lumScratch_[dst].get();
			doRenderPass(ppCalcLevelsPso_.get(),
			             lumScratch_[src].get(), nullptr, nullptr,
			             dstTex,
			             MTL::LoadActionDontCare,
			             nullptr, 0);
			src = dst;
			curW = nextW;
			curH = nextH;
		}

		// Temporal blend: lumRaw → lumAccum (ping-pong).
		if (!lumAccumInitialized_) {
			// Prime the accumulator with the raw value on first use.
			doRenderPass(ppPassthroughPso_.get(),
			             lumRawTarget_.get(), nullptr, nullptr,
			             lumAccumTarget_[lumAccumIdx_].get(),
			             MTL::LoadActionDontCare,
			             nullptr, 0);
			lumAccumInitialized_ = true;
		}

		int nextAccum = lumAccumIdx_ ^ 1;
		struct LumBlendParams { float alpha; float _pad[3]; };
		LumBlendParams lbp;
		lbp.alpha = 0.03f; // 3% new per frame, matches GL2
		// Clamp to faster response when cvar allows.
		if (r_autoExposure_ && r_autoExposure_->integer == 2) lbp.alpha = 0.1f;
		doRenderPass(ppLumBlendPso_.get(),
		             lumRawTarget_.get(),
		             lumAccumTarget_[lumAccumIdx_].get(),
		             nullptr,
		             lumAccumTarget_[nextAccum].get(),
		             MTL::LoadActionDontCare,
		             &lbp, sizeof(lbp));
		lumAccumIdx_ = nextAccum;
	}

	// -----------------------------------------------------------------------
	// 2. SSAO
	// -----------------------------------------------------------------------
	if (r_ssao_ && r_ssao_->integer && depthTexture_ && ssaoTarget_) {
		struct SSAOParams {
			float viewInfo[4]; // (zFar/zNear, zFar, 1/w, 1/h)
			float invProj[16];
		};
		SSAOParams sp{};
		sp.viewInfo[0] = sceneCamera_.zFar / sceneCamera_.zNear;
		sp.viewInfo[1] = sceneCamera_.zFar;
		sp.viewInfo[2] = 1.0f / (float)config_.vidWidth;
		sp.viewInfo[3] = 1.0f / (float)config_.vidHeight;
		// We don't have the projection inverse readily — pass zeros; the
		// shader falls back to the depth buffer directly.
		doRenderPass(ppSSAOPso_.get(),
		             depthTexture_.get(), nullptr, nullptr,
		             ssaoTarget_.get(),
		             MTL::LoadActionDontCare,
		             &sp, sizeof(sp));

		// Depth-aware bilateral blur (H pass → bloomTarget_[0], V pass → ssaoTarget_).
		{
			struct DepthBlurParams {
				float viewInfo[4];   // x=zFar/zNear, y=zFar, z=1/w, w=1/h
				float scale[2];      // texel-space blur half-axis
				float isHorizontal;
				float _pad;
			};
			DepthBlurParams dbpH{}, dbpV{};
			const float invW = config_.vidWidth  > 0 ? 1.0f / (float)config_.vidWidth  : 0.0f;
			const float invH = config_.vidHeight > 0 ? 1.0f / (float)config_.vidHeight : 0.0f;
			dbpH.viewInfo[0] = dbpV.viewInfo[0] = sceneCamera_.zFar / std::max(sceneCamera_.zNear, 0.001f);
			dbpH.viewInfo[1] = dbpV.viewInfo[1] = sceneCamera_.zFar;
			dbpH.viewInfo[2] = dbpV.viewInfo[2] = invW;
			dbpH.viewInfo[3] = dbpV.viewInfo[3] = invH;
			dbpH.scale[0] = dbpV.scale[0] = invW;
			dbpH.scale[1] = dbpV.scale[1] = invH;
			dbpH.isHorizontal = 1.0f;
			dbpV.isHorizontal = 0.0f;
			doRenderPass(ppDepthBlurPso_.get(),
			             ssaoTarget_.get(), depthTexture_.get(), nullptr,
			             bloomTarget_[0].get(),
			             MTL::LoadActionDontCare,
			             &dbpH, sizeof(dbpH));
			doRenderPass(ppDepthBlurPso_.get(),
			             bloomTarget_[0].get(), depthTexture_.get(), nullptr,
			             ssaoTarget_.get(),
			             MTL::LoadActionDontCare,
			             &dbpV, sizeof(dbpV));
		}
	}

	// -----------------------------------------------------------------------
	// 3. Tonemap (HDR → drawable)
	// -----------------------------------------------------------------------
	{
		struct TonemapParams {
			float exposureBias;
			float autoExposureMinMax[2];   // [0]=min, [1]=max
			float toneAvgFactor;
			int   hasSSAO;
			int   hasAutoExposure;
			float _pad[2];
		};
		TonemapParams tp{};
		tp.exposureBias         = r_cameraExposure_ ? r_cameraExposure_->value : 0.0f;
		tp.autoExposureMinMax[0] = r_autoExposureMinValue_ ? r_autoExposureMinValue_->value : -2.0f;
		tp.autoExposureMinMax[1] = r_autoExposureMaxValue_ ? r_autoExposureMaxValue_->value :  2.0f;
		tp.toneAvgFactor         = r_tonemapExposure_ ? r_tonemapExposure_->value : 0.18f;
		tp.hasSSAO               = (r_ssao_ && r_ssao_->integer) ? 1 : 0;

		// Bind HDR source, optional SSAO, optional lum accum.
		MTL::Texture* lumTex = lumAccumInitialized_ ? lumAccumTarget_[lumAccumIdx_].get() : nullptr;
		MTL::Texture* aoTex  = (tp.hasSSAO && ssaoTarget_) ? ssaoTarget_.get() : nullptr;
		// Only enable auto-exposure if the luminance accumulator texture is ready.
		tp.hasAutoExposure = (r_autoExposure_ && r_autoExposure_->integer && lumTex) ? 1 : 0;

		MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
		rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
		rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
		rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
		rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
		auto* enc = currentCommandBuffer_->renderCommandEncoder(rpd);
		rpd->release();
		if (enc) {
			enc->setRenderPipelineState(ppTonemapPso_.get());
			enc->setFragmentSamplerState(ppLinearSampler_.get(), 0);
			enc->setFragmentTexture(hdrColorTarget_.get(), 0);
			if (lumTex) enc->setFragmentTexture(lumTex, 1);
			if (aoTex)  enc->setFragmentTexture(aoTex,  2);
			enc->setFragmentBytes(&tp, sizeof(tp), 0);
			enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
			enc->endEncoding();
			enc->release();
		}
	}

	// -----------------------------------------------------------------------
	// 4. Bloom (extract bright → blur → additive composite onto drawable)
	// -----------------------------------------------------------------------
	if (r_bloom_ && r_bloom_->integer && bloomTarget_[0] && ppBloomExtractPso_) {
		struct BloomExtractParams {
			float threshold;
			float strength;
			float _pad[2];
		};
		BloomExtractParams bep{};
		bep.threshold = r_bloomThreshold_ ? r_bloomThreshold_->value : 1.0f;
		bep.strength  = r_bloomStrength_  ? r_bloomStrength_->value  : 0.5f;

		// Extract bright pixels → bloomTarget_[0]
		doRenderPass(ppBloomExtractPso_.get(),
		             hdrColorTarget_.get(), nullptr, nullptr,
		             bloomTarget_[0].get(),
		             MTL::LoadActionDontCare,
		             &bep, sizeof(bep));

		// H blur → bloomTarget_[1], V blur → bloomTarget_[0]
		{
			const float invW = bloomTargetWidth_  > 0 ? 1.0f / (float)bloomTargetWidth_  : 0.0f;
			const float invH = bloomTargetHeight_ > 0 ? 1.0f / (float)bloomTargetHeight_ : 0.0f;
			const float blurParams[4] = { invW, invH, 1.0f, 0.0f };
			doRenderPass(ppGaussianBlurHPso_.get(),
			             bloomTarget_[0].get(), nullptr, nullptr,
			             bloomTarget_[1].get(),
			             MTL::LoadActionDontCare,
			             blurParams, sizeof(blurParams));
			doRenderPass(ppGaussianBlurVPso_.get(),
			             bloomTarget_[1].get(), nullptr, nullptr,
			             bloomTarget_[0].get(),
			             MTL::LoadActionDontCare,
			             blurParams, sizeof(blurParams));
		}

		// Additive composite onto drawable.
		struct { float strength; float _pad[3]; } bc{ bep.strength };
		MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
		rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
		rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
		rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
		auto* enc = currentCommandBuffer_->renderCommandEncoder(rpd);
		rpd->release();
		if (enc) {
			enc->setRenderPipelineState(ppAdditivePso_.get());
			enc->setFragmentSamplerState(ppLinearSampler_.get(), 0);
			enc->setFragmentTexture(bloomTarget_[0].get(), 0);
			enc->setFragmentBytes(&bc, sizeof(bc), 0);
			enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
			enc->endEncoding();
			enc->release();
		}
	}

	// -----------------------------------------------------------------------
	// 5. Sun rays (additive radial blur onto drawable)
	// -----------------------------------------------------------------------
	if (r_sunlightMode_ && r_sunlightMode_->integer && sceneCamera_.valid) {
		vec3_t sunDir;
		R_GetSunDirection(sunDir);
		float dotVal = DotProduct(sunDir, sceneCamera_.viewAxis[0]);
		if (dotVal >= 0.25f) {
			float dist = sceneCamera_.zFar / 1.75f;
			float wx = sceneCamera_.viewOrigin[0] + sunDir[0] * dist;
			float wy = sceneCamera_.viewOrigin[1] + sunDir[1] * dist;
			float wz = sceneCamera_.viewOrigin[2] + sunDir[2] * dist;

			// Project into clip space using the stored viewProjectionMatrix.
			float cx = sceneCamera_.viewProjectionMatrix[0]*wx + sceneCamera_.viewProjectionMatrix[4]*wy + sceneCamera_.viewProjectionMatrix[8]*wz  + sceneCamera_.viewProjectionMatrix[12];
			float cy = sceneCamera_.viewProjectionMatrix[1]*wx + sceneCamera_.viewProjectionMatrix[5]*wy + sceneCamera_.viewProjectionMatrix[9]*wz  + sceneCamera_.viewProjectionMatrix[13];
			float cw = sceneCamera_.viewProjectionMatrix[3]*wx + sceneCamera_.viewProjectionMatrix[7]*wy + sceneCamera_.viewProjectionMatrix[11]*wz + sceneCamera_.viewProjectionMatrix[15];

			if (cw > 0.0f) {
				float sunU = 0.5f + cx / cw * 0.5f;
				float sunV = 0.5f - cy / cw * 0.5f;

				struct SunRaysParams {
					float sunPos[2];
					float exposure;
					float _pad;
				};
				SunRaysParams srp{};
				srp.sunPos[0] = sunU;
				srp.sunPos[1] = sunV;
				srp.exposure  = dotVal;

				MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
				rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
				rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
				rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
				auto* enc = currentCommandBuffer_->renderCommandEncoder(rpd);
				rpd->release();
				if (enc) {
					enc->setRenderPipelineState(ppSunRaysPso_.get());
					enc->setFragmentSamplerState(ppLinearSampler_.get(), 0);
					enc->setFragmentTexture(hdrColorTarget_.get(), 0);
					enc->setFragmentBytes(&srp, sizeof(srp), 0);
					enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
					enc->endEncoding();
					enc->release();
				}
			}
		}
	}

	// -----------------------------------------------------------------------
	// 6. DOF (bokeh blur, blended by backEnd.refdef.blurFactor or r_dof cvar)
	// -----------------------------------------------------------------------
	{
		float dofFactor = (r_dof_ && r_dof_->integer) ? r_dof_->value : 0.0f;
		// Also support the engine's blurFactor from the scene camera refdef.
		if (sceneCamera_.valid && (sceneCamera_.refdef.rdflags & RDF_NOWORLDMODEL) == 0) {
			// blurFactor isn't in refdef_t by default; skip if not available.
		}
		if (dofFactor > 0.001f && depthTexture_ && bloomTarget_[0] && ppDofBlurPso_) {
			struct BokehParams {
				float blurFactor;
				float _pad[3];
			} bp{ dofFactor };
			doRenderPass(ppDofBlurPso_.get(),
			             hdrColorTarget_.get(), depthTexture_.get(), nullptr,
			             bloomTarget_[0].get(),
			             MTL::LoadActionDontCare,
			             &bp, sizeof(bp));

			// Blend DOF result onto drawable (src=alpha, dst=1-alpha).
			MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
			rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
			rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
			rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
			auto* enc = currentCommandBuffer_->renderCommandEncoder(rpd);
			rpd->release();
			if (enc) {
				// Use passthrough with additive disabled — need alpha blend.
				// Reuse ppAdditivePso_ with src=One, dst=One isn't correct for DOF;
				// for now use a simple alpha-blended overlay if we have a suitable PSO.
				// Without a separate DOF composite PSO we fall back to passthrough.
				enc->setRenderPipelineState(ppPassthroughPso_ ? ppPassthroughPso_.get() : ppAdditivePso_.get());
				enc->setFragmentSamplerState(ppLinearSampler_.get(), 0);
				enc->setFragmentTexture(bloomTarget_[0].get(), 0);
				enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
				enc->endEncoding();
				enc->release();
			}
		}
	}

create_2d_encoder:
	// -----------------------------------------------------------------------
	// Create the 2-D encoder targeting the drawable for UI / 2D draws.
	// -----------------------------------------------------------------------
	{
		MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
		rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
		rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
		rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
		// No depth attachment for 2D.
		currentRenderEncoder_ = currentCommandBuffer_->renderCommandEncoder(rpd);
		rpd->release();
		if (currentRenderEncoder_) {
			MetalStateCache::Instance().resetEncoder(currentRenderEncoder_);
			currentRenderEncoder_->setFrontFacingWinding(MTL::WindingCounterClockwise);
			currentRenderEncoder_->setCullMode(MTL::CullModeNone);
		}
	}
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

	if (ext && !Q_stricmp(ext, ".mdr")) {
		// Load MDR skeletal model
		result = MetalModel_RegisterMDR(name, model, ri_);
	} else if (ext && !Q_stricmp(ext, ".iqm")) {
		// Load IQM skeletal model
		result = MetalModel_RegisterIQM(name, model, ri_);
	} else if (ext && !Q_stricmp(ext, ".md3")) {
		// Load MD3 model
		result = MetalModel_RegisterMD3(name, model, ri_);
	} else {
		// Try MD3 as default if no extension
		char namebuf[MAX_QPATH];
		Com_sprintf(namebuf, sizeof(namebuf), "%s.md3", name);
		result = MetalModel_RegisterMD3(namebuf, model, ri_);

		if (!result) {
			// Try MDR if MD3 fails
			Com_sprintf(namebuf, sizeof(namebuf), "%s.mdr", name);
			result = MetalModel_RegisterMDR(namebuf, model, ri_);
		}

		if (!result) {
			// Try IQM if MDR fails
			Com_sprintf(namebuf, sizeof(namebuf), "%s.iqm", name);
			result = MetalModel_RegisterIQM(namebuf, model, ri_);
		}

		if (!result) {
			ri_.Printf(PRINT_WARNING, "MetalRenderer::registerModel: couldn't load %s\n", name);
			model->type = MetalModelType::BAD;
		}
	}

	if (result) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: loaded model '%s' as handle %d\n", name, handle);

		if (model->type == MetalModelType::MDR) {
			// Register shaders embedded in MDR surfaces
			registerMDRShaders(model);
		} else if (model->type == MetalModelType::IQM) {
			// Register shaders embedded in IQM surfaces
			registerIQMShaders(model);
		} else {
			// Create GPU buffers for MD3 models
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
	// Run post-processing before any 2D draw so the UI is never tonemapped.
	ensurePostProcessed();
	
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

	// Build quad instance data — must match ui_2d.metal QuadInstance layout exactly
	struct QuadInstance {
		float rect[4];      // x, y, w, h in NDC
		float texCoords[4]; // s1, t1, s2, t2
		float color[4];     // RGBA
		float pivot[2];     // rotation pivot in NDC (unused when rotation == 0)
		float rotation;     // rotation angle in radians
		float _pad;
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
	instance.pivot[0] = 0.0f;
	instance.pivot[1] = 0.0f;
	instance.rotation = 0.0f;
	instance._pad = 0.0f;

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

// Helper shared by drawRotatePic and drawRotatePic2 — submits a rotated quad instance.
// pivotNdcX/pivotNdcY specify the NDC-space rotation pivot.
void MetalRenderer::drawRotatePicImpl(float x, float y, float w, float h,
                                      float s1, float t1, float s2, float t2,
                                      float degrees, qhandle_t shader,
                                      float pivotNdcX, float pivotNdcY) {
	submitScene();
	ensureSceneRendered();
	// Run post-processing before any 2D draw so the UI is never tonemapped.
	ensurePostProcessed();

	if (!device_ || !currentRenderEncoder_) {
		return;
	}

	TextureManager* textureManager = ensureTextureManager();
	if (!textureManager) {
		return;
	}

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

	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipeline2D_.get());
	if (depthState2D_) {
		currentRenderEncoder_->setDepthStencilState(depthState2D_.get());
	}
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

	float ndcX = (x * 2.0f / config_.vidWidth) - 1.0f;
	float ndcY = 1.0f - (y * 2.0f / config_.vidHeight);
	float ndcW = w * 2.0f / config_.vidWidth;
	float ndcH = h * 2.0f / config_.vidHeight;

	// Must match ui_2d.metal QuadInstance layout exactly
	struct QuadInstance {
		float rect[4];
		float texCoords[4];
		float color[4];
		float pivot[2];
		float rotation;
		float _pad;
	};

	QuadInstance instance;
	instance.rect[0] = ndcX;
	instance.rect[1] = ndcY;
	instance.rect[2] = ndcW;
	instance.rect[3] = -ndcH;
	instance.texCoords[0] = s1;
	instance.texCoords[1] = t1;
	instance.texCoords[2] = s2;
	instance.texCoords[3] = t2;
	instance.color[0] = currentColor_[0];
	instance.color[1] = currentColor_[1];
	instance.color[2] = currentColor_[2];
	instance.color[3] = currentColor_[3];
	instance.pivot[0] = pivotNdcX;
	instance.pivot[1] = pivotNdcY;
	instance.rotation = degrees * (float)(M_PI / 180.0);
	instance._pad = 0.0f;

	MTL::Viewport vp;
	vp.originX = 0.0;
	vp.originY = 0.0;
	vp.width = static_cast<double>(config_.vidWidth);
	vp.height = static_cast<double>(config_.vidHeight);
	vp.znear = 0.0;
	vp.zfar = 1.0;
	currentRenderEncoder_->setViewport(vp);

	currentRenderEncoder_->setVertexBytes(&instance, sizeof(instance), 0);
	MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, texture);
	MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler2D_.get());
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

	currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, NS::UInteger(0), NS::UInteger(4), NS::UInteger(1));
}

// Rotates around the top-left corner (x, y).
void MetalRenderer::drawRotatePic(float x, float y, float w, float h,
                                  float s1, float t1, float s2, float t2,
                                  float degrees, qhandle_t shader) {
	float pivotNdcX = (x * 2.0f / config_.vidWidth) - 1.0f;
	float pivotNdcY = 1.0f - (y * 2.0f / config_.vidHeight);
	drawRotatePicImpl(x, y, w, h, s1, t1, s2, t2, degrees, shader, pivotNdcX, pivotNdcY);
}

// Rotates around the center of the quad.
void MetalRenderer::drawRotatePic2(float x, float y, float w, float h,
                                   float s1, float t1, float s2, float t2,
                                   float degrees, qhandle_t shader) {
	float ndcX = (x * 2.0f / config_.vidWidth) - 1.0f;
	float ndcY = 1.0f - (y * 2.0f / config_.vidHeight);
	float ndcW = w * 2.0f / config_.vidWidth;
	float ndcH = h * 2.0f / config_.vidHeight;
	float pivotNdcX = ndcX + ndcW * 0.5f;
	float pivotNdcY = ndcY - ndcH * 0.5f;
	drawRotatePicImpl(x, y, w, h, s1, t1, s2, t2, degrees, shader, pivotNdcX, pivotNdcY);
}

void MetalRenderer::beginFrame(stereoFrame_t stereoFrame) {
	sceneReady_ = false;
	sceneDispatched_ = false;

	// Advance the scene frame counter so submitScene() can detect the new frame.
	MetalScene_BeginFrame();

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

	// Decide whether to render 3D scene into an HDR intermediate target.
	usingHDRRenderPath_ = (r_hdr_ && r_hdr_->integer) ? true : false;
	postProcessingDone_ = false;

	MTL::Texture* colorTarget = nullptr;
	if (usingHDRRenderPath_) {
		ensureHDRTargets(config_.vidWidth, config_.vidHeight);
		colorTarget = hdrColorTarget_.get();
	}
	if (!colorTarget) {
		// HDR disabled or allocation failed — render directly to drawable.
		usingHDRRenderPath_ = false;
		colorTarget = currentDrawable_->texture();
	}

	rpd->colorAttachments()->object(0)->setTexture(colorTarget);
	rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
	rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
	rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

	ensureDepthTexture(config_.vidWidth, config_.vidHeight);
	if (depthTexture_) {
		MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = rpd->depthAttachment();
		depthAttachment->setTexture(depthTexture_.get());
		depthAttachment->setLoadAction(MTL::LoadActionClear);
		depthAttachment->setClearDepth(1.0);
		// Store depth so SSAO can sample it after the scene pass.
		bool needDepthForSSAO = usingHDRRenderPath_ && r_ssao_ && r_ssao_->integer;
		depthAttachment->setStoreAction(needDepthForSSAO
		                                ? MTL::StoreActionStore
		                                : MTL::StoreActionDontCare);
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

	// Render lens flares on top of the 3-D scene, before ending the encoder.
	if (r_flares_ && r_flares_->integer) {
		RB_AddDlightFlares();
		RB_RenderFlares();
	}

	// End the 3D/HDR encoder and run post-processing.
	// If 2D draws already happened this frame, this is a no-op.
	ensurePostProcessed();

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

	// updateCamera must run first so the view frustum is available for PVS traversal.
	updateCamera(scene);
	processEntities(scene);
	processPolys(scene);
	processLights(scene);
	polyVertexBufferDirty_ = true;
	lightBufferDirty_ = true;
}

void MetalRenderer::processEntities(const MetalSceneState& scene) {
	// Process only the entities from the world scene (saved by RE_RenderScene)
	// Ignore UI scene entities (those were discarded)
	const int firstEntity = scene.worldSceneFirstEntity;
	const int numEntities = scene.worldSceneNumEntities;

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
	if (!(scene.refdef.rdflags & RDF_NOWORLDMODEL)) {
		markWorldSurfaces();
		appendWorldGeometry();
	}
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

	// Render portal/mirror views first (before main scene)
	if (!renderPortalViews()) {
		return;
	}

	// Compute per-object shadow map state and run caster passes (offscreen depth renders).
	// Must happen after portal views and before main scene draws so the shadow depth
	// textures are ready when the receiver overlay is composited later.
	computePshadows();
	if (numPshadows_ > 0 && ensurePshadowResources()) {
		// End the current (empty) main encoder to free it for the offscreen passes.
		MTL::RenderCommandEncoder* savedEncoder = currentRenderEncoder_;
		savedEncoder->endEncoding();
		MetalStateCache::Instance().resetEncoder(nullptr);
		currentRenderEncoder_ = nullptr;

		// Render each shadow's caster geometry into its depth texture.
		for (int pi = 0; pi < numPshadows_; pi++) {
			renderPshadowCasterPass(pi);
		}

		// Recreate the main encoder.  Use LoadActionLoad on colour to preserve
		// the black clear from beginFrame, and LoadActionClear on depth so the
		// main scene gets a fresh depth buffer.
		resumeMainEncoder(savedEncoder);
		if (!currentRenderEncoder_) {
			return;
		}
		configureSceneViewport(sceneCamera_.refdef);
	}

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

	// PShadow receiver overlay: alpha-blend a dark shadow on top of surfaces
	// that lie within the shadow footprint of a caster entity.
	if (numPshadows_ > 0) {
		renderPshadowReceiverPasses();
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

	// Set greyscale
	sceneUniforms_.greyscale = r_greyscale_ ? r_greyscale_->value : 0.0f;

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

// ============================================================================
// Cubemap helper implementations
// ============================================================================

/*
 * createSolidColorCubemap – create a Metal TextureTypeCube with all six
 * 1×1 faces set to (r, g, b, 1).  Used as the per-probe fallback when no
 * pre-baked DDS file exists.
 */
MTL::Texture* MetalRenderer::createSolidColorCubemap(float r, float g, float b) {
	if (!device_) return nullptr;

	byte pixel[4] = {
		static_cast<byte>(std::min(255, static_cast<int>(r * 255.0f))),
		static_cast<byte>(std::min(255, static_cast<int>(g * 255.0f))),
		static_cast<byte>(std::min(255, static_cast<int>(b * 255.0f))),
		255u
	};

	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
	desc->setTextureType(MTL::TextureTypeCube);
	desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	desc->setWidth(1);
	desc->setHeight(1);
	desc->setMipmapLevelCount(1);
	desc->setUsage(MTL::TextureUsageShaderRead);
	desc->setStorageMode(MTL::StorageModeShared);

	MTL::Texture* tex = device_->newTexture(desc);
	desc->release();
	if (!tex) return nullptr;

	MTL::Region region = MTL::Region::Make2D(0, 0, 1, 1);
	for (NS::UInteger face = 0; face < 6; ++face) {
		tex->replaceRegion(region, 0, face, pixel, 4, 0);
	}
	return tex;
}

/*
 * loadCubemapDDS – attempt to load a DDS cubemap file from the virtual
 * file system.  Supports the DXT1/DXT3/DXT5 and RGBA8 variants that
 * ioq3's cubemap baker writes.  Returns nullptr on failure.
 *
 * DDS cubemap layout:  6 faces × numMips mip levels, +X/-X/+Y/-Y/+Z/-Z.
 * Metal's TextureTypeCube uses the same face order for slice indices 0–5.
 */
MTL::Texture* MetalRenderer::loadCubemapDDS(const char* filename) {
	if (!device_ || !filename) return nullptr;

	byte* buf = nullptr;
	const int len = ri_.FS_ReadFile(filename, reinterpret_cast<void**>(&buf));
	if (len <= 0 || !buf) return nullptr;

	// -----------------------------------------------------------------------
	// Minimal DDS structures (matching tr_image_dds.cpp)
	// -----------------------------------------------------------------------
	struct DDSPixFmt {
		uint32_t size, flags, fourCC, rgbBitCount;
		uint32_t rMask, gMask, bMask, aMask;
	};
	struct DDSHdr {
		uint32_t size, flags, height, width, pitchOrSize, depth, numMips;
		uint32_t reserved1[11];
		DDSPixFmt pf;
		uint32_t caps, caps2, caps3, caps4, reserved2;
	};

	static constexpr uint32_t DDS_CAPS2_CUBEMAP = 0xFE00u;
	static constexpr uint32_t DDS_PF_FOURCC     = 0x4u;
	static constexpr uint32_t DDS_FLAGS_MIPCOUNT = 0x20000u;

	auto makeFCC = [](const char* s) -> uint32_t {
		return static_cast<uint32_t>(s[0])        |
		       (static_cast<uint32_t>(s[1]) <<  8) |
		       (static_cast<uint32_t>(s[2]) << 16) |
		       (static_cast<uint32_t>(s[3]) << 24);
	};

	if (len < 4 + static_cast<int>(sizeof(DDSHdr))) { ri_.FS_FreeFile(buf); return nullptr; }
	if (memcmp(buf, "DDS ", 4) != 0)                { ri_.FS_FreeFile(buf); return nullptr; }

	const DDSHdr* hdr = reinterpret_cast<const DDSHdr*>(buf + 4);
	if (!(hdr->caps2 & DDS_CAPS2_CUBEMAP))          { ri_.FS_FreeFile(buf); return nullptr; }

	const byte* data = buf + 4 + sizeof(DDSHdr);
	int remaining    = len - 4 - static_cast<int>(sizeof(DDSHdr));

	const int width   = static_cast<int>(hdr->width);
	const int height  = static_cast<int>(hdr->height);
	const int numMips = (hdr->flags & DDS_FLAGS_MIPCOUNT) ? static_cast<int>(hdr->numMips) : 1;

	// Map FourCC → Metal pixel format and block size (bytes per 4×4 block)
	uint32_t fcc   = (hdr->pf.flags & DDS_PF_FOURCC) ? hdr->pf.fourCC : 0;
	MTL::PixelFormat fmt = MTL::PixelFormatRGBA8Unorm;
	size_t blockSize = 0; // 0 = uncompressed

	if      (fcc == makeFCC("DXT1")) { fmt = MTL::PixelFormatBC1_RGBA;    blockSize =  8; }
	else if (fcc == makeFCC("DXT3")) { fmt = MTL::PixelFormatBC2_RGBA;    blockSize = 16; }
	else if (fcc == makeFCC("DXT5")) { fmt = MTL::PixelFormatBC3_RGBA;    blockSize = 16; }
	else if (fcc == makeFCC("ATI1") || fcc == makeFCC("BC4U")) { fmt = MTL::PixelFormatBC4_RUnorm;  blockSize = 8;  }
	else if (fcc == makeFCC("ATI2") || fcc == makeFCC("BC5U")) { fmt = MTL::PixelFormatBC5_RGUnorm; blockSize = 16; }
	else if (fcc != 0) { ri_.FS_FreeFile(buf); return nullptr; } // unknown FourCC

	// Helper: byte size of one mip level
	auto mipBytes = [&](int w, int h) -> size_t {
		if (blockSize > 0) {
			size_t bw = (static_cast<size_t>(w) + 3) / 4;
			size_t bh = (static_cast<size_t>(h) + 3) / 4;
			return bw * bh * blockSize;
		}
		return static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
	};
	auto bytesPerRow = [&](int w) -> size_t {
		if (blockSize > 0) return ((static_cast<size_t>(w) + 3) / 4) * blockSize;
		return static_cast<size_t>(w) * 4;
	};

	// Create Metal cubemap texture
	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
	desc->setTextureType(MTL::TextureTypeCube);
	desc->setPixelFormat(fmt);
	desc->setWidth(static_cast<NS::UInteger>(width));
	desc->setHeight(static_cast<NS::UInteger>(height));
	desc->setMipmapLevelCount(static_cast<NS::UInteger>(numMips));
	desc->setUsage(MTL::TextureUsageShaderRead);
	desc->setStorageMode(MTL::StorageModeShared);

	MTL::Texture* tex = device_->newTexture(desc);
	desc->release();

	if (!tex) { ri_.FS_FreeFile(buf); return nullptr; }

	// Upload each face's mip chain
	const byte* src = data;
	for (NS::UInteger face = 0; face < 6; ++face) {
		int mipW = width, mipH = height;
		for (int mip = 0; mip < numMips; ++mip) {
			size_t mb  = mipBytes(mipW, mipH);
			size_t bpr = bytesPerRow(mipW);
			if (static_cast<int>(mb) > remaining) break;

			MTL::Region region = MTL::Region::Make2D(0, 0,
			    static_cast<NS::UInteger>(mipW),
			    static_cast<NS::UInteger>(mipH));
			tex->replaceRegion(region, static_cast<NS::UInteger>(mip), face, src, bpr, 0);

			src       += mb;
			remaining -= static_cast<int>(mb);
			mipW = std::max(1, mipW / 2);
			mipH = std::max(1, mipH / 2);
		}
	}

	ri_.FS_FreeFile(buf);

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: Loaded cubemap DDS '%s' (%dx%d, %d mips)\n",
		           filename, width, height, numMips);
	}
	return tex;
}

/*
 * getOrCreateFallbackCubemap – return (or lazily create) a permanent 1×1
 * black cubemap used as the null binding for texture slot 3 when no probe
 * is assigned to a surface.
 */
MTL::Texture* MetalRenderer::getOrCreateFallbackCubemap() {
	if (!fallbackCubemapTexture_) {
		fallbackCubemapTexture_.reset(createSolidColorCubemap(0.0f, 0.0f, 0.0f));
	}
	return fallbackCubemapTexture_.get();
}

/*
 * loadWorldCubemaps – load or generate cubemap textures for every probe
 * that was found in the entity string, then assign a per-surface cubemap
 * index to each world packet using the nearest-probe selection from
 * R_CubemapForPoint (same algorithm as GL2's R_AssignCubemapsToWorldSurfaces).
 *
 * Slot 0 in worldCubemapTextures_ is always the fallback (black).
 * Slots 1..N correspond to probes 0..N-1 (1-based as returned by
 * R_CubemapForPoint).
 *
 * For each probe we first try to load a pre-baked DDS file from
 *   cubemaps/<mapBaseName>/<NNN>.dds
 * and fall back to a solid-colour cubemap sampled from the light grid at
 * the probe's world position.
 */
void MetalRenderer::loadWorldCubemaps(const std::string& requestedName) {
	worldCubemapTextures_.clear();

	const int numProbes = R_GetNumCubemapProbes();
	if (numProbes <= 0) return;

	// Derive the map base name: "maps/q3dm1.bsp" → "q3dm1"
	std::string mapBaseName = requestedName;
	{
		const size_t slash = mapBaseName.rfind('/');
		if (slash != std::string::npos) mapBaseName = mapBaseName.substr(slash + 1);
		const size_t dot = mapBaseName.rfind('.');
		if (dot   != std::string::npos) mapBaseName = mapBaseName.substr(0, dot);
	}

	// Slot 0 = always-available fallback (black, useCubemap disabled)
	worldCubemapTextures_.push_back(MetalPtr<MTL::Texture>(createSolidColorCubemap(0.0f, 0.0f, 0.0f)));

	// Slots 1..numProbes = per-probe textures
	for (int i = 0; i < numProbes; ++i) {
		MTL::Texture* cubeTex = nullptr;

		// Try pre-baked DDS (matches GL2's R_LoadCubemaps path)
		char ddsPath[MAX_QPATH];
		Com_sprintf(ddsPath, sizeof(ddsPath), "cubemaps/%s/%03d.dds",
		            mapBaseName.c_str(), i);
		cubeTex = loadCubemapDDS(ddsPath);

		if (!cubeTex) {
			// Fallback: sample the light grid at the probe origin to get an
			// approximate ambient colour and create a solid-colour cubemap.
			vec3_t probeOrigin;
			R_GetCubemapProbeOrigin(i, probeOrigin);

			vec3_t ambient = {0.5f, 0.5f, 0.5f};
			vec3_t directed, lightDir;
			if (R_LightForPoint(probeOrigin, ambient, directed, lightDir)) {
				// Average ambient + directed, normalised from 0-255 to 0-1
				for (int c = 0; c < 3; ++c)
					ambient[c] = (ambient[c] + directed[c]) / 510.0f;
			}
			cubeTex = createSolidColorCubemap(ambient[0], ambient[1], ambient[2]);

			if (ri_.Printf) {
				ri_.Printf(PRINT_DEVELOPER,
				           "Metal: Probe %d: no DDS found, using solid-color cubemap "
				           "(%.2f, %.2f, %.2f)\n",
				           i, ambient[0], ambient[1], ambient[2]);
			}
		}

		worldCubemapTextures_.push_back(MetalPtr<MTL::Texture>(cubeTex));
	}

	// Assign the nearest probe index to every world surface packet.
	// Uses the position of the first vertex as the surface representative point,
	// matching GL2's R_AssignCubemapsToWorldSurfaces which averages the
	// surface bounds (we keep it simpler for performance).
	for (auto& packet : worldPacketTemplate_) {
		if (packet.firstVertex < 0 ||
		    packet.vertexCount <= 0 ||
		    static_cast<size_t>(packet.firstVertex) >= worldVertexTemplate_.size()) {
			continue;
		}
		const MetalPolyVertex& v0 = worldVertexTemplate_[packet.firstVertex];
		vec3_t surfCenter;
		VectorSet(surfCenter, v0.xyz[0], v0.xyz[1], v0.xyz[2]);
		packet.cubemapIndex = R_CubemapForPoint(surfCenter); // 0 = none, 1+ = probe
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL,
		           "Metal: Loaded %d cubemap probe texture(s) for '%s'\n",
		           numProbes, mapBaseName.c_str());
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

		// Probe for companion normal-map and specular-map files alongside the primary diffuse.
		// Skip stages that use the lightmap or white-image placeholder.
		if (runtime.primaryStageImage != 0 && !stageInfo.usesLightmap && !stageInfo.usesWhiteImage
		    && !stageInfo.imagePaths.empty()) {
			const std::string basePath = normalizePath(stageInfo.imagePaths[0]);
			if (!basePath.empty()) {
				TextureManager* tm = ensureTextureManager();
				auto probeMap = [&](const std::initializer_list<const char*>& suffixes) -> qhandle_t {
					for (const char* suffix : suffixes) {
						std::string candidate = basePath + suffix;
						if (assetExists(candidate.c_str()) && tm) {
							return tm->registerShader(candidate.c_str(), true);
						}
					}
					return 0;
				};
				runtime.normalMapHandle   = probeMap({"_n.tga",    "_norm.tga",     "_normal.tga"});
				runtime.specularMapHandle = probeMap({"_s.tga",    "_spec.tga",     "_specular.tga"});
			}
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

MetalRenderer::TCModParams MetalRenderer::computeTCModParams(const MetalShaderStageInfo* stageInfo, float timeSeconds, const refEntity_t* entity) const {
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
		
		ComputeTCModMatrix(mod, timeSeconds, matrix, turbAmp, turbPhase, entity);
		
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

	if (!sceneClampSampler_) {
		MTL::SamplerDescriptor* sampDesc = MTL::SamplerDescriptor::alloc()->init();
		sampDesc->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampDesc->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampDesc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampDesc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		sceneClampSampler_.reset(device_->newSamplerState(sampDesc));
		sampDesc->release();
	}

	return sceneSampler_.get() != nullptr &&
	       sceneClampSampler_.get() != nullptr &&
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

bool MetalRenderer::ensureShadowPipeline() {
	if (shadowPipeline_) {
		return true;
	}
	if (!device_ || !depthTexture_) {
		return false;
	}

	if (!sceneLibrary_) {
		sceneLibrary_.reset(device_->newDefaultLibrary());
		if (!sceneLibrary_) {
			return false;
		}
	}

	if (!shadowVertexFunction_) {
		NS::String* name = NS::String::string("vertex_shadow", NS::UTF8StringEncoding);
		shadowVertexFunction_.reset(sceneLibrary_->newFunction(name));
		if (!shadowVertexFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: Failed to load vertex_shadow shader function\n");
			}
			return false;
		}
	}

	if (!shadowFragmentFunction_) {
		NS::String* name = NS::String::string("fragment_shadow", NS::UTF8StringEncoding);
		shadowFragmentFunction_.reset(sceneLibrary_->newFunction(name));
		if (!shadowFragmentFunction_) {
			if (ri_.Printf) {
				ri_.Printf(PRINT_WARNING, "Metal: Failed to load fragment_shadow shader function\n");
			}
			return false;
		}
	}

	// Vertex descriptor: one attribute - float3 position, stride 12 bytes
	if (!shadowVertexDescriptor_) {
		shadowVertexDescriptor_.reset(MTL::VertexDescriptor::alloc()->init());

		MTL::VertexAttributeDescriptor* attr0 = shadowVertexDescriptor_->attributes()->object(0);
		attr0->setFormat(MTL::VertexFormatFloat3);
		attr0->setOffset(0);
		attr0->setBufferIndex(0);

		MTL::VertexBufferLayoutDescriptor* layout0 = shadowVertexDescriptor_->layouts()->object(0);
		layout0->setStride(sizeof(float) * 3);
		layout0->setStepRate(1);
		layout0->setStepFunction(MTL::VertexStepFunctionPerVertex);
	}

	// Pipeline: dark translucent, SrcAlpha / OneMinusSrcAlpha blending
	NS::Error* error = nullptr;
	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();

	pd->setVertexFunction(shadowVertexFunction_.get());
	pd->setFragmentFunction(shadowFragmentFunction_.get());
	pd->setVertexDescriptor(shadowVertexDescriptor_.get());

	MTL::RenderPipelineColorAttachmentDescriptor* ca = pd->colorAttachments()->object(0);
	ca->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	ca->setBlendingEnabled(true);
	ca->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	ca->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	ca->setRgbBlendOperation(MTL::BlendOperationAdd);
	ca->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	ca->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	ca->setAlphaBlendOperation(MTL::BlendOperationAdd);

	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	shadowPipeline_.reset(device_->newRenderPipelineState(pd, &error));
	pd->release();

	if (!shadowPipeline_) {
		if (error && ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to create shadow pipeline: %s\n",
				error->localizedDescription()->utf8String());
			error->release();
		}
		return false;
	}

	// Depth state: depth test enabled (LessEqual), depth write disabled
	// so multiple shadow passes don't z-fight with each other.
	if (!shadowDepthState_) {
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthWriteEnabled(false);
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
		shadowDepthState_.reset(device_->newDepthStencilState(depthDesc));
		depthDesc->release();
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_ALL, "Metal: Created projection shadow pipeline\n");
	}
	return true;
}

//=============================================================================
// PER-OBJECT SHADOW MAPS (PSHADOW)
//=============================================================================

// Collect shadow-casting entities for this frame and compute their light params.
// Ported from renderergl2/tr_main.c R_AddPshadowDrawSurfs.
void MetalRenderer::computePshadows() {
	numPshadows_ = 0;

	// Only active when r_shadows >= 2 (per-object shadows mode).
	if (!r_shadows_ || r_shadows_->integer < 2) {
		return;
	}
	if (drawPackets_.empty()) {
		return;
	}

	const float pshadowDist = r_pshadowDist_ ? r_pshadowDist_->value : 512.0f;
	const float* vieworg   = sceneCamera_.refdef.vieworg;
	const float* viewaxis0 = sceneCamera_.refdef.viewaxis[0]; // forward

	int numCalc = 0;
	pshadow_t calc[MAX_CALC_PSHADOWS];

	for (int ei = 0; ei < (int)drawPackets_.size(); ei++) {
		const refEntity_t& ent = drawPackets_[ei].entity;

		if (ent.reType != RT_MODEL)              continue;
		if (ent.renderfx & RF_NOSHADOW)          continue;

		if (ent.hModel <= 0 || (size_t)ent.hModel >= models_.size()) continue;
		MetalModel* model = models_[ent.hModel];
		if (!model || model->type != MetalModelType::MD3) continue;
		int lod = 0;
		if (lod >= model->numLods || !model->lods[lod]) continue;
		MetalModelLOD* lodData = model->lods[lod];
		if (lodData->frames.empty()) continue;

		int fIdx = ent.frame;
		if (fIdx < 0) fIdx = 0;
		if (fIdx >= (int)lodData->frames.size()) fIdx = (int)lodData->frames.size() - 1;
		float radius = lodData->frames[fIdx].radius;
		if (radius <= 0.0f) continue;

		if (ent.nonNormalizedAxes) {
			float axLen = sqrtf(ent.axis[0][0]*ent.axis[0][0] +
			                    ent.axis[0][1]*ent.axis[0][1] +
			                    ent.axis[0][2]*ent.axis[0][2]);
			radius *= axLen;
		}

		// Cull entities behind the camera by more than pshadowDist.
		float diff[3] = { ent.origin[0]-vieworg[0], ent.origin[1]-vieworg[1], ent.origin[2]-vieworg[2] };
		float dotFwd  = diff[0]*viewaxis0[0] + diff[1]*viewaxis0[1] + diff[2]*viewaxis0[2];
		if (dotFwd < -(pshadowDist)) {
			continue;
		}

		float distSq = diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2];

		// Build initial pshadow candidate.
		pshadow_t s = {};
		s.numEntities = 1;
		s.entityNums[0] = ei;
		VectorCopy(ent.origin, s.entityOrigins[0]);
		s.entityRadiuses[0] = radius;
		s.viewRadius         = radius;
		s.lightRadius        = pshadowDist;
		VectorCopy(ent.origin, s.viewOrigin);
		s.sort = (radius * radius > 0.0f) ? (distSq / (radius * radius)) : distSq;

		// Insert into sorted list (ascending sort).
		if (numCalc < MAX_CALC_PSHADOWS) {
			int ins = numCalc;
			for (int k = 0; k < numCalc; k++) {
				if (calc[k].sort > s.sort) { ins = k; break; }
			}
			for (int k = numCalc; k > ins; k--) calc[k] = calc[k-1];
			calc[ins] = s;
			numCalc++;
		} else if (s.sort < calc[numCalc-1].sort) {
			calc[numCalc-1] = s;
			// Re-sort the last element into place.
			for (int k = numCalc-1; k > 0 && calc[k].sort < calc[k-1].sort; k--) {
				pshadow_t tmp = calc[k]; calc[k] = calc[k-1]; calc[k-1] = tmp;
			}
		}
	}

	// Merge overlapping shadow spheres (up to 8 entities per shadow map).
	for (int i = 0; i < numCalc; i++) {
		pshadow_t* ps1 = &calc[i];
		for (int j = i + 1; j < numCalc && ps1->numEntities < 8; j++) {
			pshadow_t* ps2 = &calc[j];
			bool touch = false;
			for (int k = 0; k < ps1->numEntities && !touch; k++) {
				touch = SpheresIntersect(ps1->entityOrigins[k], ps1->entityRadiuses[k],
				                         ps2->viewOrigin, ps2->viewRadius);
			}
			if (touch) {
				vec3_t newOrig; float newRad;
				BoundingSphereOfSpheres(ps1->viewOrigin, ps1->viewRadius,
				                         ps2->viewOrigin, ps2->viewRadius,
				                         newOrig, &newRad);
				VectorCopy(newOrig, ps1->viewOrigin);
				ps1->viewRadius = newRad;

				int ne = ps1->numEntities;
				ps1->entityNums[ne]      = ps2->entityNums[0];
				VectorCopy(ps2->entityOrigins[0], ps1->entityOrigins[ne]);
				ps1->entityRadiuses[ne]  = ps2->entityRadiuses[0];
				ps1->numEntities++;

				for (int k = j; k < numCalc-1; k++) calc[k] = calc[k+1];
				j--;
				numCalc--;
			}
		}
	}

	if (numCalc > MAX_DRAWN_PSHADOWS) numCalc = MAX_DRAWN_PSHADOWS;
	numPshadows_ = numCalc;

	// Compute light direction and axes for each final shadow.
	for (int i = 0; i < numPshadows_; i++) {
		pshadow_t* shadow = &calc[i];

		vec3_t ambientLight, directedLight, lightDir;
		VectorSet(lightDir, 0.57735f, 0.57735f, 0.57735f);
		R_LightForPoint(shadow->viewOrigin, ambientLight, directedLight, lightDir);
		// Normalise just in case R_LightForPoint returned a degenerate vector.
		float ldLen = VectorLength(lightDir);
		if (ldLen < 0.5f) VectorSet(lightDir, 0.0f, 0.0f, 1.0f);
		else VectorScale(lightDir, 1.0f / ldLen, lightDir);

		if (shadow->viewRadius * 3.0f > shadow->lightRadius) {
			shadow->lightRadius = shadow->viewRadius * 3.0f;
		}

		// Light origin is above the entity cluster, along the light direction.
		VectorMA(shadow->viewOrigin, shadow->viewRadius, lightDir, shadow->lightOrigin);

		// Build orthonormal light-space axes.
		// axis[0] = forward from light toward scene (opposite of lightDir).
		VectorScale(lightDir, -1.0f, shadow->lightViewAxis[0]);
		vec3_t up = {0.0f, 0.0f, -1.0f};
		if (fabsf(DotProduct(up, shadow->lightViewAxis[0])) > 0.9f) {
			VectorSet(up, -1.0f, 0.0f, 0.0f);
		}
		CrossProduct(shadow->lightViewAxis[0], up, shadow->lightViewAxis[1]);
		VectorNormalize(shadow->lightViewAxis[1]);
		CrossProduct(shadow->lightViewAxis[0], shadow->lightViewAxis[1], shadow->lightViewAxis[2]);

		// Cull plane faces toward the light; surfaces on its back are culled.
		VectorCopy(shadow->lightViewAxis[0], shadow->cullPlane.normal);
		shadow->cullPlane.dist = DotProduct(shadow->cullPlane.normal, shadow->lightOrigin);
		shadow->cullPlane.type = PlaneTypeForNormal(shadow->cullPlane.normal);
		SetPlaneSignbits(&shadow->cullPlane);

		pshadows_[i] = *shadow;
	}
}

// Lazily create all PShadow pipeline objects.
bool MetalRenderer::ensurePshadowResources() {
	if (pshadowCasterPipeline_ && pshadowRecvWorldPipeline_ && pshadowRecvModelPipeline_) {
		return true;
	}
	if (!device_) return false;

	// Ensure prerequisite resources are available.
	if (!ensureSceneShaderResources() || !sceneVertexDescriptor_) return false;
	if (!ensureModelPipeline()       || !modelVertexDescriptor_)  return false;

	if (!sceneLibrary_) {
		sceneLibrary_.reset(device_->newDefaultLibrary());
		if (!sceneLibrary_) return false;
	}

	// Load shader functions.
	auto loadFn = [&](const char* name, MetalPtr<MTL::Function>& fn) -> bool {
		if (fn) return true;
		NS::String* s = NS::String::string(name, NS::UTF8StringEncoding);
		fn.reset(sceneLibrary_->newFunction(s));
		if (!fn && ri_.Printf)
			ri_.Printf(PRINT_WARNING, "Metal PShadow: missing shader function '%s'\n", name);
		return !!fn;
	};
	if (!loadFn("vertex_pshadow_caster",         pshadowCasterVFn_))    return false;
	if (!loadFn("vertex_pshadow_receiver_world",  pshadowRecvWorldVFn_)) return false;
	if (!loadFn("vertex_pshadow_receiver_model",  pshadowRecvModelVFn_)) return false;
	if (!loadFn("fragment_pshadow_receiver",      pshadowRecvFFn_))      return false;

	// Caster vertex descriptor: only float3 position, stride 12.
	if (!pshadowCasterVD_) {
		pshadowCasterVD_.reset(MTL::VertexDescriptor::alloc()->init());
		auto* attr = pshadowCasterVD_->attributes()->object(0);
		attr->setFormat(MTL::VertexFormatFloat3);
		attr->setOffset(0);
		attr->setBufferIndex(0);
		auto* layout = pshadowCasterVD_->layouts()->object(0);
		layout->setStride(sizeof(float) * 3);
		layout->setStepRate(1);
		layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
	}

	NS::Error* err = nullptr;

	// ---- Caster pipeline (depth-only, no colour attachment) ----
	if (!pshadowCasterPipeline_) {
		MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
		pd->setVertexFunction(pshadowCasterVFn_.get());
		pd->setVertexDescriptor(pshadowCasterVD_.get());
		// No colour attachment; depth only.
		pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
		pshadowCasterPipeline_.reset(device_->newRenderPipelineState(pd, &err));
		pd->release();
		if (!pshadowCasterPipeline_) {
			if (err && ri_.Printf)
				ri_.Printf(PRINT_WARNING, "Metal PShadow: caster pipeline failed: %s\n",
				           err->localizedDescription()->utf8String());
			return false;
		}
	}

	// ---- World receiver pipeline ----
	auto makeRecvPipeline = [&](MTL::Function* vfn, MTL::VertexDescriptor* vd,
	                             MetalPtr<MTL::RenderPipelineState>& out) -> bool {
		if (out) return true;
		MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
		pd->setVertexFunction(vfn);
		pd->setFragmentFunction(pshadowRecvFFn_.get());
		pd->setVertexDescriptor(vd);
		auto* ca = pd->colorAttachments()->object(0);
		ca->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
		ca->setBlendingEnabled(true);
		ca->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
		ca->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
		ca->setRgbBlendOperation(MTL::BlendOperationAdd);
		ca->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
		ca->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
		ca->setAlphaBlendOperation(MTL::BlendOperationAdd);
		pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
		err = nullptr;
		out.reset(device_->newRenderPipelineState(pd, &err));
		pd->release();
		if (!out) {
			if (err && ri_.Printf)
				ri_.Printf(PRINT_WARNING, "Metal PShadow: receiver pipeline failed: %s\n",
				           err->localizedDescription()->utf8String());
			return false;
		}
		return true;
	};
	if (!makeRecvPipeline(pshadowRecvWorldVFn_.get(), sceneVertexDescriptor_.get(),
	                       pshadowRecvWorldPipeline_)) return false;
	if (!makeRecvPipeline(pshadowRecvModelVFn_.get(), modelVertexDescriptor_.get(),
	                       pshadowRecvModelPipeline_)) return false;

	// ---- Caster depth state: write enabled, LessEqual compare ----
	if (!pshadowCasterDepthState_) {
		MTL::DepthStencilDescriptor* dd = MTL::DepthStencilDescriptor::alloc()->init();
		dd->setDepthWriteEnabled(true);
		dd->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
		pshadowCasterDepthState_.reset(device_->newDepthStencilState(dd));
		dd->release();
	}

	// ---- Receiver depth state: no write, Always (depth buffer not preserved) ----
	if (!pshadowRecvDepthState_) {
		MTL::DepthStencilDescriptor* dd = MTL::DepthStencilDescriptor::alloc()->init();
		dd->setDepthWriteEnabled(false);
		dd->setDepthCompareFunction(MTL::CompareFunctionAlways);
		pshadowRecvDepthState_.reset(device_->newDepthStencilState(dd));
		dd->release();
	}

	if (ri_.Printf)
		ri_.Printf(PRINT_ALL, "Metal: Created pshadow pipelines\n");
	return true;
}

// Ensure a 512x512 Depth32Float shadow-map texture exists for slot i.
void MetalRenderer::ensurePshadowTexture(int i) {
	if (i < 0 || i >= MAX_DRAWN_PSHADOWS || !device_) return;
	if (pshadowMaps_[i]) return;

	MTL::TextureDescriptor* td = MTL::TextureDescriptor::texture2DDescriptor(
		MTL::PixelFormatDepth32Float,
		PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, false);
	td->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
	td->setStorageMode(MTL::StorageModePrivate);
	pshadowMaps_[i].reset(device_->newTexture(td));
}

// Render the shadow-casting entities for shadow slot shadowIdx into its
// depth-only offscreen texture.
void MetalRenderer::renderPshadowCasterPass(int shadowIdx) {
	if (!device_ || !currentCommandBuffer_) return;
	if (shadowIdx < 0 || shadowIdx >= numPshadows_) return;

	ensurePshadowTexture(shadowIdx);
	MTL::Texture* shadowMap = pshadowMaps_[shadowIdx].get();
	if (!shadowMap) return;

	const pshadow_t& shadow = pshadows_[shadowIdx];

	// Build a depth-only render pass targeting the shadow texture.
	MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
	rpd->depthAttachment()->setTexture(shadowMap);
	rpd->depthAttachment()->setLoadAction(MTL::LoadActionClear);
	rpd->depthAttachment()->setClearDepth(1.0);
	rpd->depthAttachment()->setStoreAction(MTL::StoreActionStore);

	MTL::RenderCommandEncoder* enc = currentCommandBuffer_->renderCommandEncoder(rpd);
	rpd->release();
	if (!enc) return;

	enc->setRenderPipelineState(pshadowCasterPipeline_.get());
	enc->setDepthStencilState(pshadowCasterDepthState_.get());
	enc->setCullMode(MTL::CullModeNone);

	// Build caster uniforms once per shadow.
	PShadowCasterUniforms cu{};
	cu.lightOrigin[0] = shadow.lightOrigin[0];
	cu.lightOrigin[1] = shadow.lightOrigin[1];
	cu.lightOrigin[2] = shadow.lightOrigin[2];
	cu.lightOrigin[3] = shadow.viewRadius;
	cu.lightForward[0] = shadow.lightViewAxis[0][0];
	cu.lightForward[1] = shadow.lightViewAxis[0][1];
	cu.lightForward[2] = shadow.lightViewAxis[0][2];
	cu.lightForward[3] = shadow.lightRadius;
	cu.lightRight[0] = shadow.lightViewAxis[1][0];
	cu.lightRight[1] = shadow.lightViewAxis[1][1];
	cu.lightRight[2] = shadow.lightViewAxis[1][2];
	cu.lightRight[3] = 0.0f;
	cu.lightUp[0] = shadow.lightViewAxis[2][0];
	cu.lightUp[1] = shadow.lightViewAxis[2][1];
	cu.lightUp[2] = shadow.lightViewAxis[2][2];
	cu.lightUp[3] = 0.0f;
	enc->setVertexBytes(&cu, sizeof(cu), 1);

	// Render each entity's MD3 surfaces with world-space vertex positions.
	for (int ei = 0; ei < shadow.numEntities; ei++) {
		int packetIdx = shadow.entityNums[ei];
		if (packetIdx < 0 || packetIdx >= (int)drawPackets_.size()) continue;
		const refEntity_t& ent = drawPackets_[packetIdx].entity;

		if (ent.hModel <= 0 || (size_t)ent.hModel >= models_.size()) continue;
		MetalModel* model = models_[ent.hModel];
		if (!model || model->type != MetalModelType::MD3) continue;
		int lod = 0;
		if (lod >= model->numLods || !model->lods[lod]) continue;
		MetalModelLOD* lodData = model->lods[lod];

		// Frame clamping + lerp factor (same as blob shadow).
		int oldFrame = ent.oldframe;
		int newFrame = ent.frame;
		if (oldFrame < 0) oldFrame = 0;
		if (oldFrame >= lodData->numSurfaces * 0 + (int)lodData->frames.size())
			oldFrame = (int)lodData->frames.size() - 1;
		if (newFrame < 0) newFrame = 0;
		if (newFrame >= (int)lodData->frames.size())
			newFrame = (int)lodData->frames.size() - 1;
		const float vertexLerp = 1.0f - ent.backlerp;

		// Entity model matrix (world transform).
		float modelMatrix[16];
		calculateEntityTransform(ent, modelMatrix);

		for (MetalModelSurface& surface : lodData->surfaces) {
			if (surface.numVerts <= 0 || surface.numIndexes <= 0) continue;
			if (!surface.indexBuffer) continue;

			// Clamp frame indices to this surface's frame count.
			int of = oldFrame < surface.numFrames ? oldFrame : surface.numFrames - 1;
			int nf = newFrame < surface.numFrames ? newFrame : surface.numFrames - 1;

			// Build world-space float3 positions (lerp model space → world space).
			std::vector<float> wpos(surface.numVerts * 3);
			for (int vi = 0; vi < surface.numVerts; vi++) {
				const MetalModelVertex& ov = surface.vertices[of * surface.numVerts + vi];
				const MetalModelVertex& nv = surface.vertices[nf * surface.numVerts + vi];
				float ms[3];
				ms[0] = ov.xyz[0] + vertexLerp * (nv.xyz[0] - ov.xyz[0]);
				ms[1] = ov.xyz[1] + vertexLerp * (nv.xyz[1] - ov.xyz[1]);
				ms[2] = ov.xyz[2] + vertexLerp * (nv.xyz[2] - ov.xyz[2]);
				// Apply model matrix (column-major) to get world space.
				wpos[vi*3+0] = modelMatrix[0]*ms[0]+modelMatrix[4]*ms[1]+modelMatrix[8] *ms[2]+modelMatrix[12];
				wpos[vi*3+1] = modelMatrix[1]*ms[0]+modelMatrix[5]*ms[1]+modelMatrix[9] *ms[2]+modelMatrix[13];
				wpos[vi*3+2] = modelMatrix[2]*ms[0]+modelMatrix[6]*ms[1]+modelMatrix[10]*ms[2]+modelMatrix[14];
			}

			MTL::Buffer* vbuf = device_->newBuffer(
				wpos.data(),
				wpos.size() * sizeof(float),
				MTL::ResourceStorageModeShared);
			if (!vbuf) continue;

			enc->setVertexBuffer(vbuf, 0, 0);

			MTL::Buffer* ibuf = static_cast<MTL::Buffer*>(surface.indexBuffer);
			enc->drawIndexedPrimitives(
				MTL::PrimitiveTypeTriangle,
				(NS::UInteger)surface.numIndexes,
				MTL::IndexTypeUInt32,
				ibuf, 0);

			vbuf->release();
		}
	}

	enc->endEncoding();
	enc->release();
}

// Alpha-blend a dark shadow overlay on all world and model surfaces that fall
// within the shadow footprint of each active shadow.
void MetalRenderer::renderPshadowReceiverPasses() {
	if (!currentRenderEncoder_ || !device_ || !sceneUniformBuffer_) return;

	const bool hasStaticWorld = staticWorldVertexBuffer_.get() != nullptr;
	const bool hasDynamic     = polyVertexBuffer_.get() != nullptr;

	for (int si = 0; si < numPshadows_; si++) {
		MTL::Texture* shadowMap = pshadowMaps_[si].get();
		if (!shadowMap) continue;

		const pshadow_t& shadow = pshadows_[si];

		// Build receiver uniforms.
		PShadowReceiverUniforms ru{};
		ru.lightOrigin[0] = shadow.lightOrigin[0];
		ru.lightOrigin[1] = shadow.lightOrigin[1];
		ru.lightOrigin[2] = shadow.lightOrigin[2];
		ru.lightOrigin[3] = shadow.lightRadius;
		ru.lightForward[0] = shadow.lightViewAxis[0][0];
		ru.lightForward[1] = shadow.lightViewAxis[0][1];
		ru.lightForward[2] = shadow.lightViewAxis[0][2];
		ru.lightForward[3] = 0.0f;
		// lightRight and lightUp are pre-scaled by 1/viewRadius so the UV
		// computation in the shader gives values in [-1, 1] for in-shadow pixels.
		float invVR = (shadow.viewRadius > 0.0f) ? (1.0f / shadow.viewRadius) : 1.0f;
		ru.lightRight[0] = shadow.lightViewAxis[1][0] * invVR;
		ru.lightRight[1] = shadow.lightViewAxis[1][1] * invVR;
		ru.lightRight[2] = shadow.lightViewAxis[1][2] * invVR;
		ru.lightRight[3] = 0.0f;
		ru.lightUp[0] = shadow.lightViewAxis[2][0] * invVR;
		ru.lightUp[1] = shadow.lightViewAxis[2][1] * invVR;
		ru.lightUp[2] = shadow.lightViewAxis[2][2] * invVR;
		ru.lightUp[3] = 0.0f;

		// ---- World surface receiver pass ----
		currentRenderEncoder_->setRenderPipelineState(pshadowRecvWorldPipeline_.get());
		currentRenderEncoder_->setDepthStencilState(pshadowRecvDepthState_.get());
		currentRenderEncoder_->setCullMode(MTL::CullModeNone);
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
		currentRenderEncoder_->setFragmentBytes(&ru, sizeof(ru), 0);
		currentRenderEncoder_->setFragmentTexture(shadowMap, 0);

		MTL::Buffer* currentVB = nullptr;
		for (const ScenePolyPacket& packet : polyPackets_) {
			if (packet.vertexCount <= 0) continue;
			if (packet.isPortal)         continue;

			MTL::Buffer* vb = nullptr;
			if (packet.isStaticWorld) {
				if (!hasStaticWorld) continue;
				vb = staticWorldVertexBuffer_.get();
			} else {
				if (!hasDynamic) continue;
				if ((size_t)(packet.firstVertex + packet.vertexCount) > polyVertexCountGPU_) continue;
				vb = polyVertexBuffer_.get();
			}
			if (vb != currentVB) {
				currentRenderEncoder_->setVertexBuffer(vb, 0, 0);
				currentVB = vb;
			}
			currentRenderEncoder_->drawPrimitives(
				packet.primitive,
				(NS::UInteger)packet.firstVertex,
				(NS::UInteger)packet.vertexCount);
		}

		// ---- Model entity receiver pass ----
		currentRenderEncoder_->setRenderPipelineState(pshadowRecvModelPipeline_.get());
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);
		// fragment state (ru, shadowMap) already bound above.

		for (const SceneDrawPacket& dp : drawPackets_) {
			const refEntity_t& ent = dp.entity;
			if (ent.reType != RT_MODEL)              continue;
			if (ent.renderfx & RF_NOSHADOW)          continue;
			if (ent.renderfx & RF_THIRD_PERSON)      continue;

			if (ent.hModel <= 0 || (size_t)ent.hModel >= models_.size()) continue;
			MetalModel* model = models_[ent.hModel];
			if (!model || model->type != MetalModelType::MD3) continue;
			int lod = 0;
			if (lod >= model->numLods || !model->lods[lod]) continue;
			MetalModelLOD* lodData = model->lods[lod];

			// Build ModelUniforms for this entity.
			float modelMatrix[16];
			calculateEntityTransform(ent, modelMatrix);
			float mvpMatrix[16];
			multiplyMatrices4x4(sceneCamera_.projectionMatrix, sceneCamera_.viewMatrix, modelMatrix, mvpMatrix);

			struct {
				float modelViewProjection[16];
				float modelMatrix[16];
				float vertexLerp;
				float padding[3];
			} mu{};
			std::memcpy(mu.modelViewProjection, mvpMatrix,     sizeof(float)*16);
			std::memcpy(mu.modelMatrix,         modelMatrix,   sizeof(float)*16);
			mu.vertexLerp = 1.0f - ent.backlerp;

			for (MetalModelSurface& surface : lodData->surfaces) {
				if (surface.numVerts <= 0 || surface.numIndexes <= 0) continue;
				if (!surface.indexBuffer) continue;

				// Create per-frame interleaved vertex buffer (same layout as main model pass).
				MTL::Buffer* vbuf = createModelVertexBuffer(ent, surface);
				if (!vbuf) continue;

				currentRenderEncoder_->setVertexBuffer(vbuf, 0, 0);
				currentRenderEncoder_->setVertexBytes(&mu, sizeof(mu), 1);

				MTL::Buffer* ibuf = static_cast<MTL::Buffer*>(surface.indexBuffer);
				currentRenderEncoder_->drawIndexedPrimitives(
					MTL::PrimitiveTypeTriangle,
					(NS::UInteger)surface.numIndexes,
					MTL::IndexTypeUInt32,
					ibuf, 0);

				vbuf->release();
			}
		}
	}

	// Restore default cull mode.
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);
}

MetalRenderer::StagePipelineEntry* MetalRenderer::getStagePipeline(
		const MetalRenderer::MetalShaderResource::MetalPipelineKey& key) {
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
	// Allow rendering as long as we have a static world buffer or a populated dynamic buffer.
	const bool hasStaticWorld = (staticWorldVertexBuffer_.get() != nullptr);
	const bool hasDynamic     = (polyVertexBuffer_ && polyVertexCountGPU_ > 0);
	if (!currentRenderEncoder_ || (!hasStaticWorld && !hasDynamic)) {
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

	// Bind uniform buffers once; the geometry vertex buffer (slot 0) is switched
	// lazily per-packet between staticWorldVertexBuffer_ and polyVertexBuffer_.
	currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
	// Also bind sceneUniformBuffer to the fragment stage so uniforms.viewOrigin is valid.
	currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 1);
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
	// Track which vertex buffer is currently bound to slot 0 to minimize rebinds.
	MTL::Buffer* currentVtxBuffer = nullptr;
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
	auto buildStageParams = [](const MetalShaderStageInfo* stageInfo, float overBrightBits, qhandle_t lightmapHandle, float timeSeconds) {
		StageFragmentParams params{};
		params.overBrightBits = overBrightBits;
		if (!stageInfo) {
			return params;
		}

		// Set tcGen type for shader
		// 0=Texture, 1=Lightmap, 2=Environment, 3=Vector, 4=Fog
		switch (stageInfo->tcGen.type) {
			case MetalTCGen::Lightmap:
				params.tcGenType = 1.0f;
				params.texCoordSelector = 1.0f;  // Legacy compatibility
				break;
			case MetalTCGen::Environment:
				params.tcGenType = 2.0f;
				params.texCoordSelector = 0.0f;
				break;
			case MetalTCGen::Vector:
				params.tcGenType = 3.0f;
				params.tcGenSVector[0] = stageInfo->tcGen.sVector[0];
				params.tcGenSVector[1] = stageInfo->tcGen.sVector[1];
				params.tcGenSVector[2] = stageInfo->tcGen.sVector[2];
				params.tcGenSVector[3] = 0.0f;
				params.tcGenTVector[0] = stageInfo->tcGen.tVector[0];
				params.tcGenTVector[1] = stageInfo->tcGen.tVector[1];
				params.tcGenTVector[2] = stageInfo->tcGen.tVector[2];
				params.tcGenTVector[3] = 0.0f;
				break;
			case MetalTCGen::Fog:
				params.tcGenType = 4.0f;
				break;
			default:
				params.tcGenType = 0.0f;
				params.texCoordSelector = 0.0f;
				break;
		}

		// Set rgbGen type for shader
		// 0=Vertex, 1=Identity, 2=IdentityLighting, 3=LightingDiffuse, 4=Wave,
		// 5=Const, 6=Entity, 7=OneMinusEntity, 8=OneMinusVertex
		// For world surfaces with lightmaps, override lightingDiffuse → vertex
		// (matches OpenGL2's behavior where lightmapped surfaces use CGEN_EXACT_VERTEX)
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
			case MetalRGBGen::Wave:
				params.rgbGenType = 4.0f;
				params.waveColorScale = EvalWaveForm(stageInfo->rgbGen.wave, timeSeconds);
				if (params.waveColorScale < 0.0f) params.waveColorScale = 0.0f;
				if (params.waveColorScale > 1.0f) params.waveColorScale = 1.0f;
				break;
			case MetalRGBGen::Const:
				params.rgbGenType = 5.0f;
				params.constColor[0] = stageInfo->rgbGen.constColor[0];
				params.constColor[1] = stageInfo->rgbGen.constColor[1];
				params.constColor[2] = stageInfo->rgbGen.constColor[2];
				params.constColor[3] = stageInfo->rgbGen.constColor[3];
				break;
			case MetalRGBGen::Entity:
				params.rgbGenType = 6.0f;
				// World entity is always white (shaderRGBA = {255,255,255,255})
				params.entityColor[0] = params.entityColor[1] = params.entityColor[2] = params.entityColor[3] = 1.0f;
				break;
			case MetalRGBGen::OneMinusEntity:
				params.rgbGenType = 7.0f;
				params.entityColor[0] = params.entityColor[1] = params.entityColor[2] = params.entityColor[3] = 1.0f;
				break;
			case MetalRGBGen::OneMinusVertex:
				params.rgbGenType = 8.0f;
				break;
			default:
				params.rgbGenType = 0.0f;  // Use vertex color
				break;
		}

		// alphaGen handling - matches GL2's ComputeShaderColors alphaGen block
		switch (stageInfo->alphaGen.type) {
			case MetalAlphaGen::Entity:
				params.alphaGenType = 1.0f;
				// entityColor.a already set to 1.0 (world entity); reuse entityColor slot
				params.entityColor[3] = 1.0f;
				break;
			case MetalAlphaGen::OneMinusEntity:
				params.alphaGenType = 2.0f;
				params.entityColor[3] = 1.0f;
				break;
			case MetalAlphaGen::Wave: {
				params.alphaGenType = 3.0f;
				float w = EvalWaveForm(stageInfo->alphaGen.wave, timeSeconds);
				if (w < 0.0f) w = 0.0f;
				if (w > 1.0f) w = 1.0f;
				params.alphaWaveValue = w;
				break;
			}
			case MetalAlphaGen::LightingSpecular:
				params.alphaGenType = 4.0f;
				break;
			case MetalAlphaGen::Portal:
				params.alphaGenType = 5.0f;
				params.portalRange = (stageInfo->alphaGen.portalRange > 0.0f)
				                     ? stageInfo->alphaGen.portalRange : 256.0f;
				break;
			default:
				params.alphaGenType = 0.0f;
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

		// Select and (if necessary) bind the correct geometry vertex buffer for this packet.
		MTL::Buffer* requiredVtxBuffer = nullptr;
		if (packet.isStaticWorld) {
			if (!hasStaticWorld) return;
			const size_t totalVerts = staticWorldVertexBufferSize_ / sizeof(MetalPolyVertex);
			const size_t endVertex  = static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount);
			if (endVertex > totalVerts) return;
			requiredVtxBuffer = staticWorldVertexBuffer_.get();
		} else {
			if (!hasDynamic) return;
			const size_t endVertex = static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount);
			if (endVertex > polyVertexCountGPU_) return;
			requiredVtxBuffer = polyVertexBuffer_.get();
		}
		if (requiredVtxBuffer != currentVtxBuffer) {
			currentRenderEncoder_->setVertexBuffer(requiredVtxBuffer, 0, 0);
			currentVtxBuffer = requiredVtxBuffer;
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

			bindStageParams(buildStageParams(stageInfo, stageOverBright, packet.lightmapHandle, sceneTimeSeconds));

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

			// For portal surfaces, use the rendered portal texture instead of shader texture
			// NOTE: Portal texture requires screen-space UV projection (not implemented yet)
			// For now, we skip portal texture application - the portal will show its base texture
			MTL::Texture* textureToUse = nullptr;
			// Portal texture application disabled - needs projective texturing
			// if (packet.isPortal && portalTexture_ && !portalSurfaces_.empty()) {
			// 	if (stageIndex == 0) {
			// 		textureToUse = portalTexture_.get();
			// 	}
			// }

			if (!textureToUse) {
				// Normal texture binding
				if (!boundTexture || desiredHandle != boundImageHandle) {
					MTL::Texture* packetTexture = texManager->getTexture(desiredHandle);
					if (!packetTexture) {
						packetTexture = defaultTexture;
						desiredHandle = 0;
					}
					textureToUse = packetTexture;
					boundImageHandle = desiredHandle;
				} else {
					textureToUse = boundTexture;
				}
			} else {
				// Portal texture - force rebind
				boundImageHandle = -1;  // Invalidate cache
			}

			if (textureToUse != boundTexture) {
				MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, textureToUse);
				boundTexture = textureToUse;
			}
			
			// Use clamp sampler if shader stage specifies clampmap
			bool useClamp = stageInfo && stageInfo->clampMap;
			MTL::SamplerState* sampler = (useClamp && sceneClampSampler_) ? sceneClampSampler_.get() : sceneSampler_.get();
			MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler);

			// Bind normal-map (texture 1) and specular-map (texture 2) for this stage.
			// Bind cubemap (texture 3) for environment reflections.
			// When no map was detected, fall back to the white/default texture so the
			// shader slot is always valid; useNormalMap/useSpecularMap flags disable sampling.
			{
				NormalSpecularParams nsParams{};
				MTL::Texture* normalTex = defaultTexture;
				MTL::Texture* specTex   = defaultTexture;
				if (stageRuntime) {
					if (stageRuntime->normalMapHandle > 0) {
						MTL::Texture* t = texManager->getTexture(stageRuntime->normalMapHandle);
						if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
					}
					if (stageRuntime->specularMapHandle > 0) {
						MTL::Texture* t = texManager->getTexture(stageRuntime->specularMapHandle);
						if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
					}
				}
				currentRenderEncoder_->setFragmentTexture(normalTex, 1);
				currentRenderEncoder_->setFragmentTexture(specTex,   2);

				// Cubemap reflection: look up the probe texture for this surface.
				// Always bind a valid cubemap to slot 3 (Metal validation requires it),
				// but only enable sampling when we have a real probe.
				MTL::Texture* cubeTex = getOrCreateFallbackCubemap();
				if (!worldCubemapTextures_.empty()) {
					int ci = packet.cubemapIndex; // 0 = fallback, 1..N = probe
					if (ci > 0 && ci < (int)worldCubemapTextures_.size()) {
						MTL::Texture* probeTex = worldCubemapTextures_[ci].get();
						if (probeTex) {
							cubeTex = probeTex;
							nsParams.useCubemap      = 1.0f;
							nsParams.cubemapStrength = 0.25f; // base strength; spec-alpha modulates it
						}
					}
				}
				currentRenderEncoder_->setFragmentTexture(cubeTex, 3);
				currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 3);
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
				// Reset state after skybox rendering — skybox uses its own vertex buffer,
				// so force a rebind of the geometry buffer on the next draw call.
				currentVtxBuffer = nullptr;
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
				// Reset state after cloud sky rendering — cloud sky uses its own vertex buffer,
				// so force a rebind of the geometry buffer on the next draw call.
				currentVtxBuffer = nullptr;
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

bool MetalRenderer::drawPolyPacketsForPortal(int excludePacketIndex) {
	// Simplified version of drawPolyPackets for portal rendering
	// Skips portal surfaces to avoid recursion
	if (polyPackets_.empty()) {
		return true;
	}
	const bool hasStaticWorld = (staticWorldVertexBuffer_.get() != nullptr);
	const bool hasDynamic     = (polyVertexBuffer_ && polyVertexCountGPU_ > 0);
	if (!currentRenderEncoder_ || (!hasStaticWorld && !hasDynamic)) {
		return true;
	}
	if (!sceneUniformBuffer_) {
		return false;
	}
	if (!ensureSceneShaderResources()) {
		return false;
	}

	currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
	currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 1);
	EntityLightingParams defaultLighting{};
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
	MTL::Buffer* currentVtxBuffer = nullptr;
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;

	// Draw all non-portal, non-sky packets
	for (int i = 0; i < static_cast<int>(polyPackets_.size()); ++i) {
		const ScenePolyPacket& packet = polyPackets_[i];

		// Skip the portal surface we're rendering for
		if (i == excludePacketIndex) {
			continue;
		}

		// Skip all portal surfaces to avoid recursion
		if (packet.isPortal) {
			continue;
		}

		// Skip sky surfaces
		MetalShaderResource* shaderResource = getShaderResource(packet.shader);
		if (shaderResource && shaderResource->hasScript && shaderResource->script.isSky) {
			continue;
		}

		// Skip invalid packets and select vertex buffer
		if (packet.vertexCount <= 0) {
			continue;
		}
		MTL::Buffer* requiredVtxBuffer = nullptr;
		if (packet.isStaticWorld) {
			if (!hasStaticWorld) continue;
			const size_t totalVerts = staticWorldVertexBufferSize_ / sizeof(MetalPolyVertex);
			if (static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount) > totalVerts) continue;
			requiredVtxBuffer = staticWorldVertexBuffer_.get();
		} else {
			if (!hasDynamic) continue;
			if (static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount) > polyVertexCountGPU_) continue;
			requiredVtxBuffer = polyVertexBuffer_.get();
		}
		if (requiredVtxBuffer != currentVtxBuffer) {
			currentRenderEncoder_->setVertexBuffer(requiredVtxBuffer, 0, 0);
			currentVtxBuffer = requiredVtxBuffer;
		}

		// Draw the packet (simplified single-stage rendering)
		if (!shaderResource) {
			continue;
		}

		const MetalShaderResource::MetalShaderStageRuntime* stageRuntime = getPrimaryStageRuntime(*shaderResource);
		if (!stageRuntime) {
			continue;
		}

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

		// Build stage params
		StageFragmentParams params{};
		params.overBrightBits = 1.0f;
		if (stageRuntime->stageInfo) {
			switch (stageRuntime->stageInfo->tcGen.type) {
				case MetalTCGen::Lightmap:
					params.tcGenType = 1.0f;
					params.texCoordSelector = 1.0f;
					break;
				case MetalTCGen::Environment:
					params.tcGenType = 2.0f;
					break;
				default:
					params.tcGenType = 0.0f;
					break;
			}
		}
		currentRenderEncoder_->setFragmentBytes(&params, sizeof(StageFragmentParams), 0);

		// Compute and bind TCMod params
		TCModParams tcModParams = computeTCModParams(stageRuntime->stageInfo, sceneTimeSeconds);
		currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);

		// Bind texture
		qhandle_t desiredHandle = selectStageImage(*shaderResource, stageRuntime, sceneTimeSeconds, packet.lightmapHandle);
		if (desiredHandle < 0) desiredHandle = 0;
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

		// Use appropriate sampler
		bool useClamp = stageRuntime->stageInfo && stageRuntime->stageInfo->clampMap;
		MTL::SamplerState* sampler = (useClamp && sceneClampSampler_) ? sceneClampSampler_.get() : sceneSampler_.get();
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler);

		// Bind normal/specular maps and cubemap so all fragment texture/buffer slots are valid.
		{
			NormalSpecularParams nsParams{};
			MTL::Texture* normalTex = defaultTexture;
			MTL::Texture* specTex   = defaultTexture;
			if (stageRuntime->normalMapHandle > 0) {
				MTL::Texture* t = texManager->getTexture(stageRuntime->normalMapHandle);
				if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
			}
			if (stageRuntime->specularMapHandle > 0) {
				MTL::Texture* t = texManager->getTexture(stageRuntime->specularMapHandle);
				if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
			}
			currentRenderEncoder_->setFragmentTexture(normalTex, 1);
			currentRenderEncoder_->setFragmentTexture(specTex,   2);
			// Always bind a valid cubemap to slot 3; cubemap reflections are
			// disabled in portal views (useCubemap stays 0.0f).
			currentRenderEncoder_->setFragmentTexture(getOrCreateFallbackCubemap(), 3);
			currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 3);
		}

		currentRenderEncoder_->drawPrimitives(packet.primitive,
		                                       static_cast<NS::UInteger>(packet.firstVertex),
		                                       static_cast<NS::UInteger>(packet.vertexCount));
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
	// When a sky portal entity is present, render the skybox centred on the portal
	// origin rather than the player's eye, matching GL2's R_AddSkyPortal behaviour.
	const float* viewOrigin = hasSkyPortal_ ? skyPortalOrigin_ : sceneCamera_.viewOrigin;
	
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
	// Mirror drawSkybox: use the portal origin when a sky portal entity is present.
	const float* viewOrigin = hasSkyPortal_ ? skyPortalOrigin_ : sceneCamera_.viewOrigin;
	
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
	// TEMPORARILY DISABLED to debug performance issue
	return true;
	
	// Check if we have lights to render
	if (lightPackets_.empty()) {
		return true;  // No lights
	}

	const bool hasStaticWorld = (staticWorldVertexBuffer_.get() != nullptr);
	const bool hasDynamic     = (polyVertexBuffer_ && polyVertexCountGPU_ > 0);
	if (!currentRenderEncoder_ || (!hasStaticWorld && !hasDynamic)) {
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
	MTL::Buffer* currentVtxBuffer = nullptr;

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
			if (packet.vertexCount <= 0) {
				continue;
			}

			// Select and bind the correct vertex buffer for this packet
			MTL::Buffer* requiredVtxBuffer = nullptr;
			if (packet.isStaticWorld) {
				if (!hasStaticWorld) continue;
				const size_t totalVerts = staticWorldVertexBufferSize_ / sizeof(MetalPolyVertex);
				if (static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount) > totalVerts) continue;
				requiredVtxBuffer = staticWorldVertexBuffer_.get();
			} else {
				if (!hasDynamic) continue;
				if (static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount) > polyVertexCountGPU_) continue;
				requiredVtxBuffer = polyVertexBuffer_.get();
			}
			if (requiredVtxBuffer != currentVtxBuffer) {
				currentRenderEncoder_->setVertexBuffer(requiredVtxBuffer, 0, 0);
				currentVtxBuffer = requiredVtxBuffer;
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

	const bool hasStaticWorld = (staticWorldVertexBuffer_.get() != nullptr);
	const bool hasDynamic     = (polyVertexBuffer_ && polyVertexCountGPU_ > 0);
	if (!currentRenderEncoder_ || (!hasStaticWorld && !hasDynamic)) {
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

	MTL::Buffer* currentVtxBuffer = nullptr;

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

			// Select the correct vertex buffer for this packet
			MTL::Buffer* requiredVtxBuffer = nullptr;
			if (packet.isStaticWorld) {
				if (!hasStaticWorld) continue;
				const size_t totalVerts = staticWorldVertexBufferSize_ / sizeof(MetalPolyVertex);
				if (static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount) > totalVerts) continue;
				requiredVtxBuffer = staticWorldVertexBuffer_.get();
			} else {
				if (!hasDynamic) continue;
				if (static_cast<size_t>(packet.firstVertex) + static_cast<size_t>(packet.vertexCount) > polyVertexCountGPU_) continue;
				requiredVtxBuffer = polyVertexBuffer_.get();
			}
			if (requiredVtxBuffer != currentVtxBuffer) {
				currentRenderEncoder_->setVertexBuffer(requiredVtxBuffer, 0, 0);
				currentVtxBuffer = requiredVtxBuffer;
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
// Portal/Mirror Rendering - Following GL2 tr_main.c exactly
//=============================================================================

// Helper: Compute perpendicular vector for portal axis (local version to avoid conflict)
static void ComputePerpendicularVector(float* out, const float* in) {
	int pos = 0;
	float minelem = 1.0f;
	// Find the smallest component
	for (int i = 0; i < 3; ++i) {
		float f = fabsf(in[i]);
		if (f < minelem) {
			pos = i;
			minelem = f;
		}
	}
	float tempvec[3] = {0, 0, 0};
	tempvec[pos] = 1.0f;
	
	// Project and normalize
	float d = in[0] * tempvec[0] + in[1] * tempvec[1] + in[2] * tempvec[2];
	out[0] = tempvec[0] - d * in[0];
	out[1] = tempvec[1] - d * in[1];
	out[2] = tempvec[2] - d * in[2];
	float len = sqrtf(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
	if (len > 0.0001f) {
		out[0] /= len;
		out[1] /= len;
		out[2] /= len;
	}
}

void MetalRenderer::detectPortalSurfaces() {
	portalSurfaces_.clear();

	// Don't recursively detect portals (avoid infinite loops)
	if (isRenderingPortal_) {
		return;
	}

	// Find all portal surfaces in the scene
	for (size_t i = 0; i < polyPackets_.size(); ++i) {
		const ScenePolyPacket& packet = polyPackets_[i];
		if (!packet.isPortal || packet.vertexCount < 3) {
			continue;
		}

		// Compute plane equation from first triangle
		if (packet.firstVertex < 0) {
			continue;
		}
		const auto* verts = packet.isStaticWorld ? worldVertexTemplate_.data() : polyVertices_.data();
		const size_t vsize = packet.isStaticWorld ? worldVertexTemplate_.size() : polyVertices_.size();
		if (static_cast<size_t>(packet.firstVertex + 2) >= vsize) {
			continue;
		}

		const MetalPolyVertex& v0 = verts[packet.firstVertex];
		const MetalPolyVertex& v1 = verts[packet.firstVertex + 1];
		const MetalPolyVertex& v2 = verts[packet.firstVertex + 2];

		// Compute edge vectors
		float e1[3] = {v1.xyz[0] - v0.xyz[0], v1.xyz[1] - v0.xyz[1], v1.xyz[2] - v0.xyz[2]};
		float e2[3] = {v2.xyz[0] - v0.xyz[0], v2.xyz[1] - v0.xyz[1], v2.xyz[2] - v0.xyz[2]};

		// Cross product for normal
		float normal[3] = {
			e1[1] * e2[2] - e1[2] * e2[1],
			e1[2] * e2[0] - e1[0] * e2[2],
			e1[0] * e2[1] - e1[1] * e2[0]
		};

		// Normalize
		float len = sqrtf(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
		if (len < 0.001f) {
			continue;  // Degenerate triangle
		}
		normal[0] /= len;
		normal[1] /= len;
		normal[2] /= len;

		// Compute center of surface (average all vertices)
		float center[3] = {0.0f, 0.0f, 0.0f};
		for (int j = 0; j < packet.vertexCount; ++j) {
			const MetalPolyVertex& v = verts[packet.firstVertex + j];
			center[0] += v.xyz[0];
			center[1] += v.xyz[1];
			center[2] += v.xyz[2];
		}
		center[0] /= static_cast<float>(packet.vertexCount);
		center[1] /= static_cast<float>(packet.vertexCount);
		center[2] /= static_cast<float>(packet.vertexCount);

		// Compute plane distance
		float dist = normal[0] * center[0] + normal[1] * center[1] + normal[2] * center[2];

		// Check if portal is facing us (back-face cull)
		float viewDir[3] = {
			sceneCamera_.viewOrigin[0] - center[0],
			sceneCamera_.viewOrigin[1] - center[1],
			sceneCamera_.viewOrigin[2] - center[2]
		};
		float dot = normal[0] * viewDir[0] + normal[1] * viewDir[1] + normal[2] * viewDir[2];
		if (dot < 0.0f) {
			continue;  // Portal facing away from camera
		}

		// Create portal surface entry
		PortalSurface portal;
		portal.packetIndex = static_cast<int>(i);
		portal.plane[0] = normal[0];
		portal.plane[1] = normal[1];
		portal.plane[2] = normal[2];
		portal.plane[3] = dist;
		portal.center[0] = center[0];
		portal.center[1] = center[1];
		portal.center[2] = center[2];
		portal.isMirror = true;

		// Build surface axis (like GL2's R_GetPortalOrientations)
		// surface.axis[0] = plane normal
		// surface.axis[1] = perpendicular to normal
		// surface.axis[2] = cross product
		portal.surface.axis[0][0] = normal[0];
		portal.surface.axis[0][1] = normal[1];
		portal.surface.axis[0][2] = normal[2];
		ComputePerpendicularVector(portal.surface.axis[1], portal.surface.axis[0]);
		// CrossProduct
		portal.surface.axis[2][0] = portal.surface.axis[0][1] * portal.surface.axis[1][2] - portal.surface.axis[0][2] * portal.surface.axis[1][1];
		portal.surface.axis[2][1] = portal.surface.axis[0][2] * portal.surface.axis[1][0] - portal.surface.axis[0][0] * portal.surface.axis[1][2];
		portal.surface.axis[2][2] = portal.surface.axis[0][0] * portal.surface.axis[1][1] - portal.surface.axis[0][1] * portal.surface.axis[1][0];

		// Search for RT_PORTALSURFACE entity that matches this plane
		// This is exactly what GL2 does in R_GetPortalOrientations
		bool foundPortalEntity = false;
		for (const SceneDrawPacket& entPacket : drawPackets_) {
			if (entPacket.entity.reType != RT_PORTALSURFACE) {
				continue;
			}

			// Check distance from entity to plane (must be within 64 units)
			float entityDist = normal[0] * entPacket.entity.origin[0] + 
			                   normal[1] * entPacket.entity.origin[1] + 
			                   normal[2] * entPacket.entity.origin[2] - dist;
			if (entityDist > 64.0f || entityDist < -64.0f) {
				continue;
			}

			// Found a matching portal entity!
			foundPortalEntity = true;

			// Check if it's a mirror: origin == oldorigin
			bool isMirror = (entPacket.entity.oldorigin[0] == entPacket.entity.origin[0] &&
			                 entPacket.entity.oldorigin[1] == entPacket.entity.origin[1] &&
			                 entPacket.entity.oldorigin[2] == entPacket.entity.origin[2]);

			if (isMirror) {
				// Mirror setup - exactly like GL2
				// surface.origin = normal * dist (point on the plane)
				portal.surface.origin[0] = normal[0] * dist;
				portal.surface.origin[1] = normal[1] * dist;
				portal.surface.origin[2] = normal[2] * dist;

				// camera.origin = same as surface (mirror is at the plane)
				portal.camera.origin[0] = portal.surface.origin[0];
				portal.camera.origin[1] = portal.surface.origin[1];
				portal.camera.origin[2] = portal.surface.origin[2];

				// camera.axis[0] = -surface.axis[0] (flip the normal)
				portal.camera.axis[0][0] = -portal.surface.axis[0][0];
				portal.camera.axis[0][1] = -portal.surface.axis[0][1];
				portal.camera.axis[0][2] = -portal.surface.axis[0][2];
				// camera.axis[1] = surface.axis[1] (keep right)
				portal.camera.axis[1][0] = portal.surface.axis[1][0];
				portal.camera.axis[1][1] = portal.surface.axis[1][1];
				portal.camera.axis[1][2] = portal.surface.axis[1][2];
				// camera.axis[2] = surface.axis[2] (keep up)
				portal.camera.axis[2][0] = portal.surface.axis[2][0];
				portal.camera.axis[2][1] = portal.surface.axis[2][1];
				portal.camera.axis[2][2] = portal.surface.axis[2][2];

				portal.isMirror = true;
			} else {
				// True portal - use entity's camera position
				// Not yet fully implemented
				portal.isMirror = false;
			}

			break;  // Use first matching entity
		}

		if (!foundPortalEntity) {
			// No RT_PORTALSURFACE entity found - can't render this portal
			// GL2 returns qfalse here and doesn't render
			continue;
		}

		portalSurfaces_.push_back(portal);

		// Only process one portal per frame (matches GL2 behavior)
		break;
	}

	if (!portalSurfaces_.empty() && ri_.Printf) {
		static int logCounter = 0;
		if (logCounter++ % 300 == 0) {
			ri_.Printf(PRINT_DEVELOPER, "Metal: Detected %zu portal surfaces\n", portalSurfaces_.size());
		}
	}
}

bool MetalRenderer::ensurePortalTexture(int width, int height) {
	// Check if existing texture is suitable
	if (portalTexture_ && portalTextureWidth_ == width && portalTextureHeight_ == height) {
		return true;
	}

	// Create color texture for portal rendering
	MTL::TextureDescriptor* colorDesc = MTL::TextureDescriptor::texture2DDescriptor(
		MTL::PixelFormatBGRA8Unorm,
		static_cast<NS::UInteger>(width),
		static_cast<NS::UInteger>(height),
		false
	);
	colorDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
	colorDesc->setStorageMode(MTL::StorageModePrivate);

	portalTexture_ = MetalPtr<MTL::Texture>(device_->newTexture(colorDesc));
	if (!portalTexture_) {
		ri_.Printf(PRINT_WARNING, "Metal: Failed to create portal color texture\n");
		return false;
	}

	// Create depth texture for portal rendering
	MTL::TextureDescriptor* depthDesc = MTL::TextureDescriptor::texture2DDescriptor(
		MTL::PixelFormatDepth32Float,
		static_cast<NS::UInteger>(width),
		static_cast<NS::UInteger>(height),
		false
	);
	depthDesc->setUsage(MTL::TextureUsageRenderTarget);
	depthDesc->setStorageMode(MTL::StorageModePrivate);

	portalDepthTexture_ = MetalPtr<MTL::Texture>(device_->newTexture(depthDesc));
	if (!portalDepthTexture_) {
		ri_.Printf(PRINT_WARNING, "Metal: Failed to create portal depth texture\n");
		portalTexture_.reset();
		return false;
	}

	portalTextureWidth_ = width;
	portalTextureHeight_ = height;

	ri_.Printf(PRINT_DEVELOPER, "Metal: Created portal render target %dx%d\n", width, height);
	return true;
}

// GL2's R_MirrorPoint: Transform a point through surface orientation to camera orientation
static void R_MirrorPoint(const float* in, const PortalOrientation& surface, 
                          const PortalOrientation& camera, float* out) {
	// VectorSubtract(in, surface.origin, local)
	float local[3] = {
		in[0] - surface.origin[0],
		in[1] - surface.origin[1],
		in[2] - surface.origin[2]
	};

	// Transform local to camera space
	float transformed[3] = {0, 0, 0};
	for (int i = 0; i < 3; ++i) {
		float d = local[0] * surface.axis[i][0] + local[1] * surface.axis[i][1] + local[2] * surface.axis[i][2];
		transformed[0] += d * camera.axis[i][0];
		transformed[1] += d * camera.axis[i][1];
		transformed[2] += d * camera.axis[i][2];
	}

	// VectorAdd(transformed, camera.origin, out)
	out[0] = transformed[0] + camera.origin[0];
	out[1] = transformed[1] + camera.origin[1];
	out[2] = transformed[2] + camera.origin[2];
}

// GL2's R_MirrorVector: Transform a vector (direction) through surface to camera orientation
static void R_MirrorVector(const float* in, const PortalOrientation& surface,
                           const PortalOrientation& camera, float* out) {
	out[0] = out[1] = out[2] = 0;
	for (int i = 0; i < 3; ++i) {
		float d = in[0] * surface.axis[i][0] + in[1] * surface.axis[i][1] + in[2] * surface.axis[i][2];
		out[0] += d * camera.axis[i][0];
		out[1] += d * camera.axis[i][1];
		out[2] += d * camera.axis[i][2];
	}
}

void MetalRenderer::calculateMirrorMatrix(const PortalSurface& portal, float* viewMatrix, float* projMatrix) {
	// Use GL2's approach: R_MirrorPoint for origin, R_MirrorVector for each axis
	float newOrigin[3];
	R_MirrorPoint(sceneCamera_.viewOrigin, portal.surface, portal.camera, newOrigin);

	float newAxis[3][3];
	R_MirrorVector(sceneCamera_.viewAxis[0], portal.surface, portal.camera, newAxis[0]);
	R_MirrorVector(sceneCamera_.viewAxis[1], portal.surface, portal.camera, newAxis[1]);
	R_MirrorVector(sceneCamera_.viewAxis[2], portal.surface, portal.camera, newAxis[2]);

	// Build view matrix from mirrored origin and axis
	// View matrix = inverse of camera transform
	// For a look-at style matrix:
	// | Rx  Ux  -Fx  0 |   | 1 0 0 -Tx |
	// | Ry  Uy  -Fy  0 | * | 0 1 0 -Ty |
	// | Rz  Uz  -Fz  0 |   | 0 0 1 -Tz |
	// | 0   0    0   1 |   | 0 0 0  1  |
	//
	// Where R=right(axis[1]), U=up(axis[2]), F=forward(axis[0]), T=position

	float* R = newAxis[1];  // right
	float* U = newAxis[2];  // up
	float* F = newAxis[0];  // forward

	// Column-major
	viewMatrix[0] = R[0];  viewMatrix[4] = R[1];  viewMatrix[8]  = R[2];   viewMatrix[12] = -(R[0]*newOrigin[0] + R[1]*newOrigin[1] + R[2]*newOrigin[2]);
	viewMatrix[1] = U[0];  viewMatrix[5] = U[1];  viewMatrix[9]  = U[2];   viewMatrix[13] = -(U[0]*newOrigin[0] + U[1]*newOrigin[1] + U[2]*newOrigin[2]);
	viewMatrix[2] = -F[0]; viewMatrix[6] = -F[1]; viewMatrix[10] = -F[2];  viewMatrix[14] = (F[0]*newOrigin[0] + F[1]*newOrigin[1] + F[2]*newOrigin[2]);
	viewMatrix[3] = 0.0f;  viewMatrix[7] = 0.0f;  viewMatrix[11] = 0.0f;   viewMatrix[15] = 1.0f;

	// Use same projection as main camera
	std::memcpy(projMatrix, sceneUniforms_.projection, sizeof(float) * 16);
}

void MetalRenderer::calculatePortalMatrix(const PortalSurface& portal, const refEntity_t& cameraEntity,
                                          float* viewMatrix, float* projMatrix) {
	// For true portals (not mirrors), the view is from the camera entity's position
	// TODO: Implement full portal camera transformation
	// For now, fall back to mirror behavior
	calculateMirrorMatrix(portal, viewMatrix, projMatrix);
}

bool MetalRenderer::renderPortalView(const PortalSurface& portal) {
	// Ensure portal texture exists
	if (!ensurePortalTexture(config_.vidWidth, config_.vidHeight)) {
		return false;
	}

	if (!currentCommandBuffer_ || !device_) {
		return false;
	}

	// Save current render encoder - we'll need to resume after portal rendering
	MTL::RenderCommandEncoder* mainEncoder = currentRenderEncoder_;
	if (!mainEncoder) {
		return false;
	}

	// End the main render encoder temporarily
	mainEncoder->endEncoding();
	MetalStateCache::Instance().resetEncoder(nullptr);

	// Create render pass for portal texture
	MTL::RenderPassDescriptor* portalRpd = MTL::RenderPassDescriptor::renderPassDescriptor();
	portalRpd->colorAttachments()->object(0)->setTexture(portalTexture_.get());
	portalRpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
	portalRpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
	portalRpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

	if (portalDepthTexture_) {
		portalRpd->depthAttachment()->setTexture(portalDepthTexture_.get());
		portalRpd->depthAttachment()->setLoadAction(MTL::LoadActionClear);
		portalRpd->depthAttachment()->setClearDepth(1.0);
		portalRpd->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
	}

	MTL::RenderCommandEncoder* portalEncoder = currentCommandBuffer_->renderCommandEncoder(portalRpd);
	portalRpd->release();

	if (!portalEncoder) {
		// Resume main encoder and return failure
		resumeMainEncoder(mainEncoder);
		return false;
	}

	// Set up portal encoder state
	currentRenderEncoder_ = portalEncoder;
	MetalStateCache::Instance().resetEncoder(portalEncoder);
	portalEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
	portalEncoder->setCullMode(MTL::CullModeNone);

	// Calculate mirrored view matrix
	float portalView[16];
	float portalProj[16];
	if (portal.isMirror) {
		calculateMirrorMatrix(portal, portalView, portalProj);
	} else {
		// For true portals, we'd need to find the camera entity
		// For now, use mirror matrix
		calculateMirrorMatrix(portal, portalView, portalProj);
	}

	// Save original scene uniforms
	SceneUniforms savedUniforms = sceneUniforms_;

	// Update scene uniforms with portal view
	std::memcpy(sceneUniforms_.view, portalView, sizeof(float) * 16);
	std::memcpy(sceneUniforms_.projection, portalProj, sizeof(float) * 16);

	// Compute viewProjection = projection * view
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float sum = 0.0f;
			for (int k = 0; k < 4; ++k) {
				sum += portalProj[i + k * 4] * portalView[k + j * 4];
			}
			sceneUniforms_.viewProjection[i + j * 4] = sum;
		}
	}

	// Upload updated uniforms
	if (sceneUniformBuffer_) {
		std::memcpy(sceneUniformBuffer_->contents(), &sceneUniforms_, sizeof(SceneUniforms));
	}

	// Configure viewport
	configureSceneViewport(sceneCamera_.refdef);

	// Upload vertex buffer for portal pass
	if (polyVertexBuffer_) {
		portalEncoder->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	}
	if (sceneUniformBuffer_) {
		portalEncoder->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
	}

	// Draw world geometry (excluding portal surfaces)
	drawPolyPacketsForPortal(portal.packetIndex);

	// Draw model entities
	drawModelEntities();

	// End portal encoder
	portalEncoder->endEncoding();
	portalEncoder->release();

	// Restore original uniforms
	sceneUniforms_ = savedUniforms;
	if (sceneUniformBuffer_) {
		std::memcpy(sceneUniformBuffer_->contents(), &sceneUniforms_, sizeof(SceneUniforms));
	}

	// Resume main render encoder with fresh render pass
	resumeMainEncoder(mainEncoder);

	return true;
}

bool MetalRenderer::renderPortalViews() {
	// Portal/mirror rendering - GL2 approach
	// 
	// GL2 renders portal views by:
	// 1. Detecting portal surfaces during draw surface sorting
	// 2. Calling R_RenderView with mirrored camera (full scene re-generation)
	// 3. The mirrored scene renders to the SAME framebuffer
	// 4. Then the main scene renders on top
	//
	// Our Metal approach is different because we batch geometry during scene submission,
	// not during rendering. We can't easily regenerate the scene from a different camera.
	//
	// For now, we skip portal rendering. The portal surface will show its base texture.
	// A proper implementation would require either:
	// - Re-architecting to support multiple render views per frame
	// - Using compute shaders for portal texture projection
	// - Deferred rendering with portal-aware compositing
	//
	// This is marked as a known limitation.
	
	return true;
}

void MetalRenderer::resumeMainEncoder(MTL::RenderCommandEncoder* oldEncoder) {
	// The old encoder has been ended and released. We need to create a fresh one
	// targeting the drawable again.
	(void)oldEncoder;  // We don't reuse it

	if (!currentCommandBuffer_ || !currentDrawable_) {
		return;
	}

	// Create a new render pass targeting the drawable
	MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
	rpd->colorAttachments()->object(0)->setTexture(currentDrawable_->texture());
	// Use LoadActionLoad to preserve any previous drawing (like 2D elements)
	rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
	rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

	if (depthTexture_) {
		rpd->depthAttachment()->setTexture(depthTexture_.get());
		// Clear depth for fresh scene rendering
		rpd->depthAttachment()->setLoadAction(MTL::LoadActionClear);
		rpd->depthAttachment()->setClearDepth(1.0);
		rpd->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
	}

	currentRenderEncoder_ = currentCommandBuffer_->renderCommandEncoder(rpd);
	rpd->release();

	if (currentRenderEncoder_) {
		MetalStateCache::Instance().resetEncoder(currentRenderEncoder_);
		currentRenderEncoder_->setFrontFacingWinding(MTL::WindingCounterClockwise);
		currentRenderEncoder_->setCullMode(MTL::CullModeNone);

		// Restore vertex buffers
		if (polyVertexBuffer_) {
			currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
		}
		if (sceneUniformBuffer_) {
			currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 1);
		}
	}
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

/*
 * renderModelShadow - projection (blob) shadow pass for a single MD3 surface.
 *
 * Mirrors RB_ProjectionShadowDeform() from renderergl2/tr_shadows.c exactly:
 *   - ground is the world-up direction expressed in model space
 *   - h = how far above the shadow plane each vertex sits (model space)
 *   - deformed position = original - light * h  (projects vertex onto plane)
 *
 * The deformed vertices remain in model space; the same mvpMatrix that was
 * used to render the surface is re-used here so they land correctly in clip space.
 */
void MetalRenderer::renderModelShadow(
	const refEntity_t& ent,
	MetalModelSurface& surface,
	const float* mvpMatrix,
	const vec3_t modelLightDir)
{
	if (!currentRenderEncoder_ || !shadowPipeline_ || !shadowDepthState_ || !device_) {
		return;
	}
	if (!surface.indexBuffer || surface.numVerts <= 0 || surface.numIndexes <= 0) {
		return;
	}

	// -----------------------------------------------------------------------
	// Compute the shadow projection parameters (matches RB_ProjectionShadowDeform)
	// -----------------------------------------------------------------------

	// ground: world-up direction (Z axis) in model space.
	// backEnd.or.axis[i][2] in GL2 == the Z-component of entity axis[i].
	vec3_t ground;
	ground[0] = ent.axis[0][2];
	ground[1] = ent.axis[1][2];
	ground[2] = ent.axis[2][2];

	// groundDist: signed distance from entity origin to shadow plane in world Z.
	const float groundDist = ent.origin[2] - ent.shadowPlane;

	// lightDir adjusted so it never becomes too horizontal (d < 0.5 case).
	vec3_t lightDir;
	VectorCopy(modelLightDir, lightDir);
	float d = DotProduct(lightDir, ground);
	if (d < 0.5f) {
		VectorMA(lightDir, (0.5f - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}
	d = 1.0f / d;

	vec3_t light;
	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	// -----------------------------------------------------------------------
	// Clamp frame indices, compute lerp factor
	// -----------------------------------------------------------------------
	int oldFrame = ent.oldframe;
	int newFrame = ent.frame;
	if (oldFrame < 0) oldFrame = 0;
	if (oldFrame >= surface.numFrames) oldFrame = surface.numFrames - 1;
	if (newFrame < 0) newFrame = 0;
	if (newFrame >= surface.numFrames) newFrame = surface.numFrames - 1;
	const float vertexLerp = 1.0f - ent.backlerp;

	// -----------------------------------------------------------------------
	// Build shadow vertex buffer: deformed positions in model space (float3 each)
	// -----------------------------------------------------------------------
	const int numVerts = surface.numVerts;
	std::vector<float> shadowPositions(numVerts * 3);

	for (int i = 0; i < numVerts; i++) {
		const MetalModelVertex& oldVert = surface.vertices[oldFrame * numVerts + i];
		const MetalModelVertex& newVert = surface.vertices[newFrame * numVerts + i];

		// Lerp between frames (matches vertex_model GPU lerp: mix(old, new, vertexLerp))
		float xyz[3];
		xyz[0] = oldVert.xyz[0] + vertexLerp * (newVert.xyz[0] - oldVert.xyz[0]);
		xyz[1] = oldVert.xyz[1] + vertexLerp * (newVert.xyz[1] - oldVert.xyz[1]);
		xyz[2] = oldVert.xyz[2] + vertexLerp * (newVert.xyz[2] - oldVert.xyz[2]);

		// Height of this vertex above the shadow plane (model space)
		const float h = DotProduct(xyz, ground) + groundDist;

		// Project vertex onto shadow plane
		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;

		shadowPositions[i * 3 + 0] = xyz[0];
		shadowPositions[i * 3 + 1] = xyz[1];
		shadowPositions[i * 3 + 2] = xyz[2];
	}

	MTL::Buffer* shadowVertexBuffer = device_->newBuffer(
		shadowPositions.data(),
		static_cast<NS::UInteger>(shadowPositions.size() * sizeof(float)),
		MTL::ResourceStorageModeShared);
	if (!shadowVertexBuffer) {
		return;
	}

	// -----------------------------------------------------------------------
	// Shadow shader uniforms (must match ShadowShaderUniforms in scene.metal)
	// -----------------------------------------------------------------------
	struct ShadowShaderUniforms {
		float mvpMatrix[16];
		float color[4];
	};
	ShadowShaderUniforms shadowUniforms{};
	std::memcpy(shadowUniforms.mvpMatrix, mvpMatrix, sizeof(float) * 16);
	shadowUniforms.color[0] = 0.0f;
	shadowUniforms.color[1] = 0.0f;
	shadowUniforms.color[2] = 0.0f;
	shadowUniforms.color[3] = 0.5f;  // 50 % opaque black blob

	// -----------------------------------------------------------------------
	// Encode shadow draw call
	// -----------------------------------------------------------------------
	currentRenderEncoder_->setRenderPipelineState(shadowPipeline_.get());
	currentRenderEncoder_->setDepthStencilState(shadowDepthState_.get());
	// Render both faces so the shadow is visible from all angles
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

	currentRenderEncoder_->setVertexBuffer(shadowVertexBuffer, 0, 0);
	currentRenderEncoder_->setVertexBytes(&shadowUniforms, sizeof(ShadowShaderUniforms), 1);
	currentRenderEncoder_->setFragmentBytes(&shadowUniforms, sizeof(ShadowShaderUniforms), 1);

	MTL::Buffer* indexBuffer = static_cast<MTL::Buffer*>(surface.indexBuffer);
	currentRenderEncoder_->drawIndexedPrimitives(
		MTL::PrimitiveTypeTriangle,
		static_cast<NS::UInteger>(surface.numIndexes),
		MTL::IndexTypeUInt32,
		indexBuffer,
		0);

	shadowVertexBuffer->release();

	// Restore the cull mode used by model rendering (front-face culled for Q3 convention)
	currentRenderEncoder_->setCullMode(MTL::CullModeFront);
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
	const vec3_t lightDir,
	uint32_t dlightBits)
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
	lightingParams.ambientLight[3] = 0.0f;
	VectorScale(directedLight, 1.0f / 255.0f, lightingParams.directedLight);
	lightingParams.directedLight[3] = 0.0f;
	vec3_t normalizedLightDir;
	VectorNormalize2(lightDir, normalizedLightDir);
	VectorCopy(normalizedLightDir, lightingParams.lightDir);
	lightingParams.lightDir[3] = 0.0f;

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
	
	// Compute clamped overbright bits matching OpenGL2's tr.overbrightBits
	int overbrightBits = r_overBrightBits_ ? r_overBrightBits_->integer : 1;
	int mapOverbrightBits = r_mapOverBrightBits_ ? r_mapOverBrightBits_->integer : 2;
	if (overbrightBits > 2) overbrightBits = 2;
	if (overbrightBits < 0) overbrightBits = 0;
	if (overbrightBits > mapOverbrightBits) overbrightBits = mapOverbrightBits;
	lightingParams.modelLightDir[3] = static_cast<float>(overbrightBits);

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
			currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 3);

			// Bind normal-map (texture 1) and specular-map (texture 2), plus NormalSpecularParams.
			{
				NormalSpecularParams nsParams{};
				MTL::Texture* defaultTex = texMgr ? texMgr->getTexture(0) : nullptr;
				MTL::Texture* normalTex  = defaultTex;
				MTL::Texture* specTex    = defaultTex;
				if (stageRuntime.normalMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(stageRuntime.normalMapHandle);
					if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
				}
				if (stageRuntime.specularMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(stageRuntime.specularMapHandle);
					if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
				}
				if (normalTex) currentRenderEncoder_->setFragmentTexture(normalTex, 1);
				if (specTex)   currentRenderEncoder_->setFragmentTexture(specTex,   2);
				currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 4);
			}

			// Bind texture for this stage
			qhandle_t textureHandle = stageRuntime.primaryStageImage;
			if (textureHandle == 0 && shaderRes->primaryImageHandle > 0) {
				textureHandle = shaderRes->primaryImageHandle;
			}
			
			if (textureHandle > 0 && texMgr) {
				MTL::Texture* tex = texMgr->getTexture(textureHandle);
				if (tex) {
					currentRenderEncoder_->setFragmentTexture(tex, 0);
					// Use clamp sampler if shader stage specifies clampmap
					bool useClamp = stageRuntime.stageInfo && stageRuntime.stageInfo->clampMap;
					if (useClamp && sceneClampSampler_) {
						currentRenderEncoder_->setFragmentSamplerState(sceneClampSampler_.get(), 0);
					} else if (sceneSampler_) {
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
		currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 3);

		// Bind normal-map (texture 1) and specular-map (texture 2), plus NormalSpecularParams.
		{
			NormalSpecularParams nsParams{};
			MTL::Texture* defaultTex = texMgr ? texMgr->getTexture(0) : nullptr;
			MTL::Texture* normalTex  = defaultTex;
			MTL::Texture* specTex    = defaultTex;
			if (shaderRes && shaderRes->primaryStageIndex < shaderRes->stageRuntimes.size()) {
				const auto& primaryRuntime = shaderRes->stageRuntimes[shaderRes->primaryStageIndex];
				if (primaryRuntime.normalMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(primaryRuntime.normalMapHandle);
					if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
				}
				if (primaryRuntime.specularMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(primaryRuntime.specularMapHandle);
					if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
				}
			}
			if (normalTex) currentRenderEncoder_->setFragmentTexture(normalTex, 1);
			if (specTex)   currentRenderEncoder_->setFragmentTexture(specTex,   2);
			currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 4);
		}

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

	// --- DLIGHT PASS for animated MD3 models ---
	// Re-draw the surface additively for each dynamic light that overlaps this model.
	// dlightBits is a pre-computed bitmask from renderModel() with per-model sphere culling.
	if (dlightBits != 0
	    && surface.indexBuffer
	    && ensureDlightAnimatedResources()
	    && dlightAnimatedPipeline_
	    && dlightTexture_
	    && dlightDepthState_) {

		currentRenderEncoder_->setRenderPipelineState(dlightAnimatedPipeline_.get());
		currentRenderEncoder_->setDepthStencilState(dlightDepthState_.get());
		currentRenderEncoder_->setCullMode(MTL::CullModeFront);
		currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
		currentRenderEncoder_->setFragmentTexture(dlightTexture_.get(), 0);
		currentRenderEncoder_->setFragmentSamplerState(sampler2D_.get(), 0);

		DlightUniforms dlightUniforms{};
		std::memcpy(dlightUniforms.modelViewProjection, mvpMatrix, sizeof(float) * 16);
		dlightUniforms.deformGen = 0;
		dlightUniforms.time = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
		dlightUniforms.vertexLerp = vertexLerp;

		const int numLights = std::min(static_cast<int>(lightPackets_.size()), 32);
		MTL::Buffer* indexBuffer = static_cast<MTL::Buffer*>(surface.indexBuffer);

		for (int i = 0; i < numLights; i++) {
			if (!(dlightBits & (1u << i))) {
				continue;
			}

			const MetalSceneLight& light = lightPackets_[i].light;
			const float radius = light.intensity;
			if (radius <= 0.0f) {
				continue;
			}

			// Transform world-space light origin into model-local space.
			// Mirrors R_TransformDlights from renderergl2/tr_light.c.
			vec3_t temp;
			VectorSubtract(light.origin, ent.origin, temp);
			dlightUniforms.dlightInfo[0] = DotProduct(temp, ent.axis[0]);
			dlightUniforms.dlightInfo[1] = DotProduct(temp, ent.axis[1]);
			dlightUniforms.dlightInfo[2] = DotProduct(temp, ent.axis[2]);
			dlightUniforms.dlightInfo[3] = 1.0f / radius;

			dlightUniforms.color[0] = light.color[0];
			dlightUniforms.color[1] = light.color[1];
			dlightUniforms.color[2] = light.color[2];
			dlightUniforms.color[3] = 1.0f;

			currentRenderEncoder_->setVertexBytes(&dlightUniforms, sizeof(DlightUniforms), 1);
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

	// fullyVisible indicates model is entirely inside frustum - could be used to skip
	// per-surface clip checks, but current implementation doesn't need it since we
	// do frustum culling at the model level, not per-surface.
	(void)cullState;

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

	// Compute which dynamic lights overlap this model (bounding-sphere cull).
	// Only lights with a set bit will trigger a dlight additive pass per surface.
	const uint32_t dlightBits = computeModelDlightBits(ent, lodData);

	// Render each surface
	for (int i = 0; i < lodData.numSurfaces; i++) {
		renderModelSurface(ent, lodData.surfaces[i], mvpMatrix, modelMatrix, vertexLerp,
		                   fogParams, ambientLight, directedLight, lightDir, dlightBits);
	}

	// Projection (blob) shadow pass.
	// Matches GL2's r_shadows == 3 / RF_SHADOW_PLANE path in tr_mesh.c.
	// We show shadows whenever r_shadows >= 1 so the default setting works,
	// provided the client game has set RF_SHADOW_PLANE and a valid shadowPlane.
	const bool shadowsEnabled = r_shadows_ && r_shadows_->integer >= 1;
	const bool hasShadowPlane = (ent.renderfx & RF_SHADOW_PLANE) != 0;
	const bool noShadow       = (ent.renderfx & RF_NOSHADOW)     != 0;

	if (shadowsEnabled && hasShadowPlane && !noShadow
	    && ensureShadowPipeline()) {
		// Compute the model-space light direction (mirrors setupEntityLighting
		// which stores modelLightDir in the trRefEntity_t on the GL2 side).
		// Here we project the world-space lightDir into model space ourselves.
		vec3_t axis0, axis1, axis2;
		VectorCopy(ent.axis[0], axis0);
		VectorCopy(ent.axis[1], axis1);
		VectorCopy(ent.axis[2], axis2);

		// Normalize if non-normalised axes are used (e.g. scaled entities)
		if (ent.nonNormalizedAxes) {
			const float axisLength = VectorLength(axis0);
			const float invLength  = (axisLength > 0.0f) ? (1.0f / axisLength) : 1.0f;
			VectorScale(axis0, invLength, axis0);
			VectorScale(axis1, invLength, axis1);
			VectorScale(axis2, invLength, axis2);
		}

		vec3_t normalizedLightDir;
		VectorNormalize2(lightDir, normalizedLightDir);

		vec3_t modelLightDir;
		modelLightDir[0] = DotProduct(normalizedLightDir, axis0);
		modelLightDir[1] = DotProduct(normalizedLightDir, axis1);
		modelLightDir[2] = DotProduct(normalizedLightDir, axis2);

		for (int i = 0; i < lodData.numSurfaces; i++) {
			renderModelShadow(ent, lodData.surfaces[i], mvpMatrix, modelLightDir);
		}
	}
}

// ===========================================================================
// MDR SKELETAL ANIMATION RENDERING
// ===========================================================================

// registerMDRShaders: traverse all LODs in an MDR model and register each
// surface's shader with the renderer, storing the handle in surf->shaderIndex.
void MetalRenderer::registerMDRShaders(MetalModel* model)
{
	if (!model || !model->mdrData) {
		return;
	}
	const mdrHeader_t* mdr = (const mdrHeader_t*)model->mdrData;
	const mdrLOD_t* lod = (const mdrLOD_t*)((const byte*)mdr + mdr->ofsLODs);

	for (int l = 0; l < mdr->numLODs; l++) {
		mdrSurface_t* surf = (mdrSurface_t*)((byte*)lod + lod->ofsSurfaces);
		for (int i = 0; i < lod->numSurfaces; i++) {
			qhandle_t sh = registerShader(surf->shader, true);
			surf->shaderIndex = sh;
			surf = (mdrSurface_t*)((byte*)surf + surf->ofsEnd);
		}
		lod = (const mdrLOD_t*)((const byte*)lod + lod->ofsEnd);
	}
}

// renderMDRSurface: skin one MDR surface on the CPU using the pre-lerped bone
// palette, upload the result as a Metal vertex buffer, and render via the
// existing model pipeline so all normal shader/tcMod/fog logic applies.
void MetalRenderer::renderMDRSurface(
	const refEntity_t& ent,
	const mdrHeader_t* header,
	const mdrSurface_t* surf,
	const mdrBone_t* bonePtr,
	const float* mvpMatrix,
	const float* modelMatrix,
	const ModelFogParams& fogParams,
	const vec3_t ambientLight,
	const vec3_t directedLight,
	const vec3_t lightDir)
{
	if (!currentRenderEncoder_ || !modelPipeline_ || !modelDepthState_ || !device_) {
		return;
	}

	const int numVerts = surf->numVerts;
	const int numTris  = surf->numTriangles;

	// -----------------------------------------------------------------
	// 1. CPU skinning: produce one skinned vertex per input vertex.
	//    Layout matches the model vertex descriptor (14 floats / 56 bytes):
	//      [0..2]  position    (old frame, == skinned)
	//      [3..5]  normal      (old frame, == skinned)
	//      [6..7]  texCoord
	//      [8..10] position2   (new frame, == skinned, no GPU lerp needed)
	//      [11..13] normal2    (new frame, == skinned)
	// -----------------------------------------------------------------
	struct MDRVert {
		float pos[3];
		float nrm[3];
		float tc[2];
		float pos2[3];
		float nrm2[3];
	};
	static_assert(sizeof(MDRVert) == 56, "MDRVert stride mismatch");

	std::vector<MDRVert> verts(numVerts);

	const mdrVertex_t* v = (const mdrVertex_t*)((const byte*)surf + surf->ofsVerts);
	for (int j = 0; j < numVerts; j++) {
		float tp[3] = {0.f, 0.f, 0.f};
		float tn[3] = {0.f, 0.f, 0.f};

		const mdrWeight_t* w = v->weights;
		for (int k = 0; k < v->numWeights; k++, w++) {
			const mdrBone_t* bone = bonePtr + w->boneIndex;
			const float bw  = w->boneWeight;
			const float ox  = w->offset[0];
			const float oy  = w->offset[1];
			const float oz  = w->offset[2];

			tp[0] += bw * (bone->matrix[0][0]*ox + bone->matrix[0][1]*oy + bone->matrix[0][2]*oz + bone->matrix[0][3]);
			tp[1] += bw * (bone->matrix[1][0]*ox + bone->matrix[1][1]*oy + bone->matrix[1][2]*oz + bone->matrix[1][3]);
			tp[2] += bw * (bone->matrix[2][0]*ox + bone->matrix[2][1]*oy + bone->matrix[2][2]*oz + bone->matrix[2][3]);

			const float nx = v->normal[0];
			const float ny = v->normal[1];
			const float nz = v->normal[2];
			tn[0] += bw * (bone->matrix[0][0]*nx + bone->matrix[0][1]*ny + bone->matrix[0][2]*nz);
			tn[1] += bw * (bone->matrix[1][0]*nx + bone->matrix[1][1]*ny + bone->matrix[1][2]*nz);
			tn[2] += bw * (bone->matrix[2][0]*nx + bone->matrix[2][1]*ny + bone->matrix[2][2]*nz);
		}

		MDRVert& out = verts[j];
		out.pos[0] = out.pos2[0] = tp[0];
		out.pos[1] = out.pos2[1] = tp[1];
		out.pos[2] = out.pos2[2] = tp[2];
		out.nrm[0] = out.nrm2[0] = tn[0];
		out.nrm[1] = out.nrm2[1] = tn[1];
		out.nrm[2] = out.nrm2[2] = tn[2];
		out.tc[0]  = v->texCoords[0];
		out.tc[1]  = v->texCoords[1];

		v = (const mdrVertex_t*)&v->weights[v->numWeights];
	}

	const size_t vertBufSize = verts.size() * sizeof(MDRVert);
	MTL::Buffer* vertexBuffer = device_->newBuffer(
		verts.data(), vertBufSize, MTL::ResourceStorageModeShared);
	if (!vertexBuffer) {
		return;
	}

	// -----------------------------------------------------------------
	// 2. Obtain (or build and cache) the index buffer for this surface.
	//    Triangles don't change between frames so we cache by surface addr.
	// -----------------------------------------------------------------
	const uintptr_t indexKey = reinterpret_cast<uintptr_t>(surf);
	MTL::Buffer* indexBuffer = nullptr;

	auto it = mdrIndexBuffers_.find(indexKey);
	if (it != mdrIndexBuffers_.end()) {
		indexBuffer = it->second.get();
	} else {
		std::vector<unsigned int> indexes(numTris * 3);
		const mdrTriangle_t* tri = (const mdrTriangle_t*)((const byte*)surf + surf->ofsTriangles);
		for (int j = 0; j < numTris; j++, tri++) {
			indexes[j*3 + 0] = (unsigned int)tri->indexes[0];
			indexes[j*3 + 1] = (unsigned int)tri->indexes[1];
			indexes[j*3 + 2] = (unsigned int)tri->indexes[2];
		}
		const size_t idxSize = indexes.size() * sizeof(unsigned int);
		MTL::Buffer* ib = device_->newBuffer(
			indexes.data(), idxSize, MTL::ResourceStorageModeShared);
		if (ib) {
			mdrIndexBuffers_[indexKey] = MetalPtr<MTL::Buffer>(ib);
			indexBuffer = ib;
		}
	}

	if (!indexBuffer) {
		vertexBuffer->release();
		return;
	}

	const int numIndexes = numTris * 3;

	// -----------------------------------------------------------------
	// 3. Shader / skin selection (same priority as renderModelSurface)
	// -----------------------------------------------------------------
	qhandle_t shaderHandle = 0;

	if (ent.customShader) {
		shaderHandle = ent.customShader;
	} else if (ent.customSkin > 0) {
		MetalSkin* skin = getSkinByHandle(ent.customSkin);
		if (skin) {
			char surfName[MAX_QPATH];
			Q_strncpyz(surfName, surf->name, sizeof(surfName));
			Q_strlwr(surfName);
			for (int j = 0; j < skin->numSurfaces; j++) {
				if (strcmp(skin->surfaces[j].name, surfName) == 0) {
					shaderHandle = skin->surfaces[j].shader;
					break;
				}
			}
		}
	}

	if (!shaderHandle) {
		shaderHandle = surf->shaderIndex;
	}
	if (!shaderHandle) {
		shaderHandle = registerShader("white", true);
	}

	const MetalShaderResource* shaderRes = nullptr;
	if (shaderHandle > 0 && static_cast<size_t>(shaderHandle) < shaderResources_.size()) {
		shaderRes = &shaderResources_[shaderHandle];
	}

	// -----------------------------------------------------------------
	// 4. Build uniform blocks (identical to renderModelSurface)
	// -----------------------------------------------------------------
	struct ModelUniforms {
		float mvpMatrix[16];
		float modelMatrix[16];
		float vertexLerp;
		float padding[3];
	};
	ModelUniforms uniforms{};
	std::memcpy(uniforms.mvpMatrix,   mvpMatrix,   sizeof(float) * 16);
	std::memcpy(uniforms.modelMatrix, modelMatrix, sizeof(float) * 16);
	uniforms.vertexLerp = 1.0f; // CPU-skinned: no GPU lerp needed

	EntityLightingParams lightingParams{};
	VectorScale(ambientLight,  1.0f / 255.0f, lightingParams.ambientLight);
	lightingParams.ambientLight[3] = 0.0f;
	VectorScale(directedLight, 1.0f / 255.0f, lightingParams.directedLight);
	lightingParams.directedLight[3] = 0.0f;
	vec3_t normalizedLightDir;
	VectorNormalize2(lightDir, normalizedLightDir);
	VectorCopy(normalizedLightDir, lightingParams.lightDir);
	lightingParams.lightDir[3] = 0.0f;

	vec3_t axis0, axis1, axis2;
	VectorCopy(ent.axis[0], axis0);
	VectorCopy(ent.axis[1], axis1);
	VectorCopy(ent.axis[2], axis2);
	if (ent.nonNormalizedAxes) {
		const float axisLength = VectorLength(axis0);
		const float invLen = (axisLength > 0.0f) ? (1.0f / axisLength) : 1.0f;
		VectorScale(axis0, invLen, axis0);
		VectorScale(axis1, invLen, axis1);
		VectorScale(axis2, invLen, axis2);
	}
	lightingParams.modelLightDir[0] = DotProduct(normalizedLightDir, axis0);
	lightingParams.modelLightDir[1] = DotProduct(normalizedLightDir, axis1);
	lightingParams.modelLightDir[2] = DotProduct(normalizedLightDir, axis2);

	int overbrightBits    = r_overBrightBits_    ? r_overBrightBits_->integer    : 1;
	int mapOverbrightBits = r_mapOverBrightBits_ ? r_mapOverBrightBits_->integer : 2;
	if (overbrightBits > 2) overbrightBits = 2;
	if (overbrightBits < 0) overbrightBits = 0;
	if (overbrightBits > mapOverbrightBits) overbrightBits = mapOverbrightBits;
	lightingParams.modelLightDir[3] = static_cast<float>(overbrightBits);

	currentRenderEncoder_->setCullMode(MTL::CullModeFront);

	TextureManager* texMgr = ensureTextureManager();
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;

	// -----------------------------------------------------------------
	// 5. Multi-pass rendering (identical structure to renderModelSurface)
	// -----------------------------------------------------------------
	const size_t numStages = (shaderRes && !shaderRes->stageRuntimes.empty())
	                          ? shaderRes->stageRuntimes.size() : 0;

	if (numStages > 0) {
		for (size_t stageIndex = 0; stageIndex < numStages; stageIndex++) {
			const auto& stageRuntime = shaderRes->stageRuntimes[stageIndex];
			ModelStagePipelineEntry* stagePipeline = getModelStagePipeline(stageRuntime.pipelineKey);
			if (!stagePipeline || !stagePipeline->pipeline) {
				continue;
			}

			currentRenderEncoder_->setRenderPipelineState(stagePipeline->pipeline.get());
			currentRenderEncoder_->setDepthStencilState(stagePipeline->depthState.get());
			currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
			currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(ModelUniforms), 1);
			currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);

			ModelStageParams stageParams{};
			if (stageRuntime.stageInfo) {
				switch (stageRuntime.stageInfo->tcGen.type) {
					case MetalTCGen::Lightmap:     stageParams.tcGenType = 1.0f; break;
					case MetalTCGen::Environment:  stageParams.tcGenType = 2.0f; break;
					default:                       stageParams.tcGenType = 0.0f; break;
				}
				TCModParams tcMod = computeTCModParams(stageRuntime.stageInfo, sceneTimeSeconds);
				std::memcpy(stageParams.texMatrix0, tcMod.texMatrix0, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix1, tcMod.texMatrix1, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix2, tcMod.texMatrix2, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix3, tcMod.texMatrix3, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix4, tcMod.texMatrix4, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix5, tcMod.texMatrix5, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix6, tcMod.texMatrix6, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix7, tcMod.texMatrix7, sizeof(float) * 4);
			}

			currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 0);
			currentRenderEncoder_->setFragmentBytes(&fogParams,       sizeof(ModelFogParams),       1);
			currentRenderEncoder_->setFragmentBytes(&stageParams,     sizeof(ModelStageParams),     2);
			currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 3);

			// Bind normal/specular maps for this stage.
			{
				NormalSpecularParams nsParams{};
				MTL::Texture* defaultTex_ = texMgr ? texMgr->getTexture(0) : nullptr;
				MTL::Texture* normalTex  = defaultTex_;
				MTL::Texture* specTex    = defaultTex_;
				if (stageRuntime.normalMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(stageRuntime.normalMapHandle);
					if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
				}
				if (stageRuntime.specularMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(stageRuntime.specularMapHandle);
					if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
				}
				if (normalTex) currentRenderEncoder_->setFragmentTexture(normalTex, 1);
				if (specTex)   currentRenderEncoder_->setFragmentTexture(specTex,   2);
				currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 4);
			}

			qhandle_t textureHandle = stageRuntime.primaryStageImage;
			if (!textureHandle && shaderRes->primaryImageHandle > 0) {
				textureHandle = shaderRes->primaryImageHandle;
			}
			if (textureHandle > 0 && texMgr) {
				MTL::Texture* tex = texMgr->getTexture(textureHandle);
				if (tex) {
					currentRenderEncoder_->setFragmentTexture(tex, 0);
					bool useClamp = stageRuntime.stageInfo && stageRuntime.stageInfo->clampMap;
					MTL::SamplerState* samp = (useClamp && sceneClampSampler_)
					                          ? sceneClampSampler_.get() : sceneSampler_.get();
					currentRenderEncoder_->setFragmentSamplerState(samp, 0);
				}
			}

			currentRenderEncoder_->drawIndexedPrimitives(
				MTL::PrimitiveTypeTriangle, numIndexes, MTL::IndexTypeUInt32, indexBuffer, 0);
		}
	} else {
		// Single-pass fallback
		currentRenderEncoder_->setRenderPipelineState(modelPipeline_.get());
		currentRenderEncoder_->setDepthStencilState(modelDepthState_.get());
		currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
		currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(ModelUniforms), 1);
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);

		ModelStageParams stageParams{};
		currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 0);
		currentRenderEncoder_->setFragmentBytes(&fogParams,       sizeof(ModelFogParams),       1);
		currentRenderEncoder_->setFragmentBytes(&stageParams,     sizeof(ModelStageParams),     2);
		currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 3);

		// Bind no-op NormalSpecularParams for single-pass fallback.
		{
			NormalSpecularParams nsParams{};
			MTL::Texture* defaultTex_ = texMgr ? texMgr->getTexture(0) : nullptr;
			if (defaultTex_) currentRenderEncoder_->setFragmentTexture(defaultTex_, 1);
			if (defaultTex_) currentRenderEncoder_->setFragmentTexture(defaultTex_, 2);
			currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 4);
		}

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

		currentRenderEncoder_->drawIndexedPrimitives(
			MTL::PrimitiveTypeTriangle, numIndexes, MTL::IndexTypeUInt32, indexBuffer, 0);
	}

	vertexBuffer->release();
}

// renderMDRModel: cull, select LOD, lerp bones, then call renderMDRSurface
// for each surface in the chosen LOD level.
void MetalRenderer::renderMDRModel(const refEntity_t& ent, MetalModel& model)
{
	if (!model.mdrData) {
		return;
	}

	const mdrHeader_t* header = (const mdrHeader_t*)model.mdrData;
	if (header->numFrames < 1 || header->numLODs < 1) {
		return;
	}

	// Clamp / wrap frame indices
	int frameIdx    = ent.frame;
	int oldFrameIdx = ent.oldframe;
	if (ent.renderfx & RF_WRAP_FRAMES) {
		frameIdx    = ((frameIdx    % header->numFrames) + header->numFrames) % header->numFrames;
		oldFrameIdx = ((oldFrameIdx % header->numFrames) + header->numFrames) % header->numFrames;
	}
	if (frameIdx    < 0 || frameIdx    >= header->numFrames) frameIdx    = 0;
	if (oldFrameIdx < 0 || oldFrameIdx >= header->numFrames) oldFrameIdx = 0;

	const int frameSize = (int)(sizeof(mdrFrame_t) + (size_t)(header->numBones - 1) * sizeof(mdrBone_t));
	const mdrFrame_t* newFrame =
		(const mdrFrame_t*)((const byte*)header + header->ofsFrames + frameSize * frameIdx);
	const mdrFrame_t* oldFrame =
		(const mdrFrame_t*)((const byte*)header + header->ofsFrames + frameSize * oldFrameIdx);

	// Sphere + box culling (mirrors R_MDRCullModel in renderergl2/tr_animation.c)
	if (!ent.nonNormalizedAxes) {
		if (frameIdx == oldFrameIdx) {
			vec3_t center;
			transformModelPoint(ent, newFrame->localOrigin, center);
			if (cullBoundingSphere(center, newFrame->radius) == CullResult::Out) {
				return;
			}
		} else {
			vec3_t newCenter, oldCenter;
			transformModelPoint(ent, newFrame->localOrigin, newCenter);
			transformModelPoint(ent, oldFrame->localOrigin, oldCenter);
			const CullResult cn = cullBoundingSphere(newCenter, newFrame->radius);
			const CullResult co = cullBoundingSphere(oldCenter, oldFrame->radius);
			if (cn == CullResult::Out && co == CullResult::Out) {
				return;
			}
		}
	}

	vec3_t mergedMins, mergedMaxs;
	for (int i = 0; i < 3; i++) {
		mergedMins[i] = std::min(oldFrame->bounds[0][i], newFrame->bounds[0][i]);
		mergedMaxs[i] = std::max(oldFrame->bounds[1][i], newFrame->bounds[1][i]);
	}
	if (cullBoundingBox(mergedMins, mergedMaxs, ent) == CullResult::Out) {
		return;
	}

	// LOD selection (mirrors computeModelLod but uses MDR frame data directly)
	int lodnum = 0;
	if (header->numLODs > 1) {
		float projectedRadius = projectRadius(newFrame->radius, ent.origin);
		float flod;
		if (projectedRadius != 0.0f) {
			float lodscale = r_lodScale_ ? r_lodScale_->value : 1.0f;
			if (lodscale > 20.0f) lodscale = 20.0f;
			if (lodscale < 0.0f)  lodscale = 0.0f;
			flod = 1.0f - projectedRadius * lodscale;
		} else {
			flod = 0.0f;
		}
		flod   *= (float)header->numLODs;
		lodnum  = (int)flod;
		if (lodnum < 0) lodnum = 0;
		if (lodnum >= header->numLODs) lodnum = header->numLODs - 1;
		if (r_lodBias_) {
			lodnum += r_lodBias_->integer;
			if (lodnum < 0) lodnum = 0;
			if (lodnum >= header->numLODs) lodnum = header->numLODs - 1;
		}
	}

	// Navigate to selected LOD
	const mdrLOD_t* lod = (const mdrLOD_t*)((const byte*)header + header->ofsLODs);
	for (int i = 0; i < lodnum; i++) {
		lod = (const mdrLOD_t*)((const byte*)lod + lod->ofsEnd);
	}

	// Fog index (mirrors R_MDRComputeFogNum in renderergl2/tr_animation.c)
	int fogIndex = 0;
	if (!(sceneCamera_.refdef.rdflags & RDF_NOWORLDMODEL)) {
		vec3_t worldOrigin;
		VectorAdd(ent.origin, newFrame->localOrigin, worldOrigin);
		for (size_t fi = 0; fi < worldFogs_.size(); fi++) {
			const FogVolume& fog = worldFogs_[fi];
			bool inside = true;
			for (int ax = 0; ax < 3; ax++) {
				if (worldOrigin[ax] - newFrame->radius >= fog.bounds[1][ax]) { inside = false; break; }
				if (worldOrigin[ax] + newFrame->radius <= fog.bounds[0][ax]) { inside = false; break; }
			}
			if (inside) { fogIndex = (int)fi + 1; break; }
		}
	}
	const ModelFogParams fogParams = buildModelFogParams(fogIndex);

	// Entity lighting
	vec3_t ambientLight, directedLight, lightDir;
	setupEntityLighting(ent, ambientLight, directedLight, lightDir);

	// Transform matrices
	float modelMatrix[16];
	calculateEntityTransform(ent, modelMatrix);
	float mvpMatrix[16];
	multiplyMatrices4x4(sceneCamera_.projectionMatrix, sceneCamera_.viewMatrix, modelMatrix, mvpMatrix);

	// Bone lerp (mirrors RB_MDRSurfaceAnim in renderergl2/tr_animation.c)
	const float backlerp  = ent.backlerp;
	const float frontlerp = 1.0f - backlerp;

	std::vector<mdrBone_t> lerpedBones(header->numBones);
	if (backlerp == 0.0f || frameIdx == oldFrameIdx) {
		std::memcpy(lerpedBones.data(), newFrame->bones,
		            header->numBones * sizeof(mdrBone_t));
	} else {
		for (int i = 0; i < header->numBones * 12; i++) {
			((float*)lerpedBones.data())[i] =
				frontlerp * ((const float*)newFrame->bones)[i] +
				backlerp  * ((const float*)oldFrame->bones)[i];
		}
	}

	// Ensure model pipeline is set up
	if (!ensureModelPipeline() || !modelPipeline_) {
		return;
	}

	// Render each surface in the selected LOD
	const mdrSurface_t* surf = (const mdrSurface_t*)((const byte*)lod + lod->ofsSurfaces);
	for (int i = 0; i < lod->numSurfaces; i++) {
		renderMDRSurface(ent, header, surf, lerpedBones.data(),
		                 mvpMatrix, modelMatrix, fogParams,
		                 ambientLight, directedLight, lightDir);
		surf = (const mdrSurface_t*)((const byte*)surf + surf->ofsEnd);
	}
}

// ===========================================================================
// IQM SKELETAL ANIMATION RENDERING
// ===========================================================================

// registerIQMShaders: walk all surfaces in an IQM model and register each
// surface's material string with the renderer.
void MetalRenderer::registerIQMShaders(MetalModel* model)
{
	if (!model || !model->iqmData) return;
	MetalIQMData* data = (MetalIQMData*)model->iqmData;
	for (int i = 0; i < data->num_surfaces; i++) {
		MetalIQMSurface* surf = &data->surfaces[i];
		if (surf->materialName[0]) {
			surf->shaderIndex = registerShader(surf->materialName, true);
		}
	}
}

// renderIQMSurface: CPU-skin one IQM surface for the current frame pair,
// upload the result as a Metal shared buffer, and draw it using the same
// model pipeline as MDR so all shader/tcMod/fog logic applies.
void MetalRenderer::renderIQMSurface(
	const refEntity_t& ent,
	MetalIQMData* data,
	const MetalIQMSurface* surf,
	int frame, int oldframe, float backlerp,
	const float* mvpMatrix, const float* modelMatrix,
	const ModelFogParams& fogParams,
	const vec3_t ambientLight, const vec3_t directedLight,
	const vec3_t lightDir)
{
	if (!currentRenderEncoder_ || !modelPipeline_ || !modelDepthState_ || !device_) {
		return;
	}

	const int numVerts = surf->num_vertexes;
	const int numTris  = surf->num_triangles;

	// -----------------------------------------------------------------
	// 1. CPU skinning – produce MetalIQMVert (== 56 bytes, MDRVert-compatible)
	// -----------------------------------------------------------------
	std::vector<MetalIQMVert> verts(numVerts);
	Metal_IQMSurfaceAnim(data, surf, frame, oldframe, backlerp, verts.data());

	const size_t vertBufSize = verts.size() * sizeof(MetalIQMVert);
	MTL::Buffer* vertexBuffer = device_->newBuffer(
		verts.data(), vertBufSize, MTL::ResourceStorageModeShared);
	if (!vertexBuffer) return;

	// -----------------------------------------------------------------
	// 2. Index buffer – cached by surface pointer (triangles are static)
	// -----------------------------------------------------------------
	const uintptr_t indexKey = reinterpret_cast<uintptr_t>(surf);
	MTL::Buffer* indexBuffer = nullptr;

	auto it = iqmIndexBuffers_.find(indexKey);
	if (it != iqmIndexBuffers_.end()) {
		indexBuffer = it->second.get();
	} else {
		std::vector<unsigned int> indexes(numTris * 3);
		const int* tri = &data->triangles[surf->first_triangle * 3];
		for (int j = 0; j < numTris; j++) {
			indexes[j*3 + 0] = (unsigned int)(tri[j*3 + 0] - surf->first_vertex);
			indexes[j*3 + 1] = (unsigned int)(tri[j*3 + 1] - surf->first_vertex);
			indexes[j*3 + 2] = (unsigned int)(tri[j*3 + 2] - surf->first_vertex);
		}
		const size_t idxSize = indexes.size() * sizeof(unsigned int);
		MTL::Buffer* ib = device_->newBuffer(
			indexes.data(), idxSize, MTL::ResourceStorageModeShared);
		if (ib) {
			iqmIndexBuffers_[indexKey] = MetalPtr<MTL::Buffer>(ib);
			indexBuffer = ib;
		}
	}

	if (!indexBuffer) {
		vertexBuffer->release();
		return;
	}

	const int numIndexes = numTris * 3;

	// -----------------------------------------------------------------
	// 3. Shader / skin selection (same priority as renderMDRSurface)
	// -----------------------------------------------------------------
	qhandle_t shaderHandle = 0;
	if (ent.customShader) {
		shaderHandle = ent.customShader;
	} else if (ent.customSkin > 0) {
		MetalSkin* skin = getSkinByHandle(ent.customSkin);
		if (skin) {
			char surfName[MAX_QPATH];
			Q_strncpyz(surfName, surf->name, sizeof(surfName));
			Q_strlwr(surfName);
			for (int j = 0; j < skin->numSurfaces; j++) {
				if (strcmp(skin->surfaces[j].name, surfName) == 0) {
					shaderHandle = skin->surfaces[j].shader;
					break;
				}
			}
		}
	}
	if (!shaderHandle) shaderHandle = surf->shaderIndex;
	if (!shaderHandle) shaderHandle = registerShader("white", true);

	const MetalShaderResource* shaderRes = nullptr;
	if (shaderHandle > 0 && static_cast<size_t>(shaderHandle) < shaderResources_.size()) {
		shaderRes = &shaderResources_[shaderHandle];
	}

	// -----------------------------------------------------------------
	// 4. Uniform blocks (identical to renderMDRSurface)
	// -----------------------------------------------------------------
	struct ModelUniforms {
		float mvpMatrix[16];
		float modelMatrix[16];
		float vertexLerp;
		float padding[3];
	};
	ModelUniforms uniforms{};
	std::memcpy(uniforms.mvpMatrix,   mvpMatrix,   sizeof(float) * 16);
	std::memcpy(uniforms.modelMatrix, modelMatrix, sizeof(float) * 16);
	uniforms.vertexLerp = 1.0f; // CPU-skinned: pos == pos2, no GPU lerp needed

	EntityLightingParams lightingParams{};
	VectorScale(ambientLight,  1.0f / 255.0f, lightingParams.ambientLight);
	lightingParams.ambientLight[3] = 0.0f;
	VectorScale(directedLight, 1.0f / 255.0f, lightingParams.directedLight);
	lightingParams.directedLight[3] = 0.0f;
	vec3_t normalizedLightDir;
	VectorNormalize2(lightDir, normalizedLightDir);
	VectorCopy(normalizedLightDir, lightingParams.lightDir);
	lightingParams.lightDir[3] = 0.0f;

	vec3_t axis0, axis1, axis2;
	VectorCopy(ent.axis[0], axis0);
	VectorCopy(ent.axis[1], axis1);
	VectorCopy(ent.axis[2], axis2);
	if (ent.nonNormalizedAxes) {
		const float axisLength = VectorLength(axis0);
		const float invLen = (axisLength > 0.0f) ? (1.0f / axisLength) : 1.0f;
		VectorScale(axis0, invLen, axis0);
		VectorScale(axis1, invLen, axis1);
		VectorScale(axis2, invLen, axis2);
	}
	lightingParams.modelLightDir[0] = DotProduct(normalizedLightDir, axis0);
	lightingParams.modelLightDir[1] = DotProduct(normalizedLightDir, axis1);
	lightingParams.modelLightDir[2] = DotProduct(normalizedLightDir, axis2);

	int overbrightBits    = r_overBrightBits_    ? r_overBrightBits_->integer    : 1;
	int mapOverbrightBits = r_mapOverBrightBits_ ? r_mapOverBrightBits_->integer : 2;
	if (overbrightBits > 2) overbrightBits = 2;
	if (overbrightBits < 0) overbrightBits = 0;
	if (overbrightBits > mapOverbrightBits) overbrightBits = mapOverbrightBits;
	lightingParams.modelLightDir[3] = static_cast<float>(overbrightBits);

	currentRenderEncoder_->setCullMode(MTL::CullModeFront);

	TextureManager* texMgr = ensureTextureManager();
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;

	// -----------------------------------------------------------------
	// 5. Multi-pass rendering (same structure as renderMDRSurface)
	// -----------------------------------------------------------------
	const size_t numStages = (shaderRes && !shaderRes->stageRuntimes.empty())
	                          ? shaderRes->stageRuntimes.size() : 0;

	if (numStages > 0) {
		for (size_t stageIndex = 0; stageIndex < numStages; stageIndex++) {
			const auto& stageRuntime = shaderRes->stageRuntimes[stageIndex];
			ModelStagePipelineEntry* stagePipeline = getModelStagePipeline(stageRuntime.pipelineKey);
			if (!stagePipeline || !stagePipeline->pipeline) continue;

			currentRenderEncoder_->setRenderPipelineState(stagePipeline->pipeline.get());
			currentRenderEncoder_->setDepthStencilState(stagePipeline->depthState.get());
			currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
			currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(ModelUniforms), 1);
			currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);

			ModelStageParams stageParams{};
			if (stageRuntime.stageInfo) {
				switch (stageRuntime.stageInfo->tcGen.type) {
					case MetalTCGen::Lightmap:    stageParams.tcGenType = 1.0f; break;
					case MetalTCGen::Environment: stageParams.tcGenType = 2.0f; break;
					default:                      stageParams.tcGenType = 0.0f; break;
				}
				TCModParams tcMod = computeTCModParams(stageRuntime.stageInfo, sceneTimeSeconds);
				std::memcpy(stageParams.texMatrix0, tcMod.texMatrix0, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix1, tcMod.texMatrix1, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix2, tcMod.texMatrix2, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix3, tcMod.texMatrix3, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix4, tcMod.texMatrix4, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix5, tcMod.texMatrix5, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix6, tcMod.texMatrix6, sizeof(float) * 4);
				std::memcpy(stageParams.texMatrix7, tcMod.texMatrix7, sizeof(float) * 4);
			}

			currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 0);
			currentRenderEncoder_->setFragmentBytes(&fogParams,       sizeof(ModelFogParams),       1);
			currentRenderEncoder_->setFragmentBytes(&stageParams,     sizeof(ModelStageParams),     2);
			currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 3);

			// Bind normal/specular maps for this stage.
			{
				NormalSpecularParams nsParams{};
				MTL::Texture* defaultTex_ = texMgr ? texMgr->getTexture(0) : nullptr;
				MTL::Texture* normalTex  = defaultTex_;
				MTL::Texture* specTex    = defaultTex_;
				if (stageRuntime.normalMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(stageRuntime.normalMapHandle);
					if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
				}
				if (stageRuntime.specularMapHandle > 0 && texMgr) {
					MTL::Texture* t = texMgr->getTexture(stageRuntime.specularMapHandle);
					if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
				}
				if (normalTex) currentRenderEncoder_->setFragmentTexture(normalTex, 1);
				if (specTex)   currentRenderEncoder_->setFragmentTexture(specTex,   2);
				currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 4);
			}

			qhandle_t textureHandle = stageRuntime.primaryStageImage;
			if (!textureHandle && shaderRes->primaryImageHandle > 0)
				textureHandle = shaderRes->primaryImageHandle;
			if (textureHandle > 0 && texMgr) {
				MTL::Texture* tex = texMgr->getTexture(textureHandle);
				if (tex) {
					currentRenderEncoder_->setFragmentTexture(tex, 0);
					bool useClamp = stageRuntime.stageInfo && stageRuntime.stageInfo->clampMap;
					MTL::SamplerState* samp = (useClamp && sceneClampSampler_)
					                          ? sceneClampSampler_.get() : sceneSampler_.get();
					currentRenderEncoder_->setFragmentSamplerState(samp, 0);
				}
			}

			currentRenderEncoder_->drawIndexedPrimitives(
				MTL::PrimitiveTypeTriangle, numIndexes, MTL::IndexTypeUInt32, indexBuffer, 0);
		}
	} else {
		// Single-pass fallback
		currentRenderEncoder_->setRenderPipelineState(modelPipeline_.get());
		currentRenderEncoder_->setDepthStencilState(modelDepthState_.get());
		currentRenderEncoder_->setVertexBuffer(vertexBuffer, 0, 0);
		currentRenderEncoder_->setVertexBytes(&uniforms, sizeof(ModelUniforms), 1);
		currentRenderEncoder_->setVertexBuffer(sceneUniformBuffer_.get(), 0, 2);

		ModelStageParams stageParams{};
		currentRenderEncoder_->setFragmentBytes(&lightingParams, sizeof(EntityLightingParams), 0);
		currentRenderEncoder_->setFragmentBytes(&fogParams,       sizeof(ModelFogParams),       1);
		currentRenderEncoder_->setFragmentBytes(&stageParams,     sizeof(ModelStageParams),     2);
		currentRenderEncoder_->setFragmentBuffer(sceneUniformBuffer_.get(), 0, 3);

		// Bind no-op NormalSpecularParams for single-pass fallback.
		{
			NormalSpecularParams nsParams{};
			MTL::Texture* defaultTex_ = texMgr ? texMgr->getTexture(0) : nullptr;
			if (defaultTex_) currentRenderEncoder_->setFragmentTexture(defaultTex_, 1);
			if (defaultTex_) currentRenderEncoder_->setFragmentTexture(defaultTex_, 2);
			currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 4);
		}

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

		currentRenderEncoder_->drawIndexedPrimitives(
			MTL::PrimitiveTypeTriangle, numIndexes, MTL::IndexTypeUInt32, indexBuffer, 0);
	}

	vertexBuffer->release();
}

// renderIQMModel: cull, validate frames, then call renderIQMSurface for each
// surface.  IQM has no LOD concept – all surfaces are rendered.
void MetalRenderer::renderIQMModel(const refEntity_t& ent, MetalModel& model)
{
	if (!model.iqmData) return;
	MetalIQMData* data = (MetalIQMData*)model.iqmData;
	if (data->num_surfaces < 1) return;

	// Clamp / wrap frame indices
	int frame    = ent.frame;
	int oldframe = ent.oldframe;
	if (data->num_frames > 0) {
		if (ent.renderfx & RF_WRAP_FRAMES) {
			frame    = ((frame    % data->num_frames) + data->num_frames) % data->num_frames;
			oldframe = ((oldframe % data->num_frames) + data->num_frames) % data->num_frames;
		}
		if (frame    < 0 || frame    >= data->num_frames) frame    = 0;
		if (oldframe < 0 || oldframe >= data->num_frames) oldframe = 0;
	} else {
		frame = oldframe = 0;
	}

	// Bounding-box culling from per-frame bounds
	if (data->bounds) {
		const float* bOld = data->bounds + 6 * oldframe;
		const float* bNew = data->bounds + 6 * frame;
		float mins[3], maxs[3];
		for (int i = 0; i < 3; i++) {
			mins[i] = std::min(bOld[i],   bNew[i]);
			maxs[i] = std::max(bOld[i+3], bNew[i+3]);
		}
		if (cullBoundingBox(mins, maxs, ent) == CullResult::Out) return;
	}

	// Fog index (same approach as MDR)
	int fogIndex = 0;
	if (!(sceneCamera_.refdef.rdflags & RDF_NOWORLDMODEL) && data->bounds) {
		const float* b = data->bounds + 6 * frame;
		vec3_t diag, center, worldOrigin;
		for (int i = 0; i < 3; i++) {
			diag[i]   = b[i+3] - b[i];
			center[i] = b[i] + 0.5f * diag[i];
		}
		VectorAdd(ent.origin, center, worldOrigin);
		const float radius = 0.5f * VectorLength(diag);
		for (size_t fi = 0; fi < worldFogs_.size(); fi++) {
			const FogVolume& fog = worldFogs_[fi];
			bool inside = true;
			for (int ax = 0; ax < 3; ax++) {
				if (worldOrigin[ax] - radius >= fog.bounds[1][ax]) { inside = false; break; }
				if (worldOrigin[ax] + radius <= fog.bounds[0][ax]) { inside = false; break; }
			}
			if (inside) { fogIndex = (int)fi + 1; break; }
		}
	}
	const ModelFogParams fogParams = buildModelFogParams(fogIndex);

	// Entity lighting
	vec3_t ambientLight, directedLight, lightDir;
	setupEntityLighting(ent, ambientLight, directedLight, lightDir);

	// Transform matrices
	float modelMatrix[16];
	calculateEntityTransform(ent, modelMatrix);
	float mvpMatrix[16];
	multiplyMatrices4x4(sceneCamera_.projectionMatrix, sceneCamera_.viewMatrix, modelMatrix, mvpMatrix);

	if (!ensureModelPipeline() || !modelPipeline_) return;

	for (int i = 0; i < data->num_surfaces; i++) {
		renderIQMSurface(ent, data, &data->surfaces[i],
		                 frame, oldframe, ent.backlerp,
		                 mvpMatrix, modelMatrix, fogParams,
		                 ambientLight, directedLight, lightDir);
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
	
	// Build modified uniforms for brush model rendering
	// The shader uses: projection * view * worldPos
	// We need to include the model matrix so it becomes: projection * view * model * localPos
	// This is equivalent to: projection * (view * model) * localPos
	// So we combine view with model: newView = view * model
	SceneUniforms brushUniforms;
	std::memcpy(&brushUniforms, sceneUniformBuffer_->contents(), sizeof(SceneUniforms));
	
	// Multiply the view matrix by the model matrix
	float brushView[16];
	Mat4Multiply(brushUniforms.view, modelMatrix, brushView);
	std::memcpy(brushUniforms.view, brushView, sizeof(brushView));
	
	// Also update viewProjection for consistency
	float brushViewProjection[16];
	Mat4Multiply(brushUniforms.projection, brushView, brushViewProjection);
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
	
	// Set up vertex buffer (brush model geometry lives in staticWorldVertexBuffer_)
	if (!staticWorldVertexBuffer_) {
		brushUniformBuffer->release();
		return;
	}
	currentRenderEncoder_->setVertexBuffer(staticWorldVertexBuffer_.get(), 0, 0);
	currentRenderEncoder_->setVertexBuffer(brushUniformBuffer, 0, 1);
	// Also bind to fragment stage so uniforms.viewOrigin is accessible.
	currentRenderEncoder_->setFragmentBuffer(brushUniformBuffer, 0, 1);
	
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

		// Set tcGen type: 0=Texture, 1=Lightmap, 2=Environment, 3=Vector, 4=Fog
		switch (stageInfo->tcGen.type) {
			case MetalTCGen::Lightmap:
				params.tcGenType = 1.0f;
				params.texCoordSelector = 1.0f;
				break;
			case MetalTCGen::Environment:
				params.tcGenType = 2.0f;
				params.texCoordSelector = 0.0f;
				break;
			case MetalTCGen::Vector:
				params.tcGenType = 3.0f;
				params.tcGenSVector[0] = stageInfo->tcGen.sVector[0];
				params.tcGenSVector[1] = stageInfo->tcGen.sVector[1];
				params.tcGenSVector[2] = stageInfo->tcGen.sVector[2];
				params.tcGenSVector[3] = 0.0f;
				params.tcGenTVector[0] = stageInfo->tcGen.tVector[0];
				params.tcGenTVector[1] = stageInfo->tcGen.tVector[1];
				params.tcGenTVector[2] = stageInfo->tcGen.tVector[2];
				params.tcGenTVector[3] = 0.0f;
				break;
			case MetalTCGen::Fog:
				params.tcGenType = 4.0f;
				break;
			default:
				params.tcGenType = 0.0f;
				params.texCoordSelector = 0.0f;
				break;
		}

		// Entity shaderRGBA normalized to 0-1 (used by Entity/OneMinusEntity modes)
		const float entR = ent.shaderRGBA[0] / 255.0f;
		const float entG = ent.shaderRGBA[1] / 255.0f;
		const float entB = ent.shaderRGBA[2] / 255.0f;
		const float entA = ent.shaderRGBA[3] / 255.0f;

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
			case MetalRGBGen::Wave: {
				params.rgbGenType = 4.0f;
				float w = EvalWaveForm(stageInfo->rgbGen.wave, sceneTimeSeconds);
				if (w < 0.0f) w = 0.0f;
				if (w > 1.0f) w = 1.0f;
				params.waveColorScale = w;
				break;
			}
			case MetalRGBGen::Const:
				params.rgbGenType = 5.0f;
				params.constColor[0] = stageInfo->rgbGen.constColor[0];
				params.constColor[1] = stageInfo->rgbGen.constColor[1];
				params.constColor[2] = stageInfo->rgbGen.constColor[2];
				params.constColor[3] = stageInfo->rgbGen.constColor[3];
				break;
			case MetalRGBGen::Entity:
				params.rgbGenType = 6.0f;
				params.entityColor[0] = entR;
				params.entityColor[1] = entG;
				params.entityColor[2] = entB;
				params.entityColor[3] = entA;
				break;
			case MetalRGBGen::OneMinusEntity:
				params.rgbGenType = 7.0f;
				params.entityColor[0] = entR;
				params.entityColor[1] = entG;
				params.entityColor[2] = entB;
				params.entityColor[3] = entA;
				break;
			case MetalRGBGen::OneMinusVertex:
				params.rgbGenType = 8.0f;
				break;
			default:
				params.rgbGenType = 0.0f;
				break;
		}

		// alphaGen handling
		switch (stageInfo->alphaGen.type) {
			case MetalAlphaGen::Entity:
				params.alphaGenType = 1.0f;
				params.entityColor[0] = entR;
				params.entityColor[1] = entG;
				params.entityColor[2] = entB;
				params.entityColor[3] = entA;
				break;
			case MetalAlphaGen::OneMinusEntity:
				params.alphaGenType = 2.0f;
				params.entityColor[0] = entR;
				params.entityColor[1] = entG;
				params.entityColor[2] = entB;
				params.entityColor[3] = entA;
				break;
			case MetalAlphaGen::Wave: {
				params.alphaGenType = 3.0f;
				float w = EvalWaveForm(stageInfo->alphaGen.wave, sceneTimeSeconds);
				if (w < 0.0f) w = 0.0f;
				if (w > 1.0f) w = 1.0f;
				params.alphaWaveValue = w;
				break;
			}
			case MetalAlphaGen::LightingSpecular:
				params.alphaGenType = 4.0f;
				break;
			case MetalAlphaGen::Portal:
				params.alphaGenType = 5.0f;
				params.portalRange = (stageInfo->alphaGen.portalRange > 0.0f)
				                     ? stageInfo->alphaGen.portalRange : 256.0f;
				break;
			default:
				params.alphaGenType = 0.0f;
				break;
		}

		const int alphaFunc = stageInfo->alphaFunc;
		if (alphaFunc != 0) {
			params.alphaTestEnabled = 1.0f;
			params.alphaFunc = static_cast<float>(alphaFunc);
			switch (alphaFunc) {
				case 1:  // GT0
					params.alphaRef = 1.0f / 255.0f;
					break;
				case 2:  // LT128
				case 3:  // GE128
				default:
					params.alphaRef = 128.0f / 255.0f;
					break;
			}
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

			// Bind normal/specular maps and NormalSpecularParams for this stage.
			{
				NormalSpecularParams nsParams{};
				MTL::Texture* normalTex = defaultTexture;
				MTL::Texture* specTex   = defaultTexture;
				if (stageRuntime->normalMapHandle > 0) {
					MTL::Texture* t = texManager->getTexture(stageRuntime->normalMapHandle);
					if (t) { normalTex = t; nsParams.useNormalMap = 1.0f; }
				}
				if (stageRuntime->specularMapHandle > 0) {
					MTL::Texture* t = texManager->getTexture(stageRuntime->specularMapHandle);
					if (t) { specTex = t; nsParams.useSpecularMap = 1.0f; }
				}
				// Bind cubemap slot 3 (always valid texture; reflections disabled for brush models).
				currentRenderEncoder_->setFragmentTexture(normalTex, 1);
				currentRenderEncoder_->setFragmentTexture(specTex,   2);
				currentRenderEncoder_->setFragmentTexture(getOrCreateFallbackCubemap(), 3);
				currentRenderEncoder_->setFragmentBytes(&nsParams, sizeof(NormalSpecularParams), 3);
			}

			// Vertices are in staticWorldVertexBuffer_ at the same offsets as worldVertexTemplate_
			currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangle,
				static_cast<NS::UInteger>(packet.firstVertex),
				static_cast<NS::UInteger>(packet.vertexCount));
		}
	}

	// --- DLIGHT PASS ---
	// After rendering all surfaces normally, apply dynamic lighting additively.
	// For each dlight that overlaps this brush model, re-draw the surfaces with
	// the dlight pipeline (additive blend, depth-test-only) using the model-local
	// light position so the shader can compute per-vertex falloff.
	{
		const int numLights = static_cast<int>(lightPackets_.size());
		if (numLights > 0 && ensureDlightResources() && dlightPipeline_ && dlightTexture_ && dlightDepthState_) {
			// Collect light origins/radii into a plain array for R_DlightBmodel
			MetalSceneLight lightArray[MAX_DLIGHTS];
			const int clampedLights = std::min(numLights, static_cast<int>(MAX_DLIGHTS));
			for (int i = 0; i < clampedLights; i++) {
				lightArray[i] = lightPackets_[i].light;
			}

			const uint32_t dlightMask = R_DlightBmodel(bmodel, ent, lightArray, clampedLights);

			if (dlightMask) {
				// Set up dlight render state once for all lights on this bmodel
				currentRenderEncoder_->setRenderPipelineState(dlightPipeline_.get());
				currentRenderEncoder_->setDepthStencilState(dlightDepthState_.get());
				currentRenderEncoder_->setVertexBuffer(staticWorldVertexBuffer_.get(), 0, 0);
				currentRenderEncoder_->setFragmentTexture(dlightTexture_.get(), 0);
				currentRenderEncoder_->setFragmentSamplerState(sampler2D_.get(), 0);

				DlightUniforms dlightUniforms{};
				// The brush-model MVP already includes the entity transform:
				//   brushViewProjection = projection * (view * model)
				std::memcpy(dlightUniforms.modelViewProjection, brushViewProjection,
				            sizeof(brushViewProjection));
				dlightUniforms.deformGen = 0;
				dlightUniforms.time = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
				dlightUniforms.vertexLerp = 0.0f;

				for (int i = 0; i < clampedLights; i++) {
					if (!(dlightMask & (1u << i))) {
						continue;
					}

					const MetalSceneLight& light = lightPackets_[i].light;
					const float radius = light.intensity;
					if (radius <= 0.0f) {
						continue;
					}

					// Transform world-space light origin into model-local space.
					// Vertices in the BSP are stored in model-local space; the entity
					// origin/axis describe the model-to-world transform.
					// This mirrors R_TransformDlights from renderergl2/tr_light.c.
					vec3_t temp;
					VectorSubtract(light.origin, ent.origin, temp);
					dlightUniforms.dlightInfo[0] = DotProduct(temp, ent.axis[0]);
					dlightUniforms.dlightInfo[1] = DotProduct(temp, ent.axis[1]);
					dlightUniforms.dlightInfo[2] = DotProduct(temp, ent.axis[2]);
					dlightUniforms.dlightInfo[3] = 1.0f / radius;  // inverse radius for shader falloff

					dlightUniforms.color[0] = light.color[0];
					dlightUniforms.color[1] = light.color[1];
					dlightUniforms.color[2] = light.color[2];
					dlightUniforms.color[3] = 1.0f;

					currentRenderEncoder_->setVertexBytes(&dlightUniforms, sizeof(DlightUniforms), 1);

					// Re-draw every surface of this brush model under the light
					for (int packetIndex : packetIndices) {
						const ScenePolyPacket& packet = worldPacketTemplate_[packetIndex];
						if (packet.vertexCount <= 0) {
							continue;
						}
						// Vertices are in staticWorldVertexBuffer_ at the same offsets as worldVertexTemplate_
						currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangle,
							static_cast<NS::UInteger>(packet.firstVertex),
							static_cast<NS::UInteger>(packet.vertexCount));
					}
				}
			}
		}
	}

	brushUniformBuffer->release();
}

// ============================================================================
// Procedural Entity Rendering (Sprites, Beams, Rails, Lightning)
// These entities are rendered as procedurally-generated quads/geometry
// ============================================================================

void MetalRenderer::addQuadToPolyBuffer(const vec3_t origin, const vec3_t left, const vec3_t up,
                                         float s1, float t1, float s2, float t2, const byte* color,
                                         qhandle_t shader, int& firstVertex, int& vertexCount) {
	// Generate 4 vertices for the quad
	MetalPolyVertex v0, v1, v2, v3;
	
	// Vertex 0: origin + left + up
	v0.xyz[0] = origin[0] + left[0] + up[0];
	v0.xyz[1] = origin[1] + left[1] + up[1];
	v0.xyz[2] = origin[2] + left[2] + up[2];
	v0.st[0] = s1; v0.st[1] = t1;
	
	// Vertex 1: origin - left + up
	v1.xyz[0] = origin[0] - left[0] + up[0];
	v1.xyz[1] = origin[1] - left[1] + up[1];
	v1.xyz[2] = origin[2] - left[2] + up[2];
	v1.st[0] = s2; v1.st[1] = t1;
	
	// Vertex 2: origin - left - up
	v2.xyz[0] = origin[0] - left[0] - up[0];
	v2.xyz[1] = origin[1] - left[1] - up[1];
	v2.xyz[2] = origin[2] - left[2] - up[2];
	v2.st[0] = s2; v2.st[1] = t2;
	
	// Vertex 3: origin + left - up
	v3.xyz[0] = origin[0] + left[0] - up[0];
	v3.xyz[1] = origin[1] + left[1] - up[1];
	v3.xyz[2] = origin[2] + left[2] - up[2];
	v3.st[0] = s1; v3.st[1] = t2;
	
	// Normal points toward camera (negative view direction)
	vec3_t normal;
	VectorSubtract(vec3_origin, sceneCamera_.viewAxis[0], normal);
	for (int i = 0; i < 3; i++) {
		v0.normal[i] = v1.normal[i] = v2.normal[i] = v3.normal[i] = normal[i];
	}
	
	// Set vertex colors
	for (int i = 0; i < 4; i++) {
		v0.modulate[i] = v1.modulate[i] = v2.modulate[i] = v3.modulate[i] = color[i];
	}
	
	// Clear lightmap coords (sprites don't use lightmaps)
	v0.lightmap[0] = v0.lightmap[1] = 0.0f;
	v1.lightmap[0] = v1.lightmap[1] = 0.0f;
	v2.lightmap[0] = v2.lightmap[1] = 0.0f;
	v3.lightmap[0] = v3.lightmap[1] = 0.0f;
	
	// Add to poly buffer as 2 triangles (6 vertices)
	firstVertex = static_cast<int>(polyVertices_.size());
	
	// Triangle 1: 0, 1, 3
	polyVertices_.push_back(v0);
	polyVertices_.push_back(v1);
	polyVertices_.push_back(v3);
	
	// Triangle 2: 3, 1, 2
	polyVertices_.push_back(v3);
	polyVertices_.push_back(v1);
	polyVertices_.push_back(v2);
	
	vertexCount = 6;
}

void MetalRenderer::renderSprite(const refEntity_t& ent) {
	if (!currentRenderEncoder_ || !sceneCamera_.valid) {
		return;
	}
	
	// Get shader for this sprite
	MetalShaderResource* shaderResource = getShaderResource(ent.customShader);
	if (!shaderResource) {
		shaderResource = getShaderResource(0); // fallback to default
	}
	if (!shaderResource || shaderResource->stageRuntimes.empty()) {
		return;
	}
	
	// Calculate the xyz locations for the four corners
	vec3_t left, up;
	float radius = ent.radius;
	
	if (ent.rotation == 0) {
		VectorScale(sceneCamera_.viewAxis[1], radius, left);
		VectorScale(sceneCamera_.viewAxis[2], radius, up);
	} else {
		float ang = static_cast<float>(M_PI) * ent.rotation / 180.0f;
		float s = std::sin(ang);
		float c = std::cos(ang);
		
		VectorScale(sceneCamera_.viewAxis[1], c * radius, left);
		VectorMA(left, -s * radius, sceneCamera_.viewAxis[2], left);
		
		VectorScale(sceneCamera_.viewAxis[2], c * radius, up);
		VectorMA(up, s * radius, sceneCamera_.viewAxis[1], up);
	}
	
	// Mirror handling (if needed)
	// if (backEnd.viewParms.isMirror) {
	//     VectorSubtract(vec3_origin, left, left);
	// }
	
	// Add quad to buffer
	int firstVertex, vertexCount;
	addQuadToPolyBuffer(ent.origin, left, up, 0.0f, 0.0f, 1.0f, 1.0f, ent.shaderRGBA, 
	                    ent.customShader, firstVertex, vertexCount);
	
	// Create a poly packet for this sprite
	ScenePolyPacket spritePacket;
	spritePacket.shader = ent.customShader;
	spritePacket.firstVertex = firstVertex;
	spritePacket.vertexCount = vertexCount;
	spritePacket.primitive = MTL::PrimitiveTypeTriangle;
	spritePacket.lightmapHandle = 0;
	
	// Render the sprite using the same path as other polys
	// We need to make sure the poly vertex buffer is up to date
	if (!polyVertexBuffer_) {
		return;
	}
	
	// Update the GPU buffer with the new vertices
	size_t requiredSize = polyVertices_.size() * sizeof(MetalPolyVertex);
	if (polyVertexBufferSize_ < requiredSize) {
		polyVertexBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		if (!polyVertexBuffer_) return;
		polyVertexBufferSize_ = requiredSize;
	}
	std::memcpy(polyVertexBuffer_->contents(), polyVertices_.data(), requiredSize);
	
	// Now render the sprite
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) return;
	
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
	
	// Render each stage
	for (size_t stageIndex = 0; stageIndex < shaderResource->stageRuntimes.size(); ++stageIndex) {
		const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
		if (!stageRuntime) continue;
		
		const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
		
		// Build stage params
		StageFragmentParams params{};
		params.overBrightBits = sceneUniforms_.overBrightBits;
		params.rgbGenType = 0.0f;  // Use vertex color
		if (stageInfo) {
			switch (stageInfo->rgbGen.type) {
				case MetalRGBGen::Identity:
					params.rgbGenType = 1.0f;
					break;
				case MetalRGBGen::IdentityLighting:
					params.rgbGenType = 2.0f;
					break;
				default:
					break;
			}
		}
		currentRenderEncoder_->setFragmentBytes(&params, sizeof(params), 0);
		
		// Set up tcMod
		TCModParams tcModParams = computeTCModParams(stageInfo, sceneTimeSeconds);
		currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
		
		// Get pipeline
		StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
		if (!pipelineEntry || !pipelineEntry->pipeline) continue;
		
		MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
		currentRenderEncoder_->setDepthStencilState(pipelineEntry->depthState.get());
		
		// Bind texture
		qhandle_t textureHandle = selectStageImage(*shaderResource, stageRuntime, sceneTimeSeconds, 0);
		if (textureHandle >= 0) {
			MTL::Texture* tex = texManager->getTexture(textureHandle);
			if (tex) {
				MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, tex);
			}
		}
		
		// Bind sampler
		bool useClamp = stageInfo && stageInfo->clampMap;
		MTL::SamplerState* sampler = (useClamp && sceneClampSampler_) ? sceneClampSampler_.get() : sceneSampler_.get();
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler);
		
		// Draw
		currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangle,
		                                       static_cast<NS::UInteger>(firstVertex),
		                                       static_cast<NS::UInteger>(vertexCount));
	}
}

void MetalRenderer::renderBeam(const refEntity_t& ent) {
	// Beam rendering - creates a tube/cylinder between two points (gauntlet effect)
	// Similar to OpenGL2's RB_SurfaceBeam
	
	constexpr int NUM_BEAM_SEGS = 6;
	
	vec3_t direction, normalizedDirection;
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;
	
	VectorCopy(ent.oldorigin, oldorigin);
	VectorCopy(ent.origin, origin);
	
	// Compute direction from origin to oldorigin
	VectorSubtract(oldorigin, origin, direction);
	VectorCopy(direction, normalizedDirection);
	if (VectorNormalize(normalizedDirection) == 0.0f) {
		return;
	}
	
	// Get a perpendicular vector
	vec3_t perpvec;
	PerpendicularVector(perpvec, normalizedDirection);
	VectorScale(perpvec, 4.0f, perpvec);  // Beam width of 4 units
	
	// Generate circle points around the beam axis
	for (int i = 0; i < NUM_BEAM_SEGS; i++) {
		RotatePointAroundVector(start_points[i], normalizedDirection, perpvec, 
		                        (360.0f / NUM_BEAM_SEGS) * i);
		VectorAdd(start_points[i], direction, end_points[i]);
	}
	
	// Reserve space for vertices (2 vertices per segment, plus 2 for closing)
	size_t baseVertex = polyVertices_.size();
	polyVertices_.reserve(polyVertices_.size() + (NUM_BEAM_SEGS + 1) * 2);
	
	// Red color for beam (like OpenGL2)
	uint8_t beamColor[4] = { 255, 0, 0, 255 };
	
	for (int i = 0; i <= NUM_BEAM_SEGS; i++) {
		int idx = i % NUM_BEAM_SEGS;
		
		// Start point
		MetalPolyVertex v1;
		v1.xyz[0] = origin[0] + start_points[idx][0];
		v1.xyz[1] = origin[1] + start_points[idx][1];
		v1.xyz[2] = origin[2] + start_points[idx][2];
		v1.st[0] = 0.0f;
		v1.st[1] = static_cast<float>(i) / NUM_BEAM_SEGS;
		v1.lightmap[0] = 0.0f;
		v1.lightmap[1] = 0.0f;
		v1.normal[0] = 0.0f; v1.normal[1] = 0.0f; v1.normal[2] = 1.0f;
		v1.modulate[0] = beamColor[0];
		v1.modulate[1] = beamColor[1];
		v1.modulate[2] = beamColor[2];
		v1.modulate[3] = beamColor[3];
		polyVertices_.push_back(v1);
		
		// End point
		MetalPolyVertex v2;
		v2.xyz[0] = origin[0] + end_points[idx][0];
		v2.xyz[1] = origin[1] + end_points[idx][1];
		v2.xyz[2] = origin[2] + end_points[idx][2];
		v2.st[0] = 1.0f;
		v2.st[1] = static_cast<float>(i) / NUM_BEAM_SEGS;
		v2.lightmap[0] = 0.0f;
		v2.lightmap[1] = 0.0f;
		v2.normal[0] = 0.0f; v2.normal[1] = 0.0f; v2.normal[2] = 1.0f;
		v2.modulate[0] = beamColor[0];
		v2.modulate[1] = beamColor[1];
		v2.modulate[2] = beamColor[2];
		v2.modulate[3] = beamColor[3];
		polyVertices_.push_back(v2);
	}
	
	// Generate triangle indices for the tube
	std::vector<uint16_t> indices;
	indices.reserve(NUM_BEAM_SEGS * 6);
	for (int i = 0; i < NUM_BEAM_SEGS; i++) {
		int base = static_cast<int>(baseVertex) + i * 2;
		indices.push_back(base);
		indices.push_back(base + 2);
		indices.push_back(base + 1);
		
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
	}
	
	// Render with additive blending
	if (!currentRenderEncoder_) return;
	
	// Update vertex buffer
	size_t requiredSize = polyVertices_.size() * sizeof(MetalPolyVertex);
	if (!polyVertexBuffer_ || polyVertexBufferSize_ < requiredSize) {
		polyVertexBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		if (!polyVertexBuffer_) return;
		polyVertexBufferSize_ = requiredSize;
	}
	std::memcpy(polyVertexBuffer_->contents(), polyVertices_.data(), requiredSize);
	
	// Create index buffer
	MetalPtr<MTL::Buffer> indexBuffer(device_->newBuffer(indices.data(), 
	                                        indices.size() * sizeof(uint16_t), 
	                                        MTL::ResourceStorageModeShared));
	if (!indexBuffer) return;
	
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) return;
	
	// Set fragment params - use vertex color (rgbGenType = 0)
	StageFragmentParams params{};
	params.overBrightBits = sceneUniforms_.overBrightBits;
	params.rgbGenType = 0.0f;  // Use vertex color
	currentRenderEncoder_->setFragmentBytes(&params, sizeof(params), 0);
	
	// No tcMod - use identity
	TCModParams tcModParams{};
	currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
	
	// Create pipeline key for additive blending (ONE + ONE)
	MetalShaderResource::MetalPipelineKey beamKey;
	beamKey.blendMode = MetalShaderBlendMode::Additive;
	beamKey.srcBlend = MetalBlendFactor::One;
	beamKey.dstBlend = MetalBlendFactor::One;
	beamKey.depthWrite = false;
	beamKey.depthTest = true;
	beamKey.alphaTest = false;
	
	StagePipelineEntry* pipelineEntry = getStagePipeline(beamKey);
	if (!pipelineEntry || !pipelineEntry->pipeline) return;
	
	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
	currentRenderEncoder_->setDepthStencilState(pipelineEntry->depthState.get());
	
	// Bind white texture (handle 0)
	MTL::Texture* whiteTex = texManager->getTexture(0);
	if (whiteTex) {
		MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, whiteTex);
	}
	
	if (sceneSampler_) {
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sceneSampler_.get());
	}
	
	// Draw indexed
	currentRenderEncoder_->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
	                                              static_cast<NS::UInteger>(indices.size()),
	                                              MTL::IndexTypeUInt16,
	                                              indexBuffer.get(),
	                                              0);
}

void MetalRenderer::renderRailCore(const refEntity_t& ent) {
	// Rail core - a flat ribbon between two points facing the camera
	// Similar to OpenGL2's RB_SurfaceRailCore + DoRailCore
	
	vec3_t start, end;
	VectorCopy(ent.oldorigin, start);
	VectorCopy(ent.origin, end);
	
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);
	if (len < 0.1f) return;
	
	// Compute side vector facing camera
	vec3_t v1, v2, right;
	VectorSubtract(start, sceneCamera_.viewOrigin, v1);
	VectorNormalize(v1);
	VectorSubtract(end, sceneCamera_.viewOrigin, v2);
	VectorNormalize(v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);
	
	// Rail core width (default 16 units like OpenGL2)
	float spanWidth = 16.0f;  // r_railCoreWidth
	float t = len / 256.0f;  // Texture coordinate scaling
	
	// Entity color (0.25 brightness at edges, full brightness in center)
	uint8_t edgeColor[4] = {
		static_cast<uint8_t>(ent.shaderRGBA[0] * 0.25f),
		static_cast<uint8_t>(ent.shaderRGBA[1] * 0.25f),
		static_cast<uint8_t>(ent.shaderRGBA[2] * 0.25f),
		ent.shaderRGBA[3]
	};
	uint8_t centerColor[4] = {
		ent.shaderRGBA[0],
		ent.shaderRGBA[1],
		ent.shaderRGBA[2],
		ent.shaderRGBA[3]
	};
	
	// Create 4 vertices for the rail ribbon
	size_t baseVertex = polyVertices_.size();
	
	vec3_t pos;
	
	// Vertex 0: start + spanWidth
	VectorMA(start, spanWidth, right, pos);
	MetalPolyVertex v0;
	v0.xyz[0] = pos[0]; v0.xyz[1] = pos[1]; v0.xyz[2] = pos[2];
	v0.st[0] = 0.0f; v0.st[1] = 0.0f;
	v0.lightmap[0] = 0.0f; v0.lightmap[1] = 0.0f;
	v0.normal[0] = 0.0f; v0.normal[1] = 0.0f; v0.normal[2] = 1.0f;
	v0.modulate[0] = edgeColor[0]; v0.modulate[1] = edgeColor[1];
	v0.modulate[2] = edgeColor[2]; v0.modulate[3] = edgeColor[3];
	polyVertices_.push_back(v0);
	
	// Vertex 1: start - spanWidth
	VectorMA(start, -spanWidth, right, pos);
	MetalPolyVertex v1_vert;
	v1_vert.xyz[0] = pos[0]; v1_vert.xyz[1] = pos[1]; v1_vert.xyz[2] = pos[2];
	v1_vert.st[0] = 0.0f; v1_vert.st[1] = 1.0f;
	v1_vert.lightmap[0] = 0.0f; v1_vert.lightmap[1] = 0.0f;
	v1_vert.normal[0] = 0.0f; v1_vert.normal[1] = 0.0f; v1_vert.normal[2] = 1.0f;
	v1_vert.modulate[0] = centerColor[0]; v1_vert.modulate[1] = centerColor[1];
	v1_vert.modulate[2] = centerColor[2]; v1_vert.modulate[3] = centerColor[3];
	polyVertices_.push_back(v1_vert);
	
	// Vertex 2: end + spanWidth
	VectorMA(end, spanWidth, right, pos);
	MetalPolyVertex v2_vert;
	v2_vert.xyz[0] = pos[0]; v2_vert.xyz[1] = pos[1]; v2_vert.xyz[2] = pos[2];
	v2_vert.st[0] = t; v2_vert.st[1] = 0.0f;
	v2_vert.lightmap[0] = 0.0f; v2_vert.lightmap[1] = 0.0f;
	v2_vert.normal[0] = 0.0f; v2_vert.normal[1] = 0.0f; v2_vert.normal[2] = 1.0f;
	v2_vert.modulate[0] = centerColor[0]; v2_vert.modulate[1] = centerColor[1];
	v2_vert.modulate[2] = centerColor[2]; v2_vert.modulate[3] = centerColor[3];
	polyVertices_.push_back(v2_vert);
	
	// Vertex 3: end - spanWidth
	VectorMA(end, -spanWidth, right, pos);
	MetalPolyVertex v3;
	v3.xyz[0] = pos[0]; v3.xyz[1] = pos[1]; v3.xyz[2] = pos[2];
	v3.st[0] = t; v3.st[1] = 1.0f;
	v3.lightmap[0] = 0.0f; v3.lightmap[1] = 0.0f;
	v3.normal[0] = 0.0f; v3.normal[1] = 0.0f; v3.normal[2] = 1.0f;
	v3.modulate[0] = centerColor[0]; v3.modulate[1] = centerColor[1];
	v3.modulate[2] = centerColor[2]; v3.modulate[3] = centerColor[3];
	polyVertices_.push_back(v3);
	
	// Render using shader
	renderRailRibbonSegment(baseVertex, 4, ent.customShader);
}

void MetalRenderer::renderRailRibbonSegment(size_t baseVertex, int vertexCount, qhandle_t shaderHandle) {
	if (!currentRenderEncoder_) return;
	
	// Update vertex buffer
	size_t requiredSize = polyVertices_.size() * sizeof(MetalPolyVertex);
	if (!polyVertexBuffer_ || polyVertexBufferSize_ < requiredSize) {
		polyVertexBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		if (!polyVertexBuffer_) return;
		polyVertexBufferSize_ = requiredSize;
	}
	std::memcpy(polyVertexBuffer_->contents(), polyVertices_.data(), requiredSize);
	
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) return;
	
	// Get shader resource
	MetalShaderResource* shaderResource = getShaderResource(shaderHandle);
	if (!shaderResource) return;
	
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
	
	// Render each stage
	for (size_t stageIndex = 0; stageIndex < shaderResource->stageRuntimes.size(); ++stageIndex) {
		const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
		if (!stageRuntime) continue;
		
		const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
		
		StageFragmentParams params{};
		params.overBrightBits = sceneUniforms_.overBrightBits;
		params.rgbGenType = 0.0f;  // Use vertex color
		currentRenderEncoder_->setFragmentBytes(&params, sizeof(params), 0);
		
		TCModParams tcModParams = computeTCModParams(stageInfo, sceneTimeSeconds);
		currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
		
		StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
		if (!pipelineEntry || !pipelineEntry->pipeline) continue;
		
		MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
		currentRenderEncoder_->setDepthStencilState(pipelineEntry->depthState.get());
		
		qhandle_t textureHandle = selectStageImage(*shaderResource, stageRuntime, sceneTimeSeconds, 0);
		if (textureHandle >= 0) {
			MTL::Texture* tex = texManager->getTexture(textureHandle);
			if (tex) {
				MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, tex);
			}
		}
		
		bool useClamp = stageInfo && stageInfo->clampMap;
		MTL::SamplerState* sampler = (useClamp && sceneClampSampler_) ? sceneClampSampler_.get() : sceneSampler_.get();
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler);
		
		// Draw as triangle strip (4 vertices = 1 quad)
		// Actually, need to emit 2 triangles: 0,1,2 and 2,1,3
		currentRenderEncoder_->drawPrimitives(MTL::PrimitiveTypeTriangleStrip,
		                                       static_cast<NS::UInteger>(baseVertex),
		                                       static_cast<NS::UInteger>(vertexCount));
	}
}

void MetalRenderer::renderRailRings(const refEntity_t& ent) {
	// Rail rings - discs along the rail path
	// Similar to OpenGL2's RB_SurfaceRailRings + DoRailDiscs
	
	vec3_t start, end;
	VectorCopy(ent.oldorigin, start);
	VectorCopy(ent.origin, end);
	
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);
	if (len < 0.1f) return;
	
	vec3_t right, up;
	MakeNormalVectors(vec, right, up);
	
	float segmentLength = 32.0f;  // r_railSegmentLength
	int numSegs = static_cast<int>(len / segmentLength);
	if (numSegs <= 0) numSegs = 1;
	if (numSegs > 1) numSegs--;
	
	// Scale the segment direction
	VectorScale(vec, segmentLength, vec);
	
	float spanWidth = 24.0f;  // r_railWidth default
	float scale = 0.25f;
	
	// Generate disc positions (4 corners for each disc)
	vec3_t pos[4];
	for (int i = 0; i < 4; i++) {
		float c = cosf((45.0f + i * 90.0f) * M_PI / 180.0f);
		float s = sinf((45.0f + i * 90.0f) * M_PI / 180.0f);
		vec3_t v;
		v[0] = (right[0] * c + up[0] * s) * scale * spanWidth;
		v[1] = (right[1] * c + up[1] * s) * scale * spanWidth;
		v[2] = (right[2] * c + up[2] * s) * scale * spanWidth;
		VectorAdd(start, v, pos[i]);
		
		if (numSegs > 1) {
			// Offset by 1 segment for long distance shots
			VectorAdd(pos[i], vec, pos[i]);
		}
	}
	
	size_t baseVertex = polyVertices_.size();
	
	uint8_t color[4] = {
		ent.shaderRGBA[0],
		ent.shaderRGBA[1],
		ent.shaderRGBA[2],
		ent.shaderRGBA[3]
	};
	
	// Generate discs
	for (int seg = 0; seg < numSegs; seg++) {
		for (int j = 0; j < 4; j++) {
			MetalPolyVertex v;
			v.xyz[0] = pos[j][0];
			v.xyz[1] = pos[j][1];
			v.xyz[2] = pos[j][2];
			v.st[0] = (j < 2) ? 1.0f : 0.0f;
			v.st[1] = (j && j != 3) ? 1.0f : 0.0f;
			v.lightmap[0] = 0.0f; v.lightmap[1] = 0.0f;
			v.normal[0] = vec[0]; v.normal[1] = vec[1]; v.normal[2] = vec[2];
			v.modulate[0] = color[0]; v.modulate[1] = color[1];
			v.modulate[2] = color[2]; v.modulate[3] = color[3];
			polyVertices_.push_back(v);
			
			// Advance position for next segment
			VectorAdd(pos[j], vec, pos[j]);
		}
	}
	
	// Generate triangle indices
	std::vector<uint16_t> indices;
	indices.reserve(numSegs * 6);
	for (int seg = 0; seg < numSegs; seg++) {
		int base = static_cast<int>(baseVertex) + seg * 4;
		// Two triangles per quad: 0,1,3 and 3,1,2
		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 3);
		indices.push_back(base + 3);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
	}
	
	// Render
	if (!currentRenderEncoder_) return;
	
	size_t requiredSize = polyVertices_.size() * sizeof(MetalPolyVertex);
	if (!polyVertexBuffer_ || polyVertexBufferSize_ < requiredSize) {
		polyVertexBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		if (!polyVertexBuffer_) return;
		polyVertexBufferSize_ = requiredSize;
	}
	std::memcpy(polyVertexBuffer_->contents(), polyVertices_.data(), requiredSize);
	
	MetalPtr<MTL::Buffer> indexBuffer(device_->newBuffer(indices.data(), 
	                                        indices.size() * sizeof(uint16_t), 
	                                        MTL::ResourceStorageModeShared));
	if (!indexBuffer) return;
	
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) return;
	
	MetalShaderResource* shaderResource = getShaderResource(ent.customShader);
	if (!shaderResource) return;
	
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
	
	for (size_t stageIndex = 0; stageIndex < shaderResource->stageRuntimes.size(); ++stageIndex) {
		const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
		if (!stageRuntime) continue;
		
		const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
		
		StageFragmentParams params{};
		params.overBrightBits = sceneUniforms_.overBrightBits;
		params.rgbGenType = 0.0f;
		currentRenderEncoder_->setFragmentBytes(&params, sizeof(params), 0);
		
		TCModParams tcModParams = computeTCModParams(stageInfo, sceneTimeSeconds);
		currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
		
		StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
		if (!pipelineEntry || !pipelineEntry->pipeline) continue;
		
		MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
		currentRenderEncoder_->setDepthStencilState(pipelineEntry->depthState.get());
		
		qhandle_t textureHandle = selectStageImage(*shaderResource, stageRuntime, sceneTimeSeconds, 0);
		if (textureHandle >= 0) {
			MTL::Texture* tex = texManager->getTexture(textureHandle);
			if (tex) {
				MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, tex);
			}
		}
		
		bool useClamp = stageInfo && stageInfo->clampMap;
		MTL::SamplerState* sampler = (useClamp && sceneClampSampler_) ? sceneClampSampler_.get() : sceneSampler_.get();
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler);
		
		currentRenderEncoder_->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
		                                              static_cast<NS::UInteger>(indices.size()),
		                                              MTL::IndexTypeUInt16,
		                                              indexBuffer.get(),
		                                              0);
	}
}

void MetalRenderer::renderLightning(const refEntity_t& ent) {
	// Lightning bolt - 4 rail cores rotated 45° around the axis
	// Similar to OpenGL2's RB_SurfaceLightningBolt
	
	vec3_t start, end;
	VectorCopy(ent.origin, start);  // Note: lightning uses origin as start, oldorigin as end
	VectorCopy(ent.oldorigin, end);
	
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);
	if (len < 0.1f) return;
	
	// Compute initial side vector facing camera
	vec3_t v1, v2, right;
	VectorSubtract(start, sceneCamera_.viewOrigin, v1);
	VectorNormalize(v1);
	VectorSubtract(end, sceneCamera_.viewOrigin, v2);
	VectorNormalize(v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);
	
	float spanWidth = 8.0f;  // Lightning width
	float t = len / 256.0f;
	
	uint8_t color[4] = {
		ent.shaderRGBA[0],
		ent.shaderRGBA[1],
		ent.shaderRGBA[2],
		ent.shaderRGBA[3]
	};
	
	size_t baseVertex = polyVertices_.size();
	
	// Generate 4 rail-like segments rotated 45° each
	for (int i = 0; i < 4; i++) {
		vec3_t pos;
		
		// Vertex 0: start + spanWidth
		VectorMA(start, spanWidth, right, pos);
		MetalPolyVertex v0;
		v0.xyz[0] = pos[0]; v0.xyz[1] = pos[1]; v0.xyz[2] = pos[2];
		v0.st[0] = 0.0f; v0.st[1] = 0.0f;
		v0.lightmap[0] = 0.0f; v0.lightmap[1] = 0.0f;
		v0.normal[0] = 0.0f; v0.normal[1] = 0.0f; v0.normal[2] = 1.0f;
		v0.modulate[0] = color[0]; v0.modulate[1] = color[1];
		v0.modulate[2] = color[2]; v0.modulate[3] = color[3];
		polyVertices_.push_back(v0);
		
		// Vertex 1: start - spanWidth
		VectorMA(start, -spanWidth, right, pos);
		MetalPolyVertex v1_vert;
		v1_vert.xyz[0] = pos[0]; v1_vert.xyz[1] = pos[1]; v1_vert.xyz[2] = pos[2];
		v1_vert.st[0] = 0.0f; v1_vert.st[1] = 1.0f;
		v1_vert.lightmap[0] = 0.0f; v1_vert.lightmap[1] = 0.0f;
		v1_vert.normal[0] = 0.0f; v1_vert.normal[1] = 0.0f; v1_vert.normal[2] = 1.0f;
		v1_vert.modulate[0] = color[0]; v1_vert.modulate[1] = color[1];
		v1_vert.modulate[2] = color[2]; v1_vert.modulate[3] = color[3];
		polyVertices_.push_back(v1_vert);
		
		// Vertex 2: end + spanWidth
		VectorMA(end, spanWidth, right, pos);
		MetalPolyVertex v2_vert;
		v2_vert.xyz[0] = pos[0]; v2_vert.xyz[1] = pos[1]; v2_vert.xyz[2] = pos[2];
		v2_vert.st[0] = t; v2_vert.st[1] = 0.0f;
		v2_vert.lightmap[0] = 0.0f; v2_vert.lightmap[1] = 0.0f;
		v2_vert.normal[0] = 0.0f; v2_vert.normal[1] = 0.0f; v2_vert.normal[2] = 1.0f;
		v2_vert.modulate[0] = color[0]; v2_vert.modulate[1] = color[1];
		v2_vert.modulate[2] = color[2]; v2_vert.modulate[3] = color[3];
		polyVertices_.push_back(v2_vert);
		
		// Vertex 3: end - spanWidth
		VectorMA(end, -spanWidth, right, pos);
		MetalPolyVertex v3;
		v3.xyz[0] = pos[0]; v3.xyz[1] = pos[1]; v3.xyz[2] = pos[2];
		v3.st[0] = t; v3.st[1] = 1.0f;
		v3.lightmap[0] = 0.0f; v3.lightmap[1] = 0.0f;
		v3.normal[0] = 0.0f; v3.normal[1] = 0.0f; v3.normal[2] = 1.0f;
		v3.modulate[0] = color[0]; v3.modulate[1] = color[1];
		v3.modulate[2] = color[2]; v3.modulate[3] = color[3];
		polyVertices_.push_back(v3);
		
		// Rotate right vector by 45° for next segment
		vec3_t temp;
		RotatePointAroundVector(temp, vec, right, 45.0f);
		VectorCopy(temp, right);
	}
	
	// Generate indices for 4 quads (each as triangle strip = 4 verts)
	// We'll render as indexed triangles
	std::vector<uint16_t> indices;
	indices.reserve(4 * 6);  // 4 quads * 6 indices each
	for (int i = 0; i < 4; i++) {
		int base = static_cast<int>(baseVertex) + i * 4;
		// First triangle: 0,1,2
		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		// Second triangle: 2,1,3
		indices.push_back(base + 2);
		indices.push_back(base + 1);
		indices.push_back(base + 3);
	}
	
	// Render
	if (!currentRenderEncoder_) return;
	
	size_t requiredSize = polyVertices_.size() * sizeof(MetalPolyVertex);
	if (!polyVertexBuffer_ || polyVertexBufferSize_ < requiredSize) {
		polyVertexBuffer_.reset(device_->newBuffer(requiredSize, MTL::ResourceStorageModeShared));
		if (!polyVertexBuffer_) return;
		polyVertexBufferSize_ = requiredSize;
	}
	std::memcpy(polyVertexBuffer_->contents(), polyVertices_.data(), requiredSize);
	
	MetalPtr<MTL::Buffer> indexBuffer(device_->newBuffer(indices.data(),
	                                        indices.size() * sizeof(uint16_t),
	                                        MTL::ResourceStorageModeShared));
	if (!indexBuffer) return;
	
	currentRenderEncoder_->setVertexBuffer(polyVertexBuffer_.get(), 0, 0);
	
	TextureManager* texManager = ensureTextureManager();
	if (!texManager) return;
	
	MetalShaderResource* shaderResource = getShaderResource(ent.customShader);
	if (!shaderResource) return;
	
	const float sceneTimeSeconds = static_cast<float>(sceneCamera_.refdef.time) * 0.001f;
	
	for (size_t stageIndex = 0; stageIndex < shaderResource->stageRuntimes.size(); ++stageIndex) {
		const auto* stageRuntime = getShaderStageRuntime(*shaderResource, stageIndex);
		if (!stageRuntime) continue;
		
		const MetalShaderStageInfo* stageInfo = stageRuntime->stageInfo;
		
		StageFragmentParams params{};
		params.overBrightBits = sceneUniforms_.overBrightBits;
		params.rgbGenType = 0.0f;
		currentRenderEncoder_->setFragmentBytes(&params, sizeof(params), 0);
		
		TCModParams tcModParams = computeTCModParams(stageInfo, sceneTimeSeconds);
		currentRenderEncoder_->setVertexBytes(&tcModParams, sizeof(TCModParams), 2);
		
		StagePipelineEntry* pipelineEntry = getStagePipeline(stageRuntime->pipelineKey);
		if (!pipelineEntry || !pipelineEntry->pipeline) continue;
		
		MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipelineEntry->pipeline.get());
		currentRenderEncoder_->setDepthStencilState(pipelineEntry->depthState.get());
		
		qhandle_t textureHandle = selectStageImage(*shaderResource, stageRuntime, sceneTimeSeconds, 0);
		if (textureHandle >= 0) {
			MTL::Texture* tex = texManager->getTexture(textureHandle);
			if (tex) {
				MetalStateCache::Instance().bindFragmentTexture(currentRenderEncoder_, 0, tex);
			}
		}
		
		bool useClamp = stageInfo && stageInfo->clampMap;
		MTL::SamplerState* sampler = (useClamp && sceneClampSampler_) ? sceneClampSampler_.get() : sceneSampler_.get();
		MetalStateCache::Instance().bindFragmentSampler(currentRenderEncoder_, 0, sampler);
		
		currentRenderEncoder_->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
		                                              static_cast<NS::UInteger>(indices.size()),
		                                              MTL::IndexTypeUInt16,
		                                              indexBuffer.get(),
		                                              0);
	}
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
		
		// TEMPORARILY DISABLED: Skip all non-model entity types to debug performance issue
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

		// Handle MDR skeletal models
		if (model->type == MetalModelType::MDR) {
			renderMDRModel(ent, *model);
			continue;
		}

		// Handle IQM skeletal models
		if (model->type == MetalModelType::IQM) {
			renderIQMModel(ent, *model);
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
	// Submit any pending 3D scene, run post-processing, then render the cinematic
	// into the 2D (drawable) encoder so it is never tonemapped.
	submitScene();
	ensureSceneRendered();
	ensurePostProcessed();

	if (!device_ || !layer_ || !currentRenderEncoder_) {
		return;
	}

	// Ensure cinematic pipeline exists (targets drawable pixel format, no depth).
	if (!pipeline_ || !sampler_) {
		if (currentDrawable_) {
			createPipeline(currentDrawable_->texture());
		}
		if (!pipeline_ || !sampler_) return;
	}

	// Upload cinematic frame data if provided.
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

	MetalStateCache::Instance().bindPipeline(currentRenderEncoder_, pipeline_.get());
	currentRenderEncoder_->setCullMode(MTL::CullModeNone);

	MTL::Viewport vp;
	vp.originX = static_cast<double>(x);
	vp.originY = static_cast<double>(y);
	vp.width  = static_cast<double>(w);
	vp.height = static_cast<double>(h);
	vp.znear  = 0.0;
	vp.zfar   = 1.0;
	currentRenderEncoder_->setViewport(vp);

	// vertex_cinematic expects: const device VertexIn* verts [[buffer(0)]]
	struct Vertex { float position[2]; float texCoord[2]; };
	Vertex verts[4] = {
		{{-1,  1}, {0, 0}},
		{{ 1,  1}, {1, 0}},
		{{-1, -1}, {0, 1}},
		{{ 1, -1}, {1, 1}}
	};
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

	// Create depth state for dlight pass: test (LessEqual) but no depth write
	// Dlights are an additive pass over already-rendered geometry
	if (!dlightDepthState_) {
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthWriteEnabled(false);
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
		dlightDepthState_.reset(device_->newDepthStencilState(depthDesc));
		depthDesc->release();
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: Dlight resources initialized\n");
	}

	return true;
}

// Create the animated-model dlight pipeline. Must be called only after
// ensureDlightResources() (for dlightFragmentFunction_, dlightTexture_,
// dlightDepthState_) and after the model pipeline (for modelVertexDescriptor_).
bool MetalRenderer::ensureDlightAnimatedResources() {
	if (dlightAnimatedPipeline_) {
		return true;
	}

	// Ensure base dlight resources (texture, depth state, fragment function) exist first.
	if (!ensureDlightResources()) {
		return false;
	}

	// modelVertexDescriptor_ is created lazily by ensureModelPipeline(); it must
	// exist before we get here because renderModelSurface() has already run.
	if (!modelVertexDescriptor_ || !dlightFragmentFunction_) {
		return false;
	}

	// Load the animated dlight vertex function from the same library.
	NS::String* animVertName = NS::String::string("vertex_dlight_animated", NS::UTF8StringEncoding);
	dlightAnimatedVertexFunction_.reset(sceneLibrary_->newFunction(animVertName));
	if (!dlightAnimatedVertexFunction_) {
		if (ri_.Printf) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to load vertex_dlight_animated shader function\n");
		}
		return false;
	}

	// Build the pipeline descriptor, mirroring the world dlight pipeline but
	// with the model vertex descriptor (14 floats / 56 bytes per vertex).
	MTL::RenderPipelineDescriptor* pd = MTL::RenderPipelineDescriptor::alloc()->init();
	pd->setVertexFunction(dlightAnimatedVertexFunction_.get());
	pd->setFragmentFunction(dlightFragmentFunction_.get());
	pd->setVertexDescriptor(modelVertexDescriptor_.get());

	// Additive blend: src ONE + dst ONE
	MTL::RenderPipelineColorAttachmentDescriptor* ca = pd->colorAttachments()->object(0);
	ca->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	ca->setBlendingEnabled(true);
	ca->setSourceRGBBlendFactor(MTL::BlendFactorOne);
	ca->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
	ca->setRgbBlendOperation(MTL::BlendOperationAdd);
	ca->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
	ca->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
	ca->setAlphaBlendOperation(MTL::BlendOperationAdd);

	pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

	NS::Error* error = nullptr;
	dlightAnimatedPipeline_.reset(device_->newRenderPipelineState(pd, &error));
	pd->release();

	if (!dlightAnimatedPipeline_) {
		if (ri_.Printf && error) {
			ri_.Printf(PRINT_WARNING, "Metal: Failed to create animated dlight pipeline: %s\n",
			           error->localizedDescription()->utf8String());
		}
		return false;
	}

	if (ri_.Printf) {
		ri_.Printf(PRINT_DEVELOPER, "Metal: Animated dlight pipeline initialized\n");
	}

	return true;
}

// Compute a bitmask of which scene lights potentially illuminate this model.
// Uses a conservative bounding-sphere test in world space to avoid drawing
// the dlight pass for lights that are clearly too far away.
uint32_t MetalRenderer::computeModelDlightBits(const refEntity_t& ent,
                                                const MetalModelLOD& lodData) const {
	if (lightPackets_.empty() || lodData.frames.empty()) {
		return 0;
	}

	// Clamp frame indices to valid range.
	int oldFrame = ent.oldframe;
	int newFrame = ent.frame;
	const int maxFrame = lodData.numFrames - 1;
	if (oldFrame < 0) oldFrame = 0;
	if (oldFrame > maxFrame) oldFrame = maxFrame;
	if (newFrame < 0) newFrame = 0;
	if (newFrame > maxFrame) newFrame = maxFrame;

	const MetalModelFrame& oldF = lodData.frames[oldFrame];
	const MetalModelFrame& newF = lodData.frames[newFrame];

	// Conservative: use the larger of the two frame radii.
	const float modelRadius = (oldF.radius > newF.radius) ? oldF.radius : newF.radius;

	// Transform frame localOrigin into world space using entity origin + axes.
	float worldCenter[3];
	for (int j = 0; j < 3; j++) {
		worldCenter[j] = ent.origin[j]
		    + ent.axis[0][j] * oldF.localOrigin[0]
		    + ent.axis[1][j] * oldF.localOrigin[1]
		    + ent.axis[2][j] * oldF.localOrigin[2];
	}

	uint32_t bits = 0;
	const int count = std::min(static_cast<int>(lightPackets_.size()), 32);
	for (int i = 0; i < count; i++) {
		const MetalSceneLight& light = lightPackets_[i].light;
		if (light.intensity <= 0.0f) {
			continue;
		}

		const float dx = light.origin[0] - worldCenter[0];
		const float dy = light.origin[1] - worldCenter[1];
		const float dz = light.origin[2] - worldCenter[2];
		const float dist2 = dx*dx + dy*dy + dz*dz;
		const float combinedR = light.intensity + modelRadius;
		if (dist2 < combinedR * combinedR) {
			bits |= (1u << i);
		}
	}

	return bits;
}

//=============================================================================
// R_MarkFragments - port of renderergl2/tr_marks.c
//=============================================================================

namespace {

#define MF_MAX_VERTS_ON_POLY 64
#define MF_SIDE_FRONT 0
#define MF_SIDE_BACK  1
#define MF_SIDE_ON    2

// Sutherland-Hodgman clip: keep points in front of the plane.
// Matches GL2's R_ChopPolyBehindPlane exactly.
static void MF_ChopPolyBehindPlane(
    int numInPoints, vec3_t inPoints[MF_MAX_VERTS_ON_POLY],
    int* numOutPoints, vec3_t outPoints[MF_MAX_VERTS_ON_POLY],
    vec3_t normal, vec_t dist, vec_t epsilon)
{
	if (numInPoints >= MF_MAX_VERTS_ON_POLY - 2) {
		*numOutPoints = 0;
		return;
	}

	float dists[MF_MAX_VERTS_ON_POLY + 4] = {};
	int   sides[MF_MAX_VERTS_ON_POLY + 4] = {};
	int   counts[3] = {};

	for (int i = 0; i < numInPoints; ++i) {
		float dot = DotProduct(inPoints[i], normal) - dist;
		dists[i] = dot;
		if (dot > epsilon)       sides[i] = MF_SIDE_FRONT;
		else if (dot < -epsilon) sides[i] = MF_SIDE_BACK;
		else                     sides[i] = MF_SIDE_ON;
		counts[sides[i]]++;
	}
	sides[numInPoints] = sides[0];
	dists[numInPoints] = dists[0];

	*numOutPoints = 0;
	if (!counts[MF_SIDE_FRONT]) return;
	if (!counts[MF_SIDE_BACK]) {
		*numOutPoints = numInPoints;
		Com_Memcpy(outPoints, inPoints, numInPoints * sizeof(vec3_t));
		return;
	}

	for (int i = 0; i < numInPoints; ++i) {
		float* p1   = inPoints[i];
		float* clip = outPoints[*numOutPoints];

		if (sides[i] == MF_SIDE_ON) {
			VectorCopy(p1, clip);
			(*numOutPoints)++;
			continue;
		}
		if (sides[i] == MF_SIDE_FRONT) {
			VectorCopy(p1, clip);
			(*numOutPoints)++;
			clip = outPoints[*numOutPoints];
		}
		if (sides[i + 1] == MF_SIDE_ON || sides[i + 1] == sides[i])
			continue;

		float* p2 = inPoints[(i + 1) % numInPoints];
		float d   = dists[i] - dists[i + 1];
		float dot = (d == 0) ? 0.0f : dists[i] / d;
		for (int j = 0; j < 3; ++j)
			clip[j] = p1[j] + dot * (p2[j] - p1[j]);
		(*numOutPoints)++;
	}
}

// Clip the surface polygon against all bounding planes and, if any fragment
// remains, append it to pointBuffer / fragmentBuffer.
// Matches GL2's R_AddMarkFragments.
static void MF_AddMarkFragments(
    int numClipPoints, vec3_t clipPoints[2][MF_MAX_VERTS_ON_POLY],
    int numPlanes, vec3_t* normals, float* dists,
    int maxPoints, vec3_t pointBuffer,
    int maxFragments, markFragment_t* fragmentBuffer,
    int* returnedPoints, int* returnedFragments,
    vec3_t /*mins*/, vec3_t /*maxs*/)
{
	int pingPong = 0;
	for (int i = 0; i < numPlanes; ++i) {
		MF_ChopPolyBehindPlane(numClipPoints, clipPoints[pingPong],
		                       &numClipPoints, clipPoints[!pingPong],
		                       normals[i], dists[i], 0.5f);
		pingPong ^= 1;
		if (!numClipPoints) return;
	}
	if (!numClipPoints) return;
	if (numClipPoints + *returnedPoints > maxPoints) return;

	markFragment_t* mf = fragmentBuffer + (*returnedFragments);
	mf->firstPoint = *returnedPoints;
	mf->numPoints  = numClipPoints;
	Com_Memcpy(pointBuffer + *returnedPoints * 3, clipPoints[pingPong],
	           numClipPoints * sizeof(vec3_t));
	(*returnedPoints)   += numClipPoints;
	(*returnedFragments)++;
}

// Recursive BSP box-surface traversal.
// childCode follows Q3 BSP convention: >= 0 means node index, < 0 means -(leafIdx+1).
// Matches GL2's R_BoxSurfaces_r.
static void MF_BoxSurfaces_r(
    int childCode,
    const std::vector<dnode_t>&  nodes,
    const std::vector<dleaf_t>&  leaves,
    const std::vector<cplane_t>& cplanes,
    const std::vector<int>&      leafSurfs,
    const std::vector<BspMarkSurface>& surfData,
    std::vector<int>& surfViewCounts, int viewCount,
    vec3_t mins, vec3_t maxs,
    const BspMarkSurface** list, int listsize, int* listlength,
    vec3_t dir)
{
	const int numNodes  = (int)nodes.size();
	const int numLeaves = (int)leaves.size();
	const int numSurfs  = (int)surfData.size();
	const int numLSurfs = (int)leafSurfs.size();

	// Tail-recurse through internal nodes, then process leaf.
	while (childCode >= 0) {
		if (childCode >= numNodes) return;
		const dnode_t& node = nodes[childCode];
		const int planeNum  = LittleLong(node.planeNum);
		if (planeNum < 0 || planeNum >= (int)cplanes.size()) return;

		int s = BoxOnPlaneSide(mins, maxs, const_cast<cplane_t*>(&cplanes[planeNum]));
		if (s == 1) {
			childCode = LittleLong(node.children[0]);
		} else if (s == 2) {
			childCode = LittleLong(node.children[1]);
		} else {
			// Box straddles the plane: visit both children.
			MF_BoxSurfaces_r(LittleLong(node.children[0]),
			                 nodes, leaves, cplanes, leafSurfs, surfData,
			                 surfViewCounts, viewCount, mins, maxs,
			                 list, listsize, listlength, dir);
			childCode = LittleLong(node.children[1]);
		}
	}

	// Leaf: childCode < 0  =>  leafIdx = -childCode - 1
	const int leafIdx = -childCode - 1;
	if (leafIdx < 0 || leafIdx >= numLeaves) return;

	const dleaf_t& leaf    = leaves[leafIdx];
	const int      firstLS = LittleLong(leaf.firstLeafSurface);
	const int      numLS   = LittleLong(leaf.numLeafSurfaces);

	for (int i = 0; i < numLS; ++i) {
		if (*listlength >= listsize) break;

		const int lsIdx = firstLS + i;
		if (lsIdx < 0 || lsIdx >= numLSurfs) continue;
		const int surfIdx = leafSurfs[lsIdx];
		if (surfIdx < 0 || surfIdx >= numSurfs) continue;

		int& svc = surfViewCounts[surfIdx];
		const BspMarkSurface& surf = surfData[surfIdx];

		// Surfaces flagged NOIMPACT / NOMARKS, or fog volumes, are silently skipped.
		if ((surf.shaderSurfFlags & (SURF_NOIMPACT | SURF_NOMARKS))
		    || (surf.shaderContFlags & CONTENTS_FOG)) {
			svc = viewCount;
		}
		// For face surfaces: cull by plane-box side and projection angle.
		else if (surf.type == BspMarkSurfType::Face) {
			cplane_t cp;
			cp.normal[0] = surf.cullNormal[0];
			cp.normal[1] = surf.cullNormal[1];
			cp.normal[2] = surf.cullNormal[2];
			cp.dist      = surf.cullDist;
			cp.type      = surf.cullType;
			cp.signbits  = surf.cullSignbits;
			int s2 = BoxOnPlaneSide(mins, maxs, &cp);
			if (s2 == 1 || s2 == 2) {
				svc = viewCount;  // entirely on one side
			} else if (DotProduct(surf.cullNormal, dir) > -0.5f) {
				svc = viewCount;  // face too oblique to projection
			}
		}
		// Skip surface types we don't handle.
		else if (surf.type != BspMarkSurfType::Face
		      && surf.type != BspMarkSurfType::Patch
		      && surf.type != BspMarkSurfType::TriSoup) {
			svc = viewCount;
		}

		// Add to list if not already processed this frame.
		if (svc != viewCount) {
			svc = viewCount;
			list[*listlength] = &surf;
			(*listlength)++;
		}
	}
}

} // anonymous namespace for MarkFragments helpers

int MetalRenderer::markFragments(
    int numPoints, const vec3_t* points, const vec3_t projection,
    int maxPoints, vec3_t pointBuffer,
    int maxFragments, markFragment_t* fragmentBuffer)
{
	if (numPoints <= 0 || bspNodes_.empty() || bspMarkSurfData_.empty())
		return 0;

	// Increment viewCount for per-frame surface deduplication.
	++bspViewCount_;
	// Ensure the per-surface view-count array is sized correctly (grows with BSP loads).
	if ((int)bspSurfViewCounts_.size() != (int)bspMarkSurfData_.size())
		bspSurfViewCounts_.assign(bspMarkSurfData_.size(), -1);

	vec3_t projectionDir;
	VectorNormalize2(projection, projectionDir);

	// Compute AABB that encloses the winding polygon plus projection extent.
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);
	for (int i = 0; i < numPoints; ++i) {
		vec3_t temp;
		AddPointToBounds(points[i], mins, maxs);
		VectorAdd(points[i], projection, temp);
		AddPointToBounds(temp, mins, maxs);
		VectorMA(points[i], -20.0f, projectionDir, temp);
		AddPointToBounds(temp, mins, maxs);
	}

	if (numPoints > MF_MAX_VERTS_ON_POLY) numPoints = MF_MAX_VERTS_ON_POLY;

	// Build side-clipping planes for the winding polygon and the projection depth.
	vec3_t normals[MF_MAX_VERTS_ON_POLY + 2];
	float  dists  [MF_MAX_VERTS_ON_POLY + 2];
	for (int i = 0; i < numPoints; ++i) {
		vec3_t v1, v2;
		VectorSubtract(points[(i + 1) % numPoints], points[i], v1);
		VectorAdd(points[i], projection, v2);
		VectorSubtract(points[i], v2, v2);
		CrossProduct(v1, v2, normals[i]);
		VectorNormalizeFast(normals[i]);
		dists[i] = DotProduct(normals[i], points[i]);
	}
	// Near plane (32 units behind the surface)
	VectorCopy(projectionDir, normals[numPoints]);
	dists[numPoints] = DotProduct(projectionDir, points[0]) - 32.0f;
	// Far plane (20 units in front)
	VectorCopy(projectionDir, normals[numPoints + 1]);
	VectorInverse(normals[numPoints + 1]);
	dists[numPoints + 1] = DotProduct(normals[numPoints + 1], points[0]) - 20.0f;
	const int numPlanes = numPoints + 2;

	// Gather candidate surfaces via BSP traversal.
	constexpr int MAX_MARK_SURFS = 64;
	const BspMarkSurface* surfaces[MAX_MARK_SURFS];
	int numsurfaces = 0;

	MF_BoxSurfaces_r(/*root = node 0*/ 0,
	                 bspNodes_, bspLeafs_, bspCPlanes_,
	                 bspLeafSurfaces_, bspMarkSurfData_,
	                 bspSurfViewCounts_, bspViewCount_,
	                 mins, maxs,
	                 surfaces, MAX_MARK_SURFS, &numsurfaces,
	                 projectionDir);

	int returnedPoints    = 0;
	int returnedFragments = 0;

	for (int i = 0; i < numsurfaces; ++i) {
		const BspMarkSurface& surf = *surfaces[i];

		if (surf.type == BspMarkSurfType::Face) {
			// Check that the projection direction faces the surface.
			if (DotProduct(surf.cullNormal, projectionDir) > -0.5f) continue;

			// Each triangle is stored as 3 consecutive vertices.
			const int numTris = (int)surf.verts.size() / 3;
			for (int t = 0; t < numTris; ++t) {
				vec3_t clipPoints[2][MF_MAX_VERTS_ON_POLY];
				for (int j = 0; j < 3; ++j) {
					const BspMarkVert& v = surf.verts[t * 3 + j];
					clipPoints[0][j][0] = v.xyz[0];
					clipPoints[0][j][1] = v.xyz[1];
					clipPoints[0][j][2] = v.xyz[2];
				}
				MF_AddMarkFragments(3, clipPoints,
				                    numPlanes, normals, dists,
				                    maxPoints, pointBuffer,
				                    maxFragments, fragmentBuffer,
				                    &returnedPoints, &returnedFragments,
				                    mins, maxs);
				if (returnedFragments == maxFragments) return returnedFragments;
			}

		} else if (surf.type == BspMarkSurfType::Patch) {
			// Tessellated patch stored as flat triangle list (3 verts per tri).
			// Check each triangle's computed normal against the projection direction.
			const int numTris = (int)surf.verts.size() / 3;
			for (int t = 0; t < numTris; ++t) {
				vec3_t clipPoints[2][MF_MAX_VERTS_ON_POLY];
				for (int j = 0; j < 3; ++j) {
					const BspMarkVert& v = surf.verts[t * 3 + j];
					clipPoints[0][j][0] = v.xyz[0];
					clipPoints[0][j][1] = v.xyz[1];
					clipPoints[0][j][2] = v.xyz[2];
				}
				// Compute triangle normal and check against projection.
				vec3_t v1, v2, normal;
				VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
				VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
				CrossProduct(v1, v2, normal);
				VectorNormalizeFast(normal);
				if (DotProduct(normal, projectionDir) < -0.1f) {
					MF_AddMarkFragments(3, clipPoints,
					                    numPlanes, normals, dists,
					                    maxPoints, pointBuffer,
					                    maxFragments, fragmentBuffer,
					                    &returnedPoints, &returnedFragments,
					                    mins, maxs);
					if (returnedFragments == maxFragments) return returnedFragments;
				}
			}

		} else if (surf.type == BspMarkSurfType::TriSoup
		        && r_marksOnTriangleMeshes_ && r_marksOnTriangleMeshes_->integer) {
			// Triangle soup: iterate triangles.
			const int numTris = (int)surf.verts.size() / 3;
			for (int t = 0; t < numTris; ++t) {
				vec3_t clipPoints[2][MF_MAX_VERTS_ON_POLY];
				for (int j = 0; j < 3; ++j) {
					const BspMarkVert& v = surf.verts[t * 3 + j];
					clipPoints[0][j][0] = v.xyz[0];
					clipPoints[0][j][1] = v.xyz[1];
					clipPoints[0][j][2] = v.xyz[2];
				}
				MF_AddMarkFragments(3, clipPoints,
				                    numPlanes, normals, dists,
				                    maxPoints, pointBuffer,
				                    maxFragments, fragmentBuffer,
				                    &returnedPoints, &returnedFragments,
				                    mins, maxs);
				if (returnedFragments == maxFragments) return returnedFragments;
			}
		}
	}
	return returnedFragments;
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

void MetalBackend_DrawRotatePic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float degrees, qhandle_t shader) {
	g_renderer.drawRotatePic(x, y, w, h, s1, t1, s2, t2, degrees, shader);
}

void MetalBackend_DrawRotatePic2(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float degrees, qhandle_t shader) {
	g_renderer.drawRotatePic2(x, y, w, h, s1, t1, s2, t2, degrees, shader);
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

qboolean MetalBackend_GetEntityToken(char* buffer, int size) {
	Metal_LogRendererCall("re.GetEntityToken");
	return g_renderer.getEntityToken(buffer, size);
}

qboolean MetalBackend_inPVS(const vec3_t p1, const vec3_t p2) {
	// Avoid per-frame log spam — inPVS is called every game tick for every entity.
	return g_renderer.inPVS(p1, p2);
}

int MetalBackend_MarkFragments(int numPoints, const vec3_t* points, const vec3_t projection,
                               int maxPoints, vec3_t pointBuffer,
                               int maxFragments, markFragment_t* fragmentBuffer)
{
	return g_renderer.markFragments(numPoints, points, projection,
	                                maxPoints, pointBuffer,
	                                maxFragments, fragmentBuffer);
}

void Metal_LogRendererCall(const char* name) {
	g_renderer.logRendererCall(name);
}

// ---------------------------------------------------------------------------
// Flare system C-wrappers called from tr_flares.cpp
// ---------------------------------------------------------------------------

qhandle_t MetalBackend_GetFlareShader() {
	return g_renderer.getFlareShader();
}

void MetalBackend_DrawFlareQuad(float x, float y, float w, float h,
                                float r, float g, float b, qhandle_t shader) {
	g_renderer.drawFlareQuad(x, y, w, h, r, g, b, shader);
}

void MetalBackend_GetSceneCameraForFlares(float viewMat[16], float projMat[16],
                                          float viewOrg[3],
                                          int* vpX, int* vpY, int* vpW, int* vpH) {
	g_renderer.getSceneCameraForFlares(viewMat, projMat, viewOrg, vpX, vpY, vpW, vpH);
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
	r_greyscale_ = ri_.Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE);

	// Lighting console variables (matching OpenGL2 renderer)
	r_ambientScale = ri_.Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
	r_directedScale = ri_.Cvar_Get("r_directedScale", "1", CVAR_CHEAT);
	r_debugLight = ri_.Cvar_Get("r_debugLight", "0", CVAR_TEMP);
	r_dlightMode = ri_.Cvar_Get("r_dlightMode", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_marksOnTriangleMeshes_ = ri_.Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);
	// Shadows: 0=off, 1+=projection (blob) shadows. Mirrors cg_shadows in GL2.
	r_shadows_ = ri_.Cvar_Get("cg_shadows", "1", 0);
	// Max distance for per-object shadow consideration (mirrors GL2's r_pshadowDist).
	r_pshadowDist_ = ri_.Cvar_Get("r_pshadowDist", "512", CVAR_ARCHIVE);
	// Patch subdivision flatness tolerance (world units). Matches GL2's r_subdivisions.
	r_subdivisions_ = ri_.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

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

	// Lens flare cvars (matching GL2 renderer defaults)
	r_flares_    = ri_.Cvar_Get("r_flares",    "0",    CVAR_ARCHIVE);
	r_flareSize_ = ri_.Cvar_Get("r_flareSize", "40",   CVAR_CHEAT);
	r_flareFade_ = ri_.Cvar_Get("r_flareFade", "7",    CVAR_CHEAT);
	r_flareCoeff_ = ri_.Cvar_Get("r_flareCoeff", "150", CVAR_CHEAT);

	// Post-processing cvars
	r_hdr_                  = ri_.Cvar_Get("r_hdr",                 "1",    CVAR_ARCHIVE);
	r_bloom_                = ri_.Cvar_Get("r_bloom",               "0",    CVAR_ARCHIVE);
	r_bloomThreshold_       = ri_.Cvar_Get("r_bloomThreshold",      "1.0",  CVAR_ARCHIVE);
	r_bloomStrength_        = ri_.Cvar_Get("r_bloomStrength",       "0.5",  CVAR_ARCHIVE);
	r_ssao_                 = ri_.Cvar_Get("r_ssao",                "0",    CVAR_ARCHIVE | CVAR_LATCH);
	r_dof_                  = ri_.Cvar_Get("r_dof",                 "0",    CVAR_ARCHIVE);
	r_sunlightMode_         = ri_.Cvar_Get("r_sunlightMode",        "0",    CVAR_ARCHIVE);
	r_autoExposure_         = ri_.Cvar_Get("r_autoExposure",        "1",    CVAR_ARCHIVE);
	r_autoExposureMinValue_ = ri_.Cvar_Get("r_autoExposureMinValue","-2",   CVAR_ARCHIVE);
	r_autoExposureMaxValue_ = ri_.Cvar_Get("r_autoExposureMaxValue","2",    CVAR_ARCHIVE);
	r_cameraExposure_       = ri_.Cvar_Get("r_cameraExposure",      "0",    CVAR_ARCHIVE);
	r_tonemapExposure_      = ri_.Cvar_Get("r_tonemapExposure",     "0.18", CVAR_ARCHIVE);

	resetShaderCaches();
	// TextureManager will be created when device is available
	Metal_InitFlares();
	return true;
}
