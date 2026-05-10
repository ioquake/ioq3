/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2025 Modern Metal Renderer Implementation

Metal renderer model loading implementation
===========================================================================
*/

#include "tr_model.h"
#include "tr_backend.h"
#include "tr_local.h"

extern "C" {
#include "../qcommon/qfiles.h"
}

#include <cstring>
#include <algorithm>

#include <Metal/Metal.hpp>

extern refimport_t ri;

#define LL(x) x=LittleLong(x)

// Convert MD3 XYZ scale
#define MD3_XYZ_SCALE (1.0/64)

namespace {

// Decode packed MD3 normal (spherical coordinates)
void DecodeNormal(short normal, float* out) {
	unsigned lat = (normal >> 8) & 0xff;
	unsigned lng = (normal & 0xff);

	// Scale to proper range (use simple math, not FUNCTABLE constants)
	float lat_rad = (lat / 256.0f) * 2.0f * M_PI;
	float lng_rad = (lng / 256.0f) * 2.0f * M_PI;

	// decode X as cos( lat ) * sin( long )
	// decode Y as sin( lat ) * sin( long )
	// decode Z as cos( long )
	out[0] = cosf(lat_rad) * sinf(lng_rad);
	out[1] = sinf(lat_rad) * sinf(lng_rad);
	out[2] = cosf(lng_rad);
}

// Calculate tangent space for a triangle
void CalculateTangent(const float* v0, const float* v1, const float* v2,
                      const float* t0, const float* t1, const float* t2,
                      float* sdir, float* tdir) {
	float edge1[3], edge2[3];
	float deltaUV1[2], deltaUV2[2];

	// Calculate edges
	edge1[0] = v1[0] - v0[0];
	edge1[1] = v1[1] - v0[1];
	edge1[2] = v1[2] - v0[2];

	edge2[0] = v2[0] - v0[0];
	edge2[1] = v2[1] - v0[1];
	edge2[2] = v2[2] - v0[2];

	deltaUV1[0] = t1[0] - t0[0];
	deltaUV1[1] = t1[1] - t0[1];

	deltaUV2[0] = t2[0] - t0[0];
	deltaUV2[1] = t2[1] - t0[1];

	float f = 1.0f / (deltaUV1[0] * deltaUV2[1] - deltaUV2[0] * deltaUV1[1]);

	sdir[0] = f * (deltaUV2[1] * edge1[0] - deltaUV1[1] * edge2[0]);
	sdir[1] = f * (deltaUV2[1] * edge1[1] - deltaUV1[1] * edge2[1]);
	sdir[2] = f * (deltaUV2[1] * edge1[2] - deltaUV1[1] * edge2[2]);

	tdir[0] = f * (-deltaUV2[0] * edge1[0] + deltaUV1[0] * edge2[0]);
	tdir[1] = f * (-deltaUV2[0] * edge1[1] + deltaUV1[0] * edge2[1]);
	tdir[2] = f * (-deltaUV2[0] * edge1[2] + deltaUV1[0] * edge2[2]);
}

void NormalizeVector(float* v) {
	float length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (length > 0.0f) {
		float invLength = 1.0f / length;
		v[0] *= invLength;
		v[1] *= invLength;
		v[2] *= invLength;
	}
}

inline float Dot(const float* a, const float* b) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline void VecAdd(const float* a, const float* b, float* out) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

} // anonymous namespace

