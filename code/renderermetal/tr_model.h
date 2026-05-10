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
	BRUSH,  // BSP brush model (doors, platforms, movers)
	MD3,
	MDR,
	IQM
};

// ============================================================
// IQM (Inter-Quake Model) skeletal animation data structures
// ============================================================

// Per-joint transform used for animation pose storage
typedef struct {
	vec3_t translate;
	quat_t rotate;
	vec3_t scale;
} IQMTransform_t;

// One mesh surface within an IQM model
struct MetalIQMSurface {
	char      name[MAX_QPATH];          // surface (mesh) name, lowercased
	char      materialName[MAX_QPATH];  // material/shader name from the IQM file
	qhandle_t shaderIndex;              // registered shader handle (filled by renderer)
	int       first_vertex,  num_vertexes;
	int       first_triangle, num_triangles;
	int       first_influence, num_influences;  // deduped blend influences for this surface
};

// All IQM model data stored in a single Hunk_Alloc'd contiguous blob.
// Pointer members point into the same allocation.
struct MetalIQMData {
	int num_vertexes;
	int num_triangles;
	int num_frames;
	int num_surfaces;
	int num_joints;
	int num_poses;
	int blendWeightsType;  // IQM_UBYTE or IQM_FLOAT

	MetalIQMSurface* surfaces;      // [num_surfaces]
	int*             triangles;     // [num_triangles * 3], global vertex indices

	// per-vertex arrays (all indexed by global vertex index)
	float* positions;               // [num_vertexes * 3]
	float* texcoords;               // [num_vertexes * 2]
	float* normals;                 // [num_vertexes * 3]
	float* tangents;                // [num_vertexes * 4]  (may be null if absent)
	byte*  colors;                  // [num_vertexes * 4]  (may be null if absent)

	// deduped blend influence table
	int*   influences;              // [num_vertexes] – index into influence table
	byte*  influenceBlendIndexes;   // [num_influences * 4]
	union {
		float* f;
		byte*  b;
	} influenceBlendWeights;        // [num_influences * 4]

	// joint hierarchy
	char*           jointNames;     // concatenated null-terminated strings
	int*            jointParents;   // [num_joints]
	float*          bindJoints;     // [num_joints * 12]  row-major 3x4
	float*          invBindJoints;  // [num_joints * 12]  row-major 3x4

	// animation
	IQMTransform_t* poses;          // [num_frames * num_poses]
	float*          bounds;         // [num_frames * 6]: bbmin[3] bbmax[3]
};

// Vertex output format for IQM CPU skinning – exactly matches the Metal model
// vertex descriptor (14 floats = 56 bytes, same as MDRVert).
struct MetalIQMVert {
	float pos[3];
	float nrm[3];
	float tc[2];
	float pos2[3];   // same as pos (no GPU lerp required after CPU skinning)
	float nrm2[3];   // same as nrm
};
static_assert(sizeof(MetalIQMVert) == 56, "MetalIQMVert stride must be 56 bytes");

// Brush model data (inline BSP model)
struct MetalBrushModel {
	float bounds[2][3];     // Bounding box
	int firstSurface;       // First surface in worldPacketTemplate_
	int numSurfaces;        // Number of surfaces
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

	MetalModelLOD* lods[MD3_MAX_LODS];  // LOD data (for MD3)
	MetalBrushModel* bmodel;        // Brush model data (for BRUSH type)
	void* mdrData;                  // For MDR models: Hunk_Alloc'd mdrHeader_t blob
	void* iqmData;                  // For IQM models: Hunk_Alloc'd MetalIQMData blob

	MetalModel() : type(MetalModelType::BAD), index(0), numLods(0), bmodel(nullptr), mdrData(nullptr), iqmData(nullptr) {
		name[0] = '\0';
		for (int i = 0; i < MD3_MAX_LODS; i++) {
			lods[i] = nullptr;
		}
	}
};

// Model loading functions
qboolean MetalModel_LoadMD3(MetalModel* model, int lod, void* buffer, int bufferSize, const char* modName);
qhandle_t MetalModel_RegisterMD3(const char* name, MetalModel* model, refimport_t& imports);

// MDR (skeletal bone animation) loading functions
qboolean MetalModel_LoadMDR(MetalModel* model, void* buffer, int filesize, const char* mod_name);
qhandle_t MetalModel_RegisterMDR(const char* name, MetalModel* model, refimport_t& imports);

// Bone matrix decompression (used by renderer for skinning verification)
void MC_UnCompress(float mat[3][4], const unsigned char* comp);

// Model management functions
MetalModel* MetalModel_Alloc(int index);
void MetalModel_Free(MetalModel* model);
void MetalModel_Bounds(const MetalModel* model, float* mins, float* maxs);

// GPU buffer management functions
qboolean MetalModel_CreateGPUBuffers(MetalModel* model, void* metalDevice);
qboolean MetalModel_CreateLODBuffers(MetalModelLOD* lod, void* metalDevice);
qboolean MetalModel_CreateSurfaceBuffers(MetalModelSurface* surf, void* metalDevice);
void MetalModel_FreeGPUBuffers(MetalModel* model);
void MetalModel_FreeLODBuffers(MetalModelLOD* lod);
void MetalModel_FreeSurfaceBuffers(MetalModelSurface* surf);

// IQM skeletal model loading functions
qboolean  MetalModel_LoadIQM(MetalModel* model, void* buffer, int filesize, const char* mod_name);
qhandle_t MetalModel_RegisterIQM(const char* name, MetalModel* model, refimport_t& imports);

// IQM CPU skinning: compute skinned vertices for surf into outVerts.
// outVerts must have at least surf->num_vertexes elements.
void Metal_IQMSurfaceAnim(const MetalIQMData* data, const MetalIQMSurface* surf,
                          int frame, int oldframe, float backlerp,
                          MetalIQMVert* outVerts);
