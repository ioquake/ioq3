/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_flares.cpp — Metal port of renderergl2/tr_flares.c
//
// Light flares are occlusion-tested in GL2 via glReadPixels. Metal does not
// expose a synchronous depth-buffer readback path without significant CPU/GPU
// synchronisation cost, so this port uses a simplified visibility model:
//   • A flare is considered visible when its world-space origin is in front of
//     the camera and projects within the viewport frustum (clip-space bounds).
//   • No per-pixel depth test is performed — flares can appear through walls.
//   • Fade-in / fade-out timing matches GL2's r_flareFade behaviour.
//   • Size and intensity calculations match GL2's formulas exactly.
//
// Flare sources: only scene dynamic lights (dlights) are supported. BSP
// surface flares (SF_FLARE) require additional surface-type handling in
// drawPolyPackets and can be added later.

#include "tr_backend.h"
#include "tr_scene.h"
#include "tr_extramath.h"

extern "C" {
#include "../qcommon/q_shared.h"
extern refimport_t ri;
}

#include <cmath>
#include <cstring>

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

#define FLARE_STDCOEFF 150.0f
#define MAX_FLARES     256

// -------------------------------------------------------------------------
// Flare state structure
// -------------------------------------------------------------------------

struct MetalFlare {
	MetalFlare* next;

	int   addedFrame;    // MetalSceneState::frameId when last added via RB_AddFlare
	int   sceneId;       // MetalSceneState::sceneCount when added
	int   lightIndex;    // Index into scene lights[] array (for matching same source)

	int      fadeTime;       // milliseconds (refdef.time) at last visibility change
	qboolean visible;        // was visible on last test
	float    drawIntensity;  // 0..1, non-zero even while fading out

	int   windowX, windowY;  // screen pixel coords, y = 0 at top of window
	float eyeZ;              // view-space Z (negative = in front of camera)

	vec3_t origin;  // world-space origin
	vec3_t color;   // per-light colour, scaled by intensity
};

// -------------------------------------------------------------------------
// Module globals
// -------------------------------------------------------------------------

static MetalFlare  r_flareStructs[MAX_FLARES];
static MetalFlare* r_activeFlares;
static MetalFlare* r_inactiveFlares;
static float       g_flareCoeff;

static cvar_t* r_flares_cvar;
static cvar_t* r_flareSize_cvar;
static cvar_t* r_flareFade_cvar;
static cvar_t* r_flareCoeff_cvar;

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

static void R_SetFlareCoeff() {
	float val = r_flareCoeff_cvar ? r_flareCoeff_cvar->value : 0.0f;
	g_flareCoeff = (val == 0.0f) ? FLARE_STDCOEFF : val;
}

// -------------------------------------------------------------------------
// R_ClearFlares — initialise / reset the active and inactive flare lists.
// Called once at renderer start-up. NOT called per-frame; the addedFrame
// expiry mechanism inside RB_RenderFlares handles per-frame housekeeping.
// -------------------------------------------------------------------------
void R_ClearFlares() {
	memset(r_flareStructs, 0, sizeof(r_flareStructs));
	r_activeFlares   = nullptr;
	r_inactiveFlares = nullptr;

	for (int i = 0; i < MAX_FLARES; i++) {
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[i];
	}
}

// -------------------------------------------------------------------------
// Metal_InitFlares — register cvars, set coefficient, clear state.
// Called from MetalBackend_Initialize.
// -------------------------------------------------------------------------
void Metal_InitFlares() {
	r_flares_cvar     = ri.Cvar_Get("r_flares",     "0",   CVAR_ARCHIVE);
	r_flareSize_cvar  = ri.Cvar_Get("r_flareSize",  "40",  CVAR_CHEAT);
	r_flareFade_cvar  = ri.Cvar_Get("r_flareFade",  "7",   CVAR_CHEAT);
	r_flareCoeff_cvar = ri.Cvar_Get("r_flareCoeff", "150", CVAR_CHEAT);
	R_SetFlareCoeff();
	R_ClearFlares();
}