// Load MD3 model from buffer
qboolean MetalModel_LoadMD3(MetalModel* model, int lod, void* buffer, int bufferSize, const char* modName) {
	if (!model || !buffer || bufferSize < sizeof(md3Header_t)) {
		ri.Printf(PRINT_WARNING, "MetalModel_LoadMD3: invalid parameters for %s\n", modName);
		return qfalse;
	}

	md3Header_t* md3 = (md3Header_t*)buffer;

	// Verify header
	int version = LittleLong(md3->version);
	if (version != MD3_VERSION) {
		ri.Printf(PRINT_WARNING, "MetalModel_LoadMD3: %s has wrong version (%i should be %i)\n",
		          modName, version, MD3_VERSION);
		return qfalse;
	}

	// Byte swap header
	LL(md3->ident);
	LL(md3->version);
	LL(md3->numFrames);
	LL(md3->numTags);
	LL(md3->numSurfaces);
	LL(md3->ofsFrames);
	LL(md3->ofsTags);
	LL(md3->ofsSurfaces);
	LL(md3->ofsEnd);

	if (md3->numFrames < 1) {
		ri.Printf(PRINT_WARNING, "MetalModel_LoadMD3: %s has no frames\n", modName);
		return qfalse;
	}

	// Allocate LOD data
	if (!model->lods[lod]) {
		model->lods[lod] = new MetalModelLOD();
	}
	MetalModelLOD* lodData = model->lods[lod];

	// Load frames
	lodData->numFrames = md3->numFrames;
	lodData->frames.resize(md3->numFrames);

	md3Frame_t* md3Frame = (md3Frame_t*)((byte*)md3 + md3->ofsFrames);
	for (int i = 0; i < md3->numFrames; i++, md3Frame++) {
		MetalModelFrame& frame = lodData->frames[i];

		frame.radius = LittleFloat(md3Frame->radius);
		for (int j = 0; j < 3; j++) {
			frame.bounds[0][j] = LittleFloat(md3Frame->bounds[0][j]);
			frame.bounds[1][j] = LittleFloat(md3Frame->bounds[1][j]);
			frame.localOrigin[j] = LittleFloat(md3Frame->localOrigin[j]);
		}
	}

	// Load tags
	lodData->numTags = md3->numTags;
	lodData->tags.resize(md3->numTags * md3->numFrames);
	lodData->tagNames.resize(md3->numTags);

	md3Tag_t* md3Tag = (md3Tag_t*)((byte*)md3 + md3->ofsTags);
	for (int i = 0; i < md3->numTags * md3->numFrames; i++, md3Tag++) {
		MetalModelTag& tag = lodData->tags[i];

		for (int j = 0; j < 3; j++) {
			tag.origin[j] = LittleFloat(md3Tag->origin[j]);
			tag.axis[0][j] = LittleFloat(md3Tag->axis[0][j]);
			tag.axis[1][j] = LittleFloat(md3Tag->axis[1][j]);
			tag.axis[2][j] = LittleFloat(md3Tag->axis[2][j]);
		}

		// Store tag names (only once per tag, not per frame)
		if (i < md3->numTags) {
			std::strncpy(lodData->tagNames[i].name, md3Tag->name, sizeof(lodData->tagNames[i].name) - 1);
			lodData->tagNames[i].name[sizeof(lodData->tagNames[i].name) - 1] = '\0';
		}
	}

	// Load surfaces
	lodData->numSurfaces = md3->numSurfaces;
	lodData->surfaces.resize(md3->numSurfaces);

	md3Surface_t* md3Surf = (md3Surface_t*)((byte*)md3 + md3->ofsSurfaces);
	for (int i = 0; i < md3->numSurfaces; i++) {
		// Byte swap surface header
		LL(md3Surf->ident);
		LL(md3Surf->flags);
		LL(md3Surf->numFrames);
		LL(md3Surf->numShaders);
		LL(md3Surf->numTriangles);
		LL(md3Surf->ofsTriangles);
		LL(md3Surf->numVerts);
		LL(md3Surf->ofsShaders);
		LL(md3Surf->ofsSt);
		LL(md3Surf->ofsXyzNormals);
		LL(md3Surf->ofsEnd);

		// Validate surface
		if (md3Surf->numVerts >= SHADER_MAX_VERTEXES) {
			ri.Printf(PRINT_WARNING, "MetalModel_LoadMD3: %s has more than %i verts on surface %i (%i)\n",
			          modName, SHADER_MAX_VERTEXES - 1, i, md3Surf->numVerts);
			return qfalse;
		}
		if (md3Surf->numTriangles * 3 >= SHADER_MAX_INDEXES) {
			ri.Printf(PRINT_WARNING, "MetalModel_LoadMD3: %s has more than %i triangles on surface %i (%i)\n",
			          modName, (SHADER_MAX_INDEXES / 3) - 1, i, md3Surf->numTriangles);
			return qfalse;
		}

		MetalModelSurface& surf = lodData->surfaces[i];

		// Copy surface name
		std::strncpy(surf.name, md3Surf->name, sizeof(surf.name) - 1);
		surf.name[sizeof(surf.name) - 1] = '\0';

		// Lowercase the surface name for skin comparisons
		for (char* p = surf.name; *p; p++) {
			*p = tolower(*p);
		}

		// Strip off trailing _1 or _2 (q3data quirk)
		int nameLen = strlen(surf.name);
		if (nameLen > 2 && surf.name[nameLen - 2] == '_') {
			surf.name[nameLen - 2] = '\0';
		}

		surf.numVerts = md3Surf->numVerts;
		surf.numIndexes = md3Surf->numTriangles * 3;
		surf.numFrames = md3Surf->numFrames;

		// Load triangles (indexes)
		surf.indexes.resize(surf.numIndexes);
		md3Triangle_t* md3Tri = (md3Triangle_t*)((byte*)md3Surf + md3Surf->ofsTriangles);
		for (int j = 0; j < md3Surf->numTriangles; j++, md3Tri++) {
			surf.indexes[j * 3 + 0] = LittleLong(md3Tri->indexes[0]);
			surf.indexes[j * 3 + 1] = LittleLong(md3Tri->indexes[1]);
			surf.indexes[j * 3 + 2] = LittleLong(md3Tri->indexes[2]);
		}

		// Load texture coordinates
		surf.texCoords.resize(md3Surf->numVerts);
		md3St_t* md3st = (md3St_t*)((byte*)md3Surf + md3Surf->ofsSt);
		for (int j = 0; j < md3Surf->numVerts; j++, md3st++) {
			surf.texCoords[j].st[0] = LittleFloat(md3st->st[0]);
			surf.texCoords[j].st[1] = LittleFloat(md3st->st[1]);
		}

		// Load vertices (positions and normals per frame)
		surf.vertices.resize(md3Surf->numVerts * md3Surf->numFrames);
		md3XyzNormal_t* md3xyz = (md3XyzNormal_t*)((byte*)md3Surf + md3Surf->ofsXyzNormals);
		for (int j = 0; j < md3Surf->numVerts * md3Surf->numFrames; j++, md3xyz++) {
			MetalModelVertex& vert = surf.vertices[j];

			// Decode position
			vert.xyz[0] = LittleShort(md3xyz->xyz[0]) * MD3_XYZ_SCALE;
			vert.xyz[1] = LittleShort(md3xyz->xyz[1]) * MD3_XYZ_SCALE;
			vert.xyz[2] = LittleShort(md3xyz->xyz[2]) * MD3_XYZ_SCALE;

			// Decode normal
			unsigned short normal = LittleShort(md3xyz->normal);
			DecodeNormal(normal, vert.normal);
		}

		// Calculate tangents for each frame
		surf.tangents.resize(md3Surf->numVerts * md3Surf->numFrames);

		for (int frame = 0; frame < md3Surf->numFrames; frame++) {
			// Allocate temporary arrays for tangent calculation
			std::vector<float> sdirs(md3Surf->numVerts * 3, 0.0f);
			std::vector<float> tdirs(md3Surf->numVerts * 3, 0.0f);

			// Accumulate tangent contributions from all triangles
			for (int tri = 0; tri < md3Surf->numTriangles; tri++) {
				int idx0 = surf.indexes[tri * 3 + 0] + frame * md3Surf->numVerts;
				int idx1 = surf.indexes[tri * 3 + 1] + frame * md3Surf->numVerts;
				int idx2 = surf.indexes[tri * 3 + 2] + frame * md3Surf->numVerts;

				float sdir[3], tdir[3];
				CalculateTangent(
					surf.vertices[idx0].xyz, surf.vertices[idx1].xyz, surf.vertices[idx2].xyz,
					surf.texCoords[surf.indexes[tri * 3 + 0]].st,
					surf.texCoords[surf.indexes[tri * 3 + 1]].st,
					surf.texCoords[surf.indexes[tri * 3 + 2]].st,
					sdir, tdir
				);

				// Accumulate to all three vertices
				for (int k = 0; k < 3; k++) {
					int vertIdx = surf.indexes[tri * 3 + k];
					VecAdd(&sdirs[vertIdx * 3], sdir, &sdirs[vertIdx * 3]);
					VecAdd(&tdirs[vertIdx * 3], tdir, &tdirs[vertIdx * 3]);
				}
			}

			// Normalize and orthogonalize tangents
			for (int j = 0; j < md3Surf->numVerts; j++) {
				int vertIdx = j + frame * md3Surf->numVerts;
				float* sdir = &sdirs[j * 3];
				float* tdir = &tdirs[j * 3];

				NormalizeVector(sdir);
				NormalizeVector(tdir);

				const float* normal = surf.vertices[vertIdx].normal;

				// Gram-Schmidt orthogonalize
				float dot = Dot(normal, sdir);
				float tangent[3];
				tangent[0] = sdir[0] - normal[0] * dot;
				tangent[1] = sdir[1] - normal[1] * dot;
				tangent[2] = sdir[2] - normal[2] * dot;
				NormalizeVector(tangent);

				// Calculate handedness
				float cross[3];
				cross[0] = normal[1] * sdir[2] - normal[2] * sdir[1];
				cross[1] = normal[2] * sdir[0] - normal[0] * sdir[2];
				cross[2] = normal[0] * sdir[1] - normal[1] * sdir[0];
				float handedness = (Dot(cross, tdir) < 0.0f) ? -1.0f : 1.0f;

				surf.tangents[vertIdx].tangent[0] = tangent[0];
				surf.tangents[vertIdx].tangent[1] = tangent[1];
				surf.tangents[vertIdx].tangent[2] = tangent[2];
				surf.tangents[vertIdx].tangent[3] = handedness;
			}
		}

		// Register shaders for this surface
		surf.shaderIndexes.resize(md3Surf->numShaders);
		md3Shader_t* md3Shader = (md3Shader_t*)((byte*)md3Surf + md3Surf->ofsShaders);
		for (int j = 0; j < md3Surf->numShaders; j++, md3Shader++) {
			// Shader registration will be done later by the renderer
			// For now, just store the shader name
			surf.shaderNames.push_back(std::string(md3Shader->name));
			surf.shaderIndexes[j] = 0; // Will be filled in later
		}

		// Move to next surface
		md3Surf = (md3Surface_t*)((byte*)md3Surf + md3Surf->ofsEnd);
	}

	model->type = MetalModelType::MD3;
	model->numLods++;

	return qtrue;
}

