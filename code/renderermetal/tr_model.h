/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2025 Modern Metal Renderer Implementation

Metal renderer model data structures and functions
===========================================================================
*/

#pragma once

extern "C" {
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
}

#include <vector>
#include <string>

// Maximum LOD levels for MD3 models
#define MD3_MAX_LODS 3

// Model types
enum class MetalModelType {
	BAD,
	MD3,
	MDR,
	IQM
};

// Model frame bounds and origin
struct MetalModelFrame {
	float bounds[2][3];     // Min and max bounds
	float localOrigin[3];   // Local origin
	float radius;           // Bounding sphere radius
};

// Model tag (attachment point)
struct MetalModelTag {
	float origin[3];        // Tag position
	float axis[3][3];       // Tag orientation matrix
};

// Model tag name
struct MetalModelTagName {
	char name[MAX_QPATH];   // Tag name
};

// Vertex data for a model
struct MetalModelVertex {
	float xyz[3];           // Position
	float normal[3];        // Normal vector
};

// Texture coordinates
struct MetalModelTexCoord {
	float st[2];            // Texture coordinates
};

// Tangent data
struct MetalModelTangent {
	float tangent[4];       // Tangent vector (xyz) + handedness (w)
};

// Model surface (mesh)
struct MetalModelSurface {
	char name[MAX_QPATH];                       // Surface name
	int numVerts;                               // Number of vertices
	int numIndexes;                             // Number of indices
	int numFrames;                              // Number of animation frames

	std::vector<unsigned int> indexes;          // Triangle indices
	std::vector<MetalModelVertex> vertices;     // Vertex positions and normals (numVerts * numFrames)
	std::vector<MetalModelTexCoord> texCoords;  // Texture coordinates (numVerts)
	std::vector<MetalModelTangent> tangents;    // Tangent vectors (numVerts * numFrames)

	std::vector<std::string> shaderNames;       // Shader names from MD3
	std::vector<int> shaderIndexes;             // Shader handles (filled by renderer)

	// Metal buffers (will be created by renderer)
	void* vertexBuffer;     // Metal vertex buffer
	void* indexBuffer;      // Metal index buffer

	MetalModelSurface() : numVerts(0), numIndexes(0), numFrames(0),
	                      vertexBuffer(nullptr), indexBuffer(nullptr) {
		name[0] = '\0';
	}
};

// Model LOD (Level of Detail)
struct MetalModelLOD {
	int numFrames;                              // Number of animation frames
	int numTags;                                // Number of tags
	int numSurfaces;                            // Number of surfaces

	std::vector<MetalModelFrame> frames;        // Frame data
	std::vector<MetalModelTag> tags;            // Tag data (numTags * numFrames)
	std::vector<MetalModelTagName> tagNames;    // Tag names (numTags)
	std::vector<MetalModelSurface> surfaces;    // Surface data

	MetalModelLOD() : numFrames(0), numTags(0), numSurfaces(0) {}
};

// Complete model
struct MetalModel {
	char name[MAX_QPATH];           // Model name
	MetalModelType type;            // Model type
	int index;                      // Model handle
	int numLods;                    // Number of LOD levels

	MetalModelLOD* lods[MD3_MAX_LODS];  // LOD data

	MetalModel() : type(MetalModelType::BAD), index(0), numLods(0) {
		name[0] = '\0';
		for (int i = 0; i < MD3_MAX_LODS; i++) {
			lods[i] = nullptr;
		}
	}
};

// Model loading functions
qboolean MetalModel_LoadMD3(MetalModel* model, int lod, void* buffer, int bufferSize, const char* modName);
qhandle_t MetalModel_RegisterMD3(const char* name, MetalModel* model, refimport_t& imports);

// Model management functions
MetalModel* MetalModel_Alloc(int index);
void MetalModel_Free(MetalModel* model);
void MetalModel_Bounds(const MetalModel* model, float* mins, float* maxs);