// -------------------------------------------------------------------------
// RB_AddFlare (internal) — transforms a world-space light position to screen
// space and upserts it into the active flare list.
// -------------------------------------------------------------------------
static void RB_AddFlare(int lightIndex, const vec3_t origin, const vec3_t color) {
	const MetalSceneState& state = MetalScene_GetState();
	if (!state.refdefValid) {
		return;
	}

	// Retrieve current view / projection matrices and viewport bounds.
	float viewMat[16], projMat[16], viewOrg[3];
	int vpX, vpY, vpW, vpH;
	MetalBackend_GetSceneCameraForFlares(viewMat, projMat, viewOrg, &vpX, &vpY, &vpW, &vpH);
	if (vpW <= 0 || vpH <= 0) {
		return;
	}

	// Transform world position into eye (view) space.
	vec4_t worldPos = {origin[0], origin[1], origin[2], 1.0f};
	vec4_t eyePos;
	Mat4Transform(viewMat, worldPos, eyePos);
	const float eyeZ = eyePos[2];
	if (eyeZ >= 0.0f) {
		return; // behind camera — reject
	}

	// Transform eye-space position into clip space.
	vec4_t clipPos;
	Mat4Transform(projMat, eyePos, clipPos);
	const float clipW = clipPos[3];
	if (clipW <= 0.0f) {
		return; // degenerate / behind near plane
	}

	// Frustum cull: NDC x and y must be in [-1, 1].
	const float ndcX = clipPos[0] / clipW;
	const float ndcY = clipPos[1] / clipW;
	if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) {
		return; // outside viewport frustum
	}

	// Convert NDC to screen pixel coordinates.
	// Metal's y = 0 is at the top of the window; ndcY = +1 is also top.
	const int screenX = vpX + (int)((ndcX + 1.0f) * 0.5f * (float)vpW);
	const int screenY = vpY + (int)((1.0f - ndcY) * 0.5f * (float)vpH);

	// Find an existing flare slot for this light source and scene.
	MetalFlare* f = nullptr;
	for (MetalFlare* cur = r_activeFlares; cur; cur = cur->next) {
		if (cur->lightIndex == lightIndex && cur->sceneId == state.sceneCount) {
			f = cur;
			break;
		}
	}

	// Allocate a new slot if not found.
	if (!f) {
		if (!r_inactiveFlares) {
			return; // flare pool exhausted
		}
		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next         = r_activeFlares;
		r_activeFlares  = f;

		f->lightIndex = lightIndex;
		f->sceneId    = state.sceneCount;
		f->addedFrame = -1;
		f->visible    = qfalse;
		f->fadeTime   = state.refdef.time - 2000;
	}

	f->addedFrame = state.frameId;
	VectorCopy(origin, f->origin);
	VectorCopy(color,  f->color);
	f->windowX = screenX;
	f->windowY = screenY;
	f->eyeZ    = eyeZ;
}

// -------------------------------------------------------------------------
// RB_AddDlightFlares — add a flare for every dynamic light in the current
// scene. Called from endFrame, before RB_RenderFlares.
// -------------------------------------------------------------------------
void RB_AddDlightFlares() {
	if (!r_flares_cvar || !r_flares_cvar->integer) {
		return;
	}

	const MetalSceneState& state = MetalScene_GetState();
	for (int i = 0; i < state.numLights; i++) {
		const MetalSceneLight& light = state.lights[i];

		// Scale light colour by intensity to approximate a "glow" brightness.
		// Divide by 200 to keep values in a sane 0..1-ish range; tweak if needed.
		vec3_t scaledColor;
		const float scale = light.intensity / 200.0f;
		scaledColor[0] = light.color[0] * scale;
		scaledColor[1] = light.color[1] * scale;
		scaledColor[2] = light.color[2] * scale;

		vec3_t org;
		VectorCopy(light.origin, org);
		RB_AddFlare(i, org, scaledColor);
	}
}

// -------------------------------------------------------------------------
// RB_TestFlare (simplified) — update visibility state and drawIntensity.
// addedThisFrame: true if this flare's origin was in the frustum this frame.
// -------------------------------------------------------------------------
static void RB_TestFlare(MetalFlare* f, int refTime, bool addedThisFrame) {
	const float flareFade = r_flareFade_cvar ? r_flareFade_cvar->value : 7.0f;

	if (addedThisFrame) {
		// Flare is in view: fade in.
		if (!f->visible) {
			f->visible  = qtrue;
			f->fadeTime = refTime - 1;
		}
		float fade = ((float)(refTime - f->fadeTime) / 1000.0f) * flareFade;
		if (fade < 0.0f) fade = 0.0f;
		if (fade > 1.0f) fade = 1.0f;
		f->drawIntensity = fade;
	} else {
		// Flare is out of view: fade out.
		if (f->visible) {
			f->visible  = qfalse;
			f->fadeTime = refTime - 1;
		}
		float fade = 1.0f - ((float)(refTime - f->fadeTime) / 1000.0f) * flareFade;
		if (fade < 0.0f) fade = 0.0f;
		if (fade > 1.0f) fade = 1.0f;
		f->drawIntensity = fade;
	}
}