// Register MD3 model with multiple LODs
qhandle_t MetalModel_RegisterMD3(const char* name, MetalModel* model, refimport_t& imports) {
	union {
		unsigned* u;
		void* v;
	} buf;
	int size;
	int lod;
	int ident;
	qboolean loaded = qfalse;
	int numLoaded = 0;
	char filename[MAX_QPATH], namebuf[MAX_QPATH + 20];
	char* fext;
	char defex[] = "md3";

	strncpy(filename, name, sizeof(filename) - 1);
	filename[sizeof(filename) - 1] = '\0';

	fext = strchr(filename, '.');
	if (!fext) {
		fext = defex;
	} else {
		*fext = '\0';
		fext++;
	}

	// Try loading LODs from highest to lowest
	for (lod = MD3_MAX_LODS - 1; lod >= 0; lod--) {
		if (lod) {
			Com_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		} else {
			Com_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);
		}

		size = imports.FS_ReadFile(namebuf, &buf.v);
		if (!buf.u) {
			continue;
		}

		ident = LittleLong(*(unsigned*)buf.u);
		if (ident == MD3_IDENT) {
			loaded = MetalModel_LoadMD3(model, lod, buf.u, size, name);
		} else {
			imports.Printf(PRINT_WARNING, "MetalModel_RegisterMD3: unknown fileid for %s\n", name);
		}

		imports.FS_FreeFile(buf.v);

		if (loaded) {
			numLoaded++;
		} else {
			break;
		}
	}

	if (numLoaded) {
		// Duplicate into higher lod spots that weren't loaded
		for (lod--; lod >= 0; lod--) {
			model->lods[lod] = model->lods[lod + 1];
			model->numLods++;
		}

		return model->index;
	}

	imports.Printf(PRINT_WARNING, "MetalModel_RegisterMD3: couldn't load %s\n", name);
	model->type = MetalModelType::BAD;
	return 0;
}

