#include "tr_scene.h"
#include "tr_backend.h"

#include <algorithm>
#include <cstring>

extern "C" {
	extern refimport_t ri;
}

namespace {
struct SceneStore {
	MetalSceneState state;
};

SceneStore g_sceneStore;

void ResetSceneCounts(MetalSceneState& state) {
	// Reset to the beginning of the frame
	// (called by BeginFrame, not ClearScene)
	state.numEntities = 0;
	state.numPolys = 0;
	state.numPolyVerts = 0;
	state.numLights = 0;
	state.firstSceneEntity = 0;
	state.firstScenePoly = 0;
	state.firstScenePolyVert = 0;
	state.firstSceneLight = 0;
	state.worldSceneFirstEntity = 0;
	state.worldSceneNumEntities = 0;
	// NOTE: Do NOT reset refdefValid here - it's set by RE_RenderScene
	// and should persist until the scene is processed
}

void CopyRefEntity(refEntity_t& dst, const refEntity_t* src) {
	if (!src) {
		std::memset(&dst, 0, sizeof(dst));
		return;
	}
	dst = *src;
}

void CopyPolyVerts(MetalSceneState& state, const polyVert_t* verts, int numVerts) {
	if (numVerts <= 0) {
		return;
	}
	const size_t bytes = static_cast<size_t>(numVerts) * sizeof(polyVert_t);
	std::memcpy(&state.polyVerts[state.numPolyVerts], verts, bytes);
}

void AddLightInternal(const vec3_t org, float intensity, float r, float g, float b, qboolean additive) {
	MetalSceneState& state = g_sceneStore.state;
	if (state.numLights >= MAX_DLIGHTS) {
		if (ri.Printf) {
			ri.Printf(PRINT_DEVELOPER, "MetalScene: MAX_DLIGHTS reached, ignoring light\n");
		}
		return;
	}

	MetalSceneLight& light = state.lights[state.numLights++];
	VectorCopy(org, light.origin);
	light.color[0] = r;
	light.color[1] = g;
	light.color[2] = b;
	light.intensity = intensity;
	light.additive = additive;
}
}

void MetalScene_BeginFrame() {
	MetalSceneState& state = g_sceneStore.state;
	++state.frameId;
	state.sceneCount = 0;
	ResetSceneCounts(state);
}

void MetalScene_EndFrame() {
	// Placeholder for future book-keeping (GPU timing, etc.)
}

MetalSceneState& MetalScene_MutableState() {
	return g_sceneStore.state;
}

const MetalSceneState& MetalScene_GetState() {
	return g_sceneStore.state;
}

void RE_ClearScene(void) {
	Metal_LogRendererCall("re.ClearScene");
	MetalSceneState& state = g_sceneStore.state;

	// if (ri.Printf) {
	// 	ri.Printf(PRINT_ALL, "DEBUG: ClearScene called - numEntities=%d (marking boundary, not clearing)\n",
	// 	          state.numEntities);
	// }

	// Like OpenGL2: Mark where the NEXT scene starts, but DON'T clear existing entities
	// This allows entities to accumulate across multiple scenes within a frame
	state.firstSceneEntity = state.numEntities;
	state.firstScenePoly = state.numPolys;
	state.firstScenePolyVert = state.numPolyVerts;
	state.firstSceneLight = state.numLights;
}

void RE_AddRefEntityToScene(const refEntity_t* re) {
	Metal_LogRendererCall("re.AddRefEntityToScene");
	MetalSceneState& state = g_sceneStore.state;
	if (!re) {
		return;
	}

	if (state.numEntities >= MAX_REFENTITIES) {
		if (ri.Printf) {
			ri.Printf(PRINT_DEVELOPER, "MetalScene: MAX_REFENTITIES reached, dropping entity\n");
		}
		return;
	}

	// DEBUG: Log entity being added
	// if (ri.Printf && re->reType == RT_MODEL) {
	// 	ri.Printf(PRINT_ALL, "DEBUG: Adding entity to scene - type=%d, hModel=%d, renderfx=0x%x\n",
	// 	          re->reType, re->hModel, re->renderfx);
	// }

	CopyRefEntity(state.entities[state.numEntities++], re);
}

void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys) {
	Metal_LogRendererCall("re.AddPolyToScene");
	MetalSceneState& state = g_sceneStore.state;
	if (!verts || numVerts <= 0 || numPolys <= 0) {
		return;
	}

	for (int i = 0; i < numPolys; ++i) {
		if (state.numPolys >= METAL_MAX_SCENE_POLYS ||
		    state.numPolyVerts + numVerts > METAL_MAX_SCENE_POLYVERTS) {
			if (ri.Printf) {
				ri.Printf(PRINT_DEVELOPER, "MetalScene: MAX_POLYS or MAX_POLYVERTS reached, dropping poly\n");
			}
			return;
		}

		MetalScenePolyRange& record = state.polys[state.numPolys++];
		record.shader = hShader;
		record.numVerts = numVerts;
		record.firstVertex = state.numPolyVerts;

		const polyVert_t* src = &verts[i * numVerts];
		CopyPolyVerts(state, src, numVerts);
		state.numPolyVerts += numVerts;
	}
}

void RE_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	Metal_LogRendererCall("re.AddLightToScene");
	AddLightInternal(org, intensity, r, g, b, qfalse);
}

void RE_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	Metal_LogRendererCall("re.AddAdditiveLightToScene");
	AddLightInternal(org, intensity, r, g, b, qtrue);
}

void RE_RenderScene(const refdef_t* fd) {
	MetalSceneState& state = g_sceneStore.state;
	if (!fd) {
		state.refdefValid = qfalse;
		return;
	}

	// DEBUG: Log scene rendering
	// if (ri.Printf) {
	// 	ri.Printf(PRINT_ALL, "DEBUG: RenderScene called - numEntities=%d, firstSceneEntity=%d, rdflags=0x%x, NOWORLDMODEL=%d\n",
	// 	          state.numEntities, state.firstSceneEntity, fd->rdflags, (fd->rdflags & RDF_NOWORLDMODEL) ? 1 : 0);
	// }

	// Only use the 3D world scene refdef, not UI/player config scenes
	// RDF_NOWORLDMODEL is set for UI scenes (player config, etc)
	if (fd->rdflags & RDF_NOWORLDMODEL) {
		// if (ri.Printf) {
		// 	ri.Printf(PRINT_ALL, "DEBUG: RenderScene - DISCARDING UI scene (firstScene=%d, num=%d)\n",
		// 	          state.firstSceneEntity, state.numEntities - state.firstSceneEntity);
		// }
		return;
	}

	// Save the world scene's entity range
	state.worldSceneFirstEntity = state.firstSceneEntity;
	state.worldSceneNumEntities = state.numEntities - state.firstSceneEntity;

	// if (ri.Printf) {
	// 	ri.Printf(PRINT_ALL, "DEBUG: RenderScene - ACCEPTING world scene (firstEntity=%d, numEntities=%d)\n",
	// 	          state.worldSceneFirstEntity, state.worldSceneNumEntities);
	// }

	state.refdef = *fd;
	state.refdefValid = qtrue;
	++state.sceneCount;
}