// -------------------------------------------------------------------------
// RB_RenderFlare — draw a single flare as a billboarded quad via the Metal
// additive 2D pipeline. Matches GL2's size / intensity formula exactly.
// -------------------------------------------------------------------------
static void RB_RenderFlare(const MetalFlare* f, int vpW) {
	if (f->drawIntensity <= 0.0f) {
		return;
	}

	// Distance is the absolute eye-space depth; clamp to at least 1.
	const float distance = (-f->eyeZ > 1.0f) ? -f->eyeZ : 1.0f;

	// Pixel half-size matching GL2: viewportWidth * (flareSize/640 + 8/distance)
	const float flareSize = r_flareSize_cvar ? r_flareSize_cvar->value : 40.0f;
	const float size = (float)vpW * (flareSize / 640.0f + 8.0f / distance);

	// Intensity fall-off: flareCoeff * size^2 / (distance + size*sqrt(flareCoeff))^2
	const float factor    = distance + size * sqrtf(g_flareCoeff);
	const float intensity = g_flareCoeff * size * size / (factor * factor);

	float r = f->color[0] * f->drawIntensity * intensity;
	float g = f->color[1] * f->drawIntensity * intensity;
	float b = f->color[2] * f->drawIntensity * intensity;

	// Clamp to [0, 1].
	if (r > 1.0f) r = 1.0f;  if (r < 0.0f) r = 0.0f;
	if (g > 1.0f) g = 1.0f;  if (g < 0.0f) g = 0.0f;
	if (b > 1.0f) b = 1.0f;  if (b < 0.0f) b = 0.0f;

	const qhandle_t shader = MetalBackend_GetFlareShader();
	MetalBackend_DrawFlareQuad(
		(float)f->windowX - size, (float)f->windowY - size,
		size * 2.0f, size * 2.0f,
		r, g, b, shader);
}

// -------------------------------------------------------------------------
// RB_RenderFlares — the main entry point, called once per frame from
// MetalRenderer::endFrame(), after all 3D geometry has been encoded.
// -------------------------------------------------------------------------
void RB_RenderFlares() {
	if (!r_flares_cvar || !r_flares_cvar->integer) {
		return;
	}

	if (r_flareCoeff_cvar && r_flareCoeff_cvar->modified) {
		R_SetFlareCoeff();
		r_flareCoeff_cvar->modified = qfalse;
	}

	const MetalSceneState& state = MetalScene_GetState();
	if (!state.refdefValid) {
		return;
	}

	// Retrieve viewport dimensions for flare size calculation.
	float viewMat[16], projMat[16], viewOrg[3];
	int vpX, vpY, vpW, vpH;
	MetalBackend_GetSceneCameraForFlares(viewMat, projMat, viewOrg, &vpX, &vpY, &vpW, &vpH);
	if (vpW <= 0) {
		return;
	}

	// Walk the active list: update visibility / fade, prune fully-faded entries.
	bool draw = false;
	MetalFlare** prev = &r_activeFlares;
	while (MetalFlare* f = *prev) {
		// A flare is "added this frame" when its origin projected into view.
		const bool addedThisFrame = (f->addedFrame == state.frameId);

		f->drawIntensity = 0.0f;

		// Only test / render flares that belong to the current scene view.
		if (f->sceneId == state.sceneCount) {
			RB_TestFlare(f, state.refdef.time, addedThisFrame);
		}

		// Prune flares that have fully faded out and were last seen > 1 frame ago.
		if (f->drawIntensity == 0.0f && f->addedFrame < state.frameId - 1) {
			*prev = f->next;
			f->next = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		if (f->drawIntensity > 0.0f) {
			draw = true;
		}

		prev = &f->next;
	}

	if (!draw) {
		return;
	}

	// Draw all visible flares.
	for (MetalFlare* f = r_activeFlares; f; f = f->next) {
		if (f->sceneId == state.sceneCount && f->drawIntensity > 0.0f) {
			RB_RenderFlare(f, vpW);
		}
	}
}