// Allocate a new model
MetalModel* MetalModel_Alloc(int index) {
	MetalModel* model = new MetalModel();
	model->index = index;
	model->type = MetalModelType::BAD;
	model->numLods = 0;
	for (int i = 0; i < MD3_MAX_LODS; i++) {
		model->lods[i] = nullptr;
	}
	return model;
}

// Free a model
void MetalModel_Free(MetalModel* model) {
	if (!model) return;

	for (int i = 0; i < MD3_MAX_LODS; i++) {
		if (model->lods[i]) {
			delete model->lods[i];
			model->lods[i] = nullptr;
		}
	}

	delete model;
}

// Get model bounds
void MetalModel_Bounds(const MetalModel* model, float* mins, float* maxs) {
	if (!model || model->type == MetalModelType::BAD) {
		mins[0] = mins[1] = mins[2] = 0.0f;
		maxs[0] = maxs[1] = maxs[2] = 0.0f;
		return;
	}

	if (model->type == MetalModelType::MD3 && model->lods[0]) {
		const MetalModelLOD* lod = model->lods[0];
		if (lod->numFrames > 0) {
			const MetalModelFrame& frame = lod->frames[0];
			for (int i = 0; i < 3; i++) {
				mins[i] = frame.bounds[0][i];
				maxs[i] = frame.bounds[1][i];
			}
			return;
		}
	}

	if (model->type == MetalModelType::MDR && model->mdrData) {
		const mdrHeader_t* mdr = (const mdrHeader_t*)model->mdrData;
		if (mdr->numFrames > 0) {
			// Compute frame size: header says how many bones
			const int frameSize = (int)(sizeof(mdrFrame_t) + (size_t)(mdr->numBones - 1) * sizeof(mdrBone_t));
			const mdrFrame_t* frame = (const mdrFrame_t*)((const byte*)mdr + mdr->ofsFrames);
			for (int i = 0; i < 3; i++) {
				mins[i] = frame->bounds[0][i];
				maxs[i] = frame->bounds[1][i];
			}
			(void)frameSize; // used via pointer arithmetic above
			return;
		}
	}

	mins[0] = mins[1] = mins[2] = 0.0f;
	maxs[0] = maxs[1] = maxs[2] = 0.0f;
}

// Create GPU buffers for a model surface (Metal implementation)
// This must be called with a valid Metal device from the renderer
qboolean MetalModel_CreateSurfaceBuffers(MetalModelSurface* surf, void* metalDevice) {
	if (!surf || !metalDevice) {
		return qfalse;
	}

	// Cast to Metal device (C++ interface)
	MTL::Device* device = static_cast<MTL::Device*>(metalDevice);

	// Create index buffer
	if (surf->numIndexes > 0 && !surf->indexBuffer) {
		size_t indexSize = surf->numIndexes * sizeof(unsigned int);
		MTL::Buffer* indexBuf = device->newBuffer(indexSize, MTL::ResourceStorageModeShared);
		if (!indexBuf) {
			ri.Printf(PRINT_WARNING, "MetalModel: Failed to create index buffer for surface %s\n", surf->name);
			return qfalse;
		}

		// Copy index data
		std::memcpy(indexBuf->contents(), surf->indexes.data(), indexSize);
		surf->indexBuffer = indexBuf;
	}

	// Note: We DON'T create vertex buffers here because we need to interleave data from multiple frames
	// The vertex buffer will be created at render time with the specific frame data needed

	return qtrue;
}

// Create GPU buffers for all surfaces in a model LOD
qboolean MetalModel_CreateLODBuffers(MetalModelLOD* lod, void* metalDevice) {
	if (!lod || !metalDevice) {
		return qfalse;
	}

	for (int i = 0; i < lod->numSurfaces; i++) {
		if (!MetalModel_CreateSurfaceBuffers(&lod->surfaces[i], metalDevice)) {
			return qfalse;
		}
	}

	return qtrue;
}

// Create GPU buffers for all LODs in a model
qboolean MetalModel_CreateGPUBuffers(MetalModel* model, void* metalDevice) {
	if (!model || !metalDevice || model->type == MetalModelType::BAD) {
		return qfalse;
	}

	for (int lod = 0; lod < model->numLods; lod++) {
		if (model->lods[lod]) {
			if (!MetalModel_CreateLODBuffers(model->lods[lod], metalDevice)) {
				return qfalse;
			}
		}
	}

	return qtrue;
}

// Free GPU buffers for a surface
void MetalModel_FreeSurfaceBuffers(MetalModelSurface* surf) {
	if (!surf) {
		return;
	}

	if (surf->vertexBuffer) {
		MTL::Buffer* buf = static_cast<MTL::Buffer*>(surf->vertexBuffer);
		buf->release();
		surf->vertexBuffer = nullptr;
	}

	if (surf->indexBuffer) {
		MTL::Buffer* buf = static_cast<MTL::Buffer*>(surf->indexBuffer);
		buf->release();
		surf->indexBuffer = nullptr;
	}
}

// Free GPU buffers for an LOD
void MetalModel_FreeLODBuffers(MetalModelLOD* lod) {
	if (!lod) {
		return;
	}

	for (int i = 0; i < lod->numSurfaces; i++) {
		MetalModel_FreeSurfaceBuffers(&lod->surfaces[i]);
	}
}

// Free all GPU buffers for a model
void MetalModel_FreeGPUBuffers(MetalModel* model) {
	if (!model) {
		return;
	}

	for (int lod = 0; lod < MD3_MAX_LODS; lod++) {
		if (model->lods[lod]) {
			MetalModel_FreeLODBuffers(model->lods[lod]);
		}
	}
}
