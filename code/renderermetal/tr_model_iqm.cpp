/*
===========================================================================
Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
Copyright (C) 2011 Matthias Bentrup <matthias.bentrup@googlemail.com>
Copyright (C) 2011-2019 Zack Middleton <zturtleman@gmail.com>
Copyright (C) 2025 Metal renderer port

IQM (Inter-Quake Model) skeletal animation for the Metal renderer.
Ported from code/renderergl2/tr_model_iqm.c.

Metal differences from the OpenGL2 implementation:
  - No VAO/VBO creation at load time; vertex data is uploaded per-frame.
  - CPU-side skinning (ComputePoseMats + per-vertex transform) matching the
    existing MDR approach in tr_backend.cpp / renderMDRSurface.
  - Shader registration is deferred to MetalRenderer::registerIQMShaders.
===========================================================================
*/

#include "tr_model.h"
#include "tr_local.h"

extern "C" {
#include "../renderercommon/iqm.h"
#include "../qcommon/q_shared.h"
}

#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>

extern refimport_t ri;

#define LL(x) x = LittleLong(x)

// ---------------------------------------------------------------------------
// Math helpers  (mirrors renderergl2/tr_model_iqm.c)
// ---------------------------------------------------------------------------

static float s_identityMatrix[12] = { 1,0,0,0, 0,1,0,0, 0,0,1,0 };

static qboolean IQM_CheckRange(const iqmHeader_t* header, int offset,
                                int count, int size) {
	return (qboolean)(count <= 0 || offset <= 0 ||
	        offset > (int)header->filesize ||
	        offset + count * size < 0 ||
	        offset + count * size > (int)header->filesize);
}

static void Matrix34Multiply(const float* a, const float* b, float* out) {
	out[ 0] = a[0]*b[0] + a[1]*b[4] + a[ 2]*b[ 8];
	out[ 1] = a[0]*b[1] + a[1]*b[5] + a[ 2]*b[ 9];
	out[ 2] = a[0]*b[2] + a[1]*b[6] + a[ 2]*b[10];
	out[ 3] = a[0]*b[3] + a[1]*b[7] + a[ 2]*b[11] + a[ 3];
	out[ 4] = a[4]*b[0] + a[5]*b[4] + a[ 6]*b[ 8];
	out[ 5] = a[4]*b[1] + a[5]*b[5] + a[ 6]*b[ 9];
	out[ 6] = a[4]*b[2] + a[5]*b[6] + a[ 6]*b[10];
	out[ 7] = a[4]*b[3] + a[5]*b[7] + a[ 6]*b[11] + a[ 7];
	out[ 8] = a[8]*b[0] + a[9]*b[4] + a[10]*b[ 8];
	out[ 9] = a[8]*b[1] + a[9]*b[5] + a[10]*b[ 9];
	out[10] = a[8]*b[2] + a[9]*b[6] + a[10]*b[10];
	out[11] = a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11];
}

static void JointToMatrix(const quat_t rot, const vec3_t scale,
                          const vec3_t trans, float* mat) {
	float xx = 2.0f * rot[0] * rot[0];
	float yy = 2.0f * rot[1] * rot[1];
	float zz = 2.0f * rot[2] * rot[2];
	float xy = 2.0f * rot[0] * rot[1];
	float xz = 2.0f * rot[0] * rot[2];
	float yz = 2.0f * rot[1] * rot[2];
	float wx = 2.0f * rot[3] * rot[0];
	float wy = 2.0f * rot[3] * rot[1];
	float wz = 2.0f * rot[3] * rot[2];

	mat[ 0] = scale[0] * (1.0f - (yy + zz));
	mat[ 1] = scale[0] * (xy - wz);
	mat[ 2] = scale[0] * (xz + wy);
	mat[ 3] = trans[0];
	mat[ 4] = scale[1] * (xy + wz);
	mat[ 5] = scale[1] * (1.0f - (xx + zz));
	mat[ 6] = scale[1] * (yz - wx);
	mat[ 7] = trans[1];
	mat[ 8] = scale[2] * (xz - wy);
	mat[ 9] = scale[2] * (yz + wx);
	mat[10] = scale[2] * (1.0f - (xx + yy));
	mat[11] = trans[2];
}

static void Matrix34Invert(const float* inMat, float* outMat) {
	vec3_t trans;
	float invSqrLen;
	float* v;

	outMat[ 0] = inMat[ 0]; outMat[ 1] = inMat[ 4]; outMat[ 2] = inMat[ 8];
	outMat[ 4] = inMat[ 1]; outMat[ 5] = inMat[ 5]; outMat[ 6] = inMat[ 9];
	outMat[ 8] = inMat[ 2]; outMat[ 9] = inMat[ 6]; outMat[10] = inMat[10];

	v = outMat + 0; invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	v = outMat + 4; invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	v = outMat + 8; invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);

	trans[0] = inMat[ 3];
	trans[1] = inMat[ 7];
	trans[2] = inMat[11];

	outMat[ 3] = -DotProduct(outMat + 0, trans);
	outMat[ 7] = -DotProduct(outMat + 4, trans);
	outMat[11] = -DotProduct(outMat + 8, trans);
}

static void QuatSlerp(const quat_t from, const quat_t _to,
                      float fraction, quat_t out) {
	float angle, cosAngle, sinAngle, backlerp, lerp;
	quat_t to;

	cosAngle = from[0]*_to[0] + from[1]*_to[1] + from[2]*_to[2] + from[3]*_to[3];

	if (cosAngle < 0.0f) {
		cosAngle = -cosAngle;
		to[0] = -_to[0]; to[1] = -_to[1]; to[2] = -_to[2]; to[3] = -_to[3];
	} else {
		QuatCopy(_to, to);
	}

	if (cosAngle < 0.999999f) {
		angle    = acosf(cosAngle);
		sinAngle = sinf(angle);
		backlerp = sinf((1.0f - fraction) * angle) / sinAngle;
		lerp     = sinf(fraction * angle) / sinAngle;
	} else {
		backlerp = 1.0f - fraction;
		lerp     = fraction;
	}

	out[0] = from[0]*backlerp + to[0]*lerp;
	out[1] = from[1]*backlerp + to[1]*lerp;
	out[2] = from[2]*backlerp + to[2]*lerp;
	out[3] = from[3]*backlerp + to[3]*lerp;
}

static float QuatNormalize2(const quat_t v, quat_t out) {
	float length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3];
	if (length) {
		float ilength = 1.0f / sqrtf(length);
		length *= ilength;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
		out[3] = v[3]*ilength;
	} else {
		out[0] = out[1] = out[2] = 0;
		out[3] = -1;
	}
	return length;
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

// Compute 3x4 pose matrices (one per pose/joint) for a given frame pair.
// poseMats must point to num_poses * 12 floats.
static void ComputePoseMats_IQM(const MetalIQMData* data,
                                int frame, int oldframe, float backlerp,
                                float* poseMats) {
	if (data->num_poses == 0) return;

	IQMTransform_t relativeJoints[IQM_MAX_JOINTS];
	IQMTransform_t* relativeJoint = relativeJoints;
	const IQMTransform_t* pose;
	const IQMTransform_t* oldpose;
	const int*   jointParent;
	const float* invBindMat;
	float*       poseMat;
	float        lerp;

	if (oldframe == frame) {
		pose = &data->poses[frame * data->num_poses];
		for (int i = 0; i < data->num_poses; i++, pose++, relativeJoint++) {
			VectorCopy(pose->translate, relativeJoint->translate);
			QuatCopy(pose->rotate,     relativeJoint->rotate);
			VectorCopy(pose->scale,    relativeJoint->scale);
		}
	} else {
		lerp    = 1.0f - backlerp;
		pose    = &data->poses[frame    * data->num_poses];
		oldpose = &data->poses[oldframe * data->num_poses];
		for (int i = 0; i < data->num_poses; i++, oldpose++, pose++, relativeJoint++) {
			relativeJoint->translate[0] = oldpose->translate[0]*backlerp + pose->translate[0]*lerp;
			relativeJoint->translate[1] = oldpose->translate[1]*backlerp + pose->translate[1]*lerp;
			relativeJoint->translate[2] = oldpose->translate[2]*backlerp + pose->translate[2]*lerp;

			relativeJoint->scale[0] = oldpose->scale[0]*backlerp + pose->scale[0]*lerp;
			relativeJoint->scale[1] = oldpose->scale[1]*backlerp + pose->scale[1]*lerp;
			relativeJoint->scale[2] = oldpose->scale[2]*backlerp + pose->scale[2]*lerp;

			QuatSlerp(oldpose->rotate, pose->rotate, lerp, relativeJoint->rotate);
		}
	}

	// Multiply relative pose by bind pose hierarchy to get final skin matrices.
	relativeJoint = relativeJoints;
	jointParent   = data->jointParents;
	invBindMat    = data->invBindJoints;
	poseMat       = poseMats;

	for (int i = 0; i < data->num_poses;
	     i++, relativeJoint++, jointParent++, invBindMat += 12, poseMat += 12) {
		float mat1[12], mat2[12];

		JointToMatrix(relativeJoint->rotate, relativeJoint->scale,
		              relativeJoint->translate, mat1);

		if (*jointParent >= 0) {
			Matrix34Multiply(&data->bindJoints[(*jointParent) * 12], mat1, mat2);
			Matrix34Multiply(mat2, invBindMat, mat1);
			Matrix34Multiply(&poseMats[(*jointParent) * 12], mat1, poseMat);
		} else {
			Matrix34Multiply(mat1, invBindMat, poseMat);
		}
	}
}

// ---------------------------------------------------------------------------
// Public skinning API
// ---------------------------------------------------------------------------

/*
==================
Metal_IQMSurfaceAnim

CPU-side skeletal deformation for a single IQM surface.  outVerts must point
to at least surf->num_vertexes MetalIQMVert elements.  Both pos and pos2 (and
nrm/nrm2) are set to the same skinned values so the model vertex shader's
GPU lerp has no visible effect (vertexLerp is uploaded as 1.0).
==================
*/
void Metal_IQMSurfaceAnim(const MetalIQMData* data, const MetalIQMSurface* surf,
                          int frame, int oldframe, float backlerp,
                          MetalIQMVert* outVerts) {
	const float* xyz       = &data->positions[surf->first_vertex * 3];
	const float* normal    = &data->normals  [surf->first_vertex * 3];
	const float* texcoords = &data->texcoords[surf->first_vertex * 2];

	if (data->num_poses > 0 && data->influences != nullptr) {
		// Allocate temporary per-influence matrices on the heap to avoid
		// blowing the stack (up to SHADER_MAX_VERTEXES influences * 12 floats).
		std::vector<float> influenceVtxMat(surf->num_influences * 12);
		std::vector<float> influenceNrmMat(surf->num_influences * 9);

		// Compute skinning matrices for this frame pair.
		std::vector<float> poseMats(data->num_poses * 12);
		ComputePoseMats_IQM(data, frame, oldframe, backlerp, poseMats.data());

		// Pre-compute one blended vertex matrix and normal matrix per unique
		// influence combination used by this surface.
		for (int i = 0; i < surf->num_influences; i++) {
			const int influence = surf->first_influence + i;
			float* vtxMat = &influenceVtxMat[12 * i];
			float* nrmMat = &influenceNrmMat[ 9 * i];

			float bw[4];
			if (data->blendWeightsType == IQM_FLOAT) {
				bw[0] = data->influenceBlendWeights.f[4*influence + 0];
				bw[1] = data->influenceBlendWeights.f[4*influence + 1];
				bw[2] = data->influenceBlendWeights.f[4*influence + 2];
				bw[3] = data->influenceBlendWeights.f[4*influence + 3];
			} else {
				bw[0] = (float)data->influenceBlendWeights.b[4*influence + 0] / 255.0f;
				bw[1] = (float)data->influenceBlendWeights.b[4*influence + 1] / 255.0f;
				bw[2] = (float)data->influenceBlendWeights.b[4*influence + 2] / 255.0f;
				bw[3] = (float)data->influenceBlendWeights.b[4*influence + 3] / 255.0f;
			}

			if (bw[0] <= 0.0f) {
				// No blend: identity
				std::memcpy(vtxMat, s_identityMatrix, 12 * sizeof(float));
			} else {
				const int j0 = data->influenceBlendIndexes[4*influence + 0];
				const float* p0 = &poseMats[12 * j0];
				for (int k = 0; k < 12; k++) vtxMat[k] = bw[0] * p0[k];

				for (int j = 1; j < 4; j++) {
					if (bw[j] <= 0.0f) break;
					const int jj = data->influenceBlendIndexes[4*influence + j];
					const float* pj = &poseMats[12 * jj];
					for (int k = 0; k < 12; k++) vtxMat[k] += bw[j] * pj[k];
				}
			}

			// Normal matrix = cofactor (transpose of adjugate) of the 3x3 part
			nrmMat[0] = vtxMat[ 5]*vtxMat[10] - vtxMat[ 6]*vtxMat[ 9];
			nrmMat[1] = vtxMat[ 6]*vtxMat[ 8] - vtxMat[ 4]*vtxMat[10];
			nrmMat[2] = vtxMat[ 4]*vtxMat[ 9] - vtxMat[ 5]*vtxMat[ 8];
			nrmMat[3] = vtxMat[ 2]*vtxMat[ 9] - vtxMat[ 1]*vtxMat[10];
			nrmMat[4] = vtxMat[ 0]*vtxMat[10] - vtxMat[ 2]*vtxMat[ 8];
			nrmMat[5] = vtxMat[ 1]*vtxMat[ 8] - vtxMat[ 0]*vtxMat[ 9];
			nrmMat[6] = vtxMat[ 1]*vtxMat[ 6] - vtxMat[ 2]*vtxMat[ 5];
			nrmMat[7] = vtxMat[ 2]*vtxMat[ 4] - vtxMat[ 0]*vtxMat[ 6];
			nrmMat[8] = vtxMat[ 0]*vtxMat[ 5] - vtxMat[ 1]*vtxMat[ 4];
		}

		// Transform each vertex.
		for (int i = 0; i < surf->num_vertexes; i++) {
			const int influenceIdx =
				data->influences[surf->first_vertex + i] - surf->first_influence;
			const float* vtxMat = &influenceVtxMat[12 * influenceIdx];
			const float* nrmMat = &influenceNrmMat[ 9 * influenceIdx];

			const float* p = &xyz      [i * 3];
			const float* n = &normal   [i * 3];

			MetalIQMVert& out = outVerts[i];
			out.tc[0] = texcoords[i * 2 + 0];
			out.tc[1] = texcoords[i * 2 + 1];

			out.pos[0] = vtxMat[0]*p[0] + vtxMat[1]*p[1] + vtxMat[ 2]*p[2] + vtxMat[ 3];
			out.pos[1] = vtxMat[4]*p[0] + vtxMat[5]*p[1] + vtxMat[ 6]*p[2] + vtxMat[ 7];
			out.pos[2] = vtxMat[8]*p[0] + vtxMat[9]*p[1] + vtxMat[10]*p[2] + vtxMat[11];

			// Normal: DotProduct(&nrmMat[row*3], n)
			out.nrm[0] = nrmMat[0]*n[0] + nrmMat[1]*n[1] + nrmMat[2]*n[2];
			out.nrm[1] = nrmMat[3]*n[0] + nrmMat[4]*n[1] + nrmMat[5]*n[2];
			out.nrm[2] = nrmMat[6]*n[0] + nrmMat[7]*n[1] + nrmMat[8]*n[2];

			// Duplicate into pos2/nrm2 – the GPU vertex shader lerps between
			// these two slots, but since skinning is already done on the CPU
			// we pass identical values and set vertexLerp = 1.0.
			out.pos2[0] = out.pos[0]; out.pos2[1] = out.pos[1]; out.pos2[2] = out.pos[2];
			out.nrm2[0] = out.nrm[0]; out.nrm2[1] = out.nrm[1]; out.nrm2[2] = out.nrm[2];
		}
	} else {
		// Static mesh (no joints) – just copy bind-pose positions.
		for (int i = 0; i < surf->num_vertexes; i++) {
			MetalIQMVert& out = outVerts[i];
			out.pos[0] = out.pos2[0] = xyz   [i*3 + 0];
			out.pos[1] = out.pos2[1] = xyz   [i*3 + 1];
			out.pos[2] = out.pos2[2] = xyz   [i*3 + 2];
			out.nrm[0] = out.nrm2[0] = normal[i*3 + 0];
			out.nrm[1] = out.nrm2[1] = normal[i*3 + 1];
			out.nrm[2] = out.nrm2[2] = normal[i*3 + 2];
			out.tc[0]  = texcoords[i*2 + 0];
			out.tc[1]  = texcoords[i*2 + 1];
		}
	}
}

// ---------------------------------------------------------------------------
// File loading
// ---------------------------------------------------------------------------

/*
=================
MetalModel_LoadIQM

Parse an IQM file and populate model->iqmData.  Allocates a single
Hunk_Alloc'd blob containing MetalIQMData followed by all vertex arrays,
surface descriptors, joint hierarchy data and pose transforms.
=================
*/
qboolean MetalModel_LoadIQM(MetalModel* model, void* buffer,
                             int filesize, const char* mod_name) {
	iqmHeader_t*      header;
	iqmVertexArray_t* vertexarray;
	iqmTriangle_t*    triangle;
	iqmMesh_t*        mesh;
	iqmJoint_t*       joint;
	iqmPose_t*        pose;
	iqmBounds_t*      bounds;
	unsigned short*   framedata;
	char*             str;
	int               i, j, k;
	IQMTransform_t*   transform;
	float*            mat;
	float*            matInv;
	size_t            blobSize;
	size_t            joint_names;
	byte*             dataPtr;
	MetalIQMData*     iqmData;
	MetalIQMSurface*  surface;
	char              meshName[MAX_QPATH];
	int               vertexArrayFormat[IQM_COLOR + 1];
	int               allocateInfluences;
	byte*             blendIndexes;
	union {
		byte*  b;
		float* f;
	} blendWeights;

	if (filesize < (int)sizeof(iqmHeader_t)) {
		return qfalse;
	}

	header = (iqmHeader_t*)buffer;
	if (Q_strncmp(header->magic, IQM_MAGIC, sizeof(header->magic))) {
		return qfalse;
	}

	LL(header->version);
	if (header->version != IQM_VERSION) {
		ri.Printf(PRINT_WARNING,
		    "MetalModel_LoadIQM: %s is unsupported IQM version (%d), only version %d supported.\n",
		    mod_name, header->version, IQM_VERSION);
		return qfalse;
	}

	LL(header->filesize);
	if (header->filesize > (unsigned int)filesize || header->filesize > (16 << 20)) {
		return qfalse;
	}

	LL(header->flags);
	LL(header->num_text);          LL(header->ofs_text);
	LL(header->num_meshes);        LL(header->ofs_meshes);
	LL(header->num_vertexarrays);  LL(header->num_vertexes); LL(header->ofs_vertexarrays);
	LL(header->num_triangles);     LL(header->ofs_triangles); LL(header->ofs_adjacency);
	LL(header->num_joints);        LL(header->ofs_joints);
	LL(header->num_poses);         LL(header->ofs_poses);
	LL(header->num_anims);         LL(header->ofs_anims);
	LL(header->num_frames);        LL(header->num_framechannels);
	LL(header->ofs_frames);        LL(header->ofs_bounds);
	LL(header->num_comment);       LL(header->ofs_comment);
	LL(header->num_extensions);    LL(header->ofs_extensions);

	if (header->num_joints > IQM_MAX_JOINTS) {
		ri.Printf(PRINT_WARNING,
		    "MetalModel_LoadIQM: %s has more than %d joints (%d).\n",
		    mod_name, IQM_MAX_JOINTS, header->num_joints);
		return qfalse;
	}

	for (i = 0; i < (int)(sizeof(vertexArrayFormat) / sizeof(vertexArrayFormat[0])); i++) {
		vertexArrayFormat[i] = -1;
	}

	blendIndexes   = nullptr;
	blendWeights.b = nullptr;
	allocateInfluences = 0;

	// -----------------------------------------------------------------------
	// Validate and byte-swap vertex arrays, triangles, meshes, joints, poses
	// -----------------------------------------------------------------------
	if (header->num_meshes) {
		if (IQM_CheckRange(header, header->ofs_vertexarrays,
		                   header->num_vertexarrays, sizeof(iqmVertexArray_t))) {
			return qfalse;
		}
		vertexarray = (iqmVertexArray_t*)((byte*)header + header->ofs_vertexarrays);
		for (i = 0; i < (int)header->num_vertexarrays; i++, vertexarray++) {
			int n, *intPtr;
			if (vertexarray->size <= 0 || vertexarray->size > 4) return qfalse;
			n = header->num_vertexes * vertexarray->size;

			LL(vertexarray->type);
			LL(vertexarray->flags);
			LL(vertexarray->format);
			LL(vertexarray->size);
			LL(vertexarray->offset);

			switch (vertexarray->format) {
			case IQM_BYTE:
			case IQM_UBYTE:
				if (IQM_CheckRange(header, vertexarray->offset, n, sizeof(byte)))
					return qfalse;
				break;
			case IQM_INT:
			case IQM_UINT:
			case IQM_FLOAT:
				if (IQM_CheckRange(header, vertexarray->offset, n, sizeof(float)))
					return qfalse;
				intPtr = (int*)((byte*)header + vertexarray->offset);
				for (j = 0; j < n; j++, intPtr++) LL(*intPtr);
				break;
			default:
				return qfalse;
			}

			if (vertexarray->type < (unsigned int)(sizeof(vertexArrayFormat)/sizeof(vertexArrayFormat[0])))
				vertexArrayFormat[vertexarray->type] = (int)vertexarray->format;

			switch (vertexarray->type) {
			case IQM_POSITION:
			case IQM_NORMAL:
				if (vertexarray->format != IQM_FLOAT || vertexarray->size != 3) return qfalse;
				break;
			case IQM_TANGENT:
				if (vertexarray->format != IQM_FLOAT || vertexarray->size != 4) return qfalse;
				break;
			case IQM_TEXCOORD:
				if (vertexarray->format != IQM_FLOAT || vertexarray->size != 2) return qfalse;
				break;
			case IQM_BLENDINDEXES:
				if ((vertexarray->format != IQM_INT && vertexarray->format != IQM_UBYTE) ||
				    vertexarray->size != 4) return qfalse;
				blendIndexes = (byte*)header + vertexarray->offset;
				break;
			case IQM_BLENDWEIGHTS:
				if ((vertexarray->format != IQM_FLOAT && vertexarray->format != IQM_UBYTE) ||
				    vertexarray->size != 4) return qfalse;
				if (vertexarray->format == IQM_FLOAT)
					blendWeights.f = (float*)((byte*)header + vertexarray->offset);
				else
					blendWeights.b = (byte*)header + vertexarray->offset;
				break;
			case IQM_COLOR:
				if (vertexarray->format != IQM_UBYTE || vertexarray->size != 4) return qfalse;
				break;
			}
		}

		if (vertexArrayFormat[IQM_POSITION] == -1 ||
		    vertexArrayFormat[IQM_NORMAL]   == -1 ||
		    vertexArrayFormat[IQM_TEXCOORD] == -1) {
			ri.Printf(PRINT_WARNING,
			    "MetalModel_LoadIQM: %s is missing IQM_POSITION, IQM_NORMAL, and/or IQM_TEXCOORD array.\n",
			    mod_name);
			return qfalse;
		}

		if (header->num_joints) {
			if (vertexArrayFormat[IQM_BLENDINDEXES] == -1 ||
			    vertexArrayFormat[IQM_BLENDWEIGHTS] == -1) {
				ri.Printf(PRINT_WARNING,
				    "MetalModel_LoadIQM: %s is missing IQM_BLENDINDEXES and/or IQM_BLENDWEIGHTS array.\n",
				    mod_name);
				return qfalse;
			}
		} else {
			vertexArrayFormat[IQM_BLENDINDEXES] = -1;
			vertexArrayFormat[IQM_BLENDWEIGHTS] = -1;
		}

		// Byte-swap triangles
		if (IQM_CheckRange(header, header->ofs_triangles,
		                   header->num_triangles, sizeof(iqmTriangle_t))) {
			return qfalse;
		}
		triangle = (iqmTriangle_t*)((byte*)header + header->ofs_triangles);
		for (i = 0; i < (int)header->num_triangles; i++, triangle++) {
			LL(triangle->vertex[0]);
			LL(triangle->vertex[1]);
			LL(triangle->vertex[2]);
			if (triangle->vertex[0] > header->num_vertexes ||
			    triangle->vertex[1] > header->num_vertexes ||
			    triangle->vertex[2] > header->num_vertexes) {
				return qfalse;
			}
		}

		// Byte-swap meshes and count unique blend influences
		if (IQM_CheckRange(header, header->ofs_meshes,
		                   header->num_meshes, sizeof(iqmMesh_t))) {
			return qfalse;
		}
		mesh = (iqmMesh_t*)((byte*)header + header->ofs_meshes);
		for (i = 0; i < (int)header->num_meshes; i++, mesh++) {
			LL(mesh->name);
			LL(mesh->material);
			LL(mesh->first_vertex);
			LL(mesh->num_vertexes);
			LL(mesh->first_triangle);
			LL(mesh->num_triangles);

			if (mesh->name < header->num_text)
				Q_strncpyz(meshName, (char*)header + header->ofs_text + mesh->name, sizeof(meshName));
			else
				meshName[0] = '\0';

			if (mesh->num_vertexes >= SHADER_MAX_VERTEXES) {
				ri.Printf(PRINT_WARNING,
				    "MetalModel_LoadIQM: %s has more than %i verts on %s (%i).\n",
				    mod_name, SHADER_MAX_VERTEXES - 1,
				    meshName[0] ? meshName : "a surface", mesh->num_vertexes);
				return qfalse;
			}
			if (mesh->num_triangles * 3 >= SHADER_MAX_INDEXES) {
				ri.Printf(PRINT_WARNING,
				    "MetalModel_LoadIQM: %s has more than %i triangles on %s (%i).\n",
				    mod_name, (SHADER_MAX_INDEXES / 3) - 1,
				    meshName[0] ? meshName : "a surface", mesh->num_triangles);
				return qfalse;
			}
			if (mesh->first_vertex >= header->num_vertexes ||
			    mesh->first_vertex + mesh->num_vertexes > header->num_vertexes ||
			    mesh->first_triangle >= header->num_triangles ||
			    mesh->first_triangle + mesh->num_triangles > header->num_triangles ||
			    mesh->name >= header->num_text ||
			    mesh->material >= header->num_text) {
				return qfalse;
			}

			if (header->num_joints && blendIndexes) {
				for (j = 0; j < (int)mesh->num_vertexes; j++) {
					int vtx = (int)mesh->first_vertex + j;
					for (k = 0; k < j; k++) {
						int inf = (int)mesh->first_vertex + k;
						if (*(int*)&blendIndexes[4*inf] != *(int*)&blendIndexes[4*vtx])
							continue;
						if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
							if (blendWeights.f[4*inf+0] == blendWeights.f[4*vtx+0] &&
							    blendWeights.f[4*inf+1] == blendWeights.f[4*vtx+1] &&
							    blendWeights.f[4*inf+2] == blendWeights.f[4*vtx+2] &&
							    blendWeights.f[4*inf+3] == blendWeights.f[4*vtx+3]) break;
						} else {
							if (*(int*)&blendWeights.b[4*inf] == *(int*)&blendWeights.b[4*vtx]) break;
						}
					}
					if (k == j) allocateInfluences++;
				}
			}
		}
	}

	if (header->num_poses != header->num_joints && header->num_poses != 0) {
		ri.Printf(PRINT_WARNING,
		    "MetalModel_LoadIQM: %s has %d poses and %d joints, must match or poses=0\n",
		    mod_name, header->num_poses, header->num_joints);
		return qfalse;
	}

	joint_names = 0;
	if (header->num_joints) {
		if (IQM_CheckRange(header, header->ofs_joints,
		                   header->num_joints, sizeof(iqmJoint_t))) {
			return qfalse;
		}
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for (i = 0; i < (int)header->num_joints; i++, joint++) {
			LL(joint->name);
			LL(joint->parent);
			LL(joint->translate[0]); LL(joint->translate[1]); LL(joint->translate[2]);
			LL(joint->rotate[0]);    LL(joint->rotate[1]);
			LL(joint->rotate[2]);    LL(joint->rotate[3]);
			LL(joint->scale[0]);     LL(joint->scale[1]);     LL(joint->scale[2]);

			if (joint->parent < -1 ||
			    joint->parent >= (int)header->num_joints ||
			    joint->name   >= (int)header->num_text) {
				return qfalse;
			}
			joint_names += strlen((char*)header + header->ofs_text + joint->name) + 1;
		}
	}

	if (header->num_poses) {
		if (IQM_CheckRange(header, header->ofs_poses,
		                   header->num_poses, sizeof(iqmPose_t))) {
			return qfalse;
		}
		pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
		for (i = 0; i < (int)header->num_poses; i++, pose++) {
			LL(pose->parent); LL(pose->mask);
			for (j = 0; j < 10; j++) {
				LL(pose->channeloffset[j]);
				LL(pose->channelscale[j]);
			}
		}
	}

	if (header->ofs_bounds) {
		if (IQM_CheckRange(header, header->ofs_bounds,
		                   header->num_frames, sizeof(iqmBounds_t))) {
			return qfalse;
		}
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for (i = 0; i < (int)header->num_frames; i++, bounds++) {
			LL(bounds->bbmin[0]); LL(bounds->bbmin[1]); LL(bounds->bbmin[2]);
			LL(bounds->bbmax[0]); LL(bounds->bbmax[1]); LL(bounds->bbmax[2]);
		}
	}

	// -----------------------------------------------------------------------
	// Calculate allocation size for the single contiguous blob
	// -----------------------------------------------------------------------
	blobSize = sizeof(MetalIQMData);
	if (header->num_meshes) {
		blobSize += header->num_meshes  * sizeof(MetalIQMSurface);
		blobSize += header->num_triangles * 3 * sizeof(int);
		blobSize += header->num_vertexes  * 3 * sizeof(float);  // positions
		blobSize += header->num_vertexes  * 2 * sizeof(float);  // texcoords
		blobSize += header->num_vertexes  * 3 * sizeof(float);  // normals
		if (vertexArrayFormat[IQM_TANGENT] != -1)
			blobSize += header->num_vertexes * 4 * sizeof(float);  // tangents
		if (vertexArrayFormat[IQM_COLOR] != -1)
			blobSize += header->num_vertexes * 4 * sizeof(byte);   // colors
		if (allocateInfluences) {
			blobSize += header->num_vertexes   * sizeof(int);           // influences
			blobSize += allocateInfluences     * 4 * sizeof(byte);      // influenceBlendIndexes
			if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_UBYTE)
				blobSize += allocateInfluences * 4 * sizeof(byte);
			else if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT)
				blobSize += allocateInfluences * 4 * sizeof(float);
		}
	}
	if (header->num_joints) {
		blobSize += joint_names;
		blobSize += header->num_joints * sizeof(int);                    // jointParents
		blobSize += header->num_joints * 12 * sizeof(float);             // bindJoints
		blobSize += header->num_joints * 12 * sizeof(float);             // invBindJoints
	}
	if (header->num_poses) {
		blobSize += (size_t)header->num_poses * header->num_frames * sizeof(IQMTransform_t);
	}
	if (header->ofs_bounds)
		blobSize += header->num_frames * 6 * sizeof(float);
	else if (header->num_meshes && header->num_frames == 0)
		blobSize += 6 * sizeof(float);

	// -----------------------------------------------------------------------
	// Allocate and fill the blob
	// -----------------------------------------------------------------------
	iqmData = (MetalIQMData*)ri.Hunk_Alloc((int)blobSize, h_low);
	Com_Memset(iqmData, 0, blobSize);
	model->iqmData = iqmData;

	iqmData->num_vertexes = (int)(header->num_meshes ? header->num_vertexes : 0);
	iqmData->num_triangles = (int)(header->num_meshes ? header->num_triangles : 0);
	iqmData->num_frames    = (int)header->num_frames;
	iqmData->num_surfaces  = (int)header->num_meshes;
	iqmData->num_joints    = (int)header->num_joints;
	iqmData->num_poses     = (int)header->num_poses;
	iqmData->blendWeightsType = vertexArrayFormat[IQM_BLENDWEIGHTS];

	dataPtr = (byte*)iqmData + sizeof(MetalIQMData);

	if (header->num_meshes) {
		iqmData->surfaces = (MetalIQMSurface*)dataPtr;
		dataPtr += header->num_meshes * sizeof(MetalIQMSurface);

		iqmData->triangles = (int*)dataPtr;
		dataPtr += header->num_triangles * 3 * sizeof(int);

		iqmData->positions = (float*)dataPtr;
		dataPtr += header->num_vertexes * 3 * sizeof(float);

		iqmData->texcoords = (float*)dataPtr;
		dataPtr += header->num_vertexes * 2 * sizeof(float);

		iqmData->normals = (float*)dataPtr;
		dataPtr += header->num_vertexes * 3 * sizeof(float);

		if (vertexArrayFormat[IQM_TANGENT] != -1) {
			iqmData->tangents = (float*)dataPtr;
			dataPtr += header->num_vertexes * 4 * sizeof(float);
		}
		if (vertexArrayFormat[IQM_COLOR] != -1) {
			iqmData->colors = (byte*)dataPtr;
			dataPtr += header->num_vertexes * 4 * sizeof(byte);
		}

		if (allocateInfluences) {
			iqmData->influences = (int*)dataPtr;
			dataPtr += header->num_vertexes * sizeof(int);

			iqmData->influenceBlendIndexes = (byte*)dataPtr;
			dataPtr += allocateInfluences * 4 * sizeof(byte);

			if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_UBYTE) {
				iqmData->influenceBlendWeights.b = (byte*)dataPtr;
				dataPtr += allocateInfluences * 4 * sizeof(byte);
			} else if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
				iqmData->influenceBlendWeights.f = (float*)dataPtr;
				dataPtr += allocateInfluences * 4 * sizeof(float);
			}
		}
	}

	if (header->num_joints) {
		iqmData->jointNames   = (char*)dataPtr;  dataPtr += joint_names;
		iqmData->jointParents = (int*) dataPtr;  dataPtr += header->num_joints * sizeof(int);
		iqmData->bindJoints   = (float*)dataPtr; dataPtr += header->num_joints * 12 * sizeof(float);
		iqmData->invBindJoints= (float*)dataPtr; dataPtr += header->num_joints * 12 * sizeof(float);
	}
	if (header->num_poses) {
		iqmData->poses = (IQMTransform_t*)dataPtr;
		dataPtr += (size_t)header->num_poses * header->num_frames * sizeof(IQMTransform_t);
	}
	if (header->ofs_bounds) {
		iqmData->bounds = (float*)dataPtr;
		dataPtr += header->num_frames * 6 * sizeof(float);
	} else if (header->num_meshes && header->num_frames == 0) {
		iqmData->bounds = (float*)dataPtr;
		dataPtr += 6 * sizeof(float);
	}

	// -----------------------------------------------------------------------
	// Fill surfaces
	// -----------------------------------------------------------------------
	if (header->num_meshes) {
		mesh    = (iqmMesh_t*)((byte*)header + header->ofs_meshes);
		surface = iqmData->surfaces;
		str     = (char*)header + header->ofs_text;

		for (i = 0; i < (int)header->num_meshes; i++, mesh++, surface++) {
			Q_strncpyz(surface->name,         str + mesh->name,     sizeof(surface->name));
			Q_strncpyz(surface->materialName, str + mesh->material, sizeof(surface->materialName));
			Q_strlwr(surface->name);

			surface->first_vertex    = (int)mesh->first_vertex;
			surface->num_vertexes    = (int)mesh->num_vertexes;
			surface->first_triangle  = (int)mesh->first_triangle;
			surface->num_triangles   = (int)mesh->num_triangles;
		}

		// Copy triangles
		triangle = (iqmTriangle_t*)((byte*)header + header->ofs_triangles);
		for (i = 0; i < (int)header->num_triangles; i++, triangle++) {
			iqmData->triangles[3*i+0] = (int)triangle->vertex[0];
			iqmData->triangles[3*i+1] = (int)triangle->vertex[1];
			iqmData->triangles[3*i+2] = (int)triangle->vertex[2];
		}

		// Copy vertex arrays
		vertexarray = (iqmVertexArray_t*)((byte*)header + header->ofs_vertexarrays);
		for (i = 0; i < (int)header->num_vertexarrays; i++, vertexarray++) {
			int n = (int)(header->num_vertexes * vertexarray->size);
			if (vertexarray->type < (unsigned int)(sizeof(vertexArrayFormat)/sizeof(vertexArrayFormat[0])) &&
			    vertexArrayFormat[vertexarray->type] == -1) continue;
			switch (vertexarray->type) {
			case IQM_POSITION:
				Com_Memcpy(iqmData->positions, (byte*)header + vertexarray->offset, n * sizeof(float));
				break;
			case IQM_NORMAL:
				Com_Memcpy(iqmData->normals,   (byte*)header + vertexarray->offset, n * sizeof(float));
				break;
			case IQM_TANGENT:
				Com_Memcpy(iqmData->tangents,  (byte*)header + vertexarray->offset, n * sizeof(float));
				break;
			case IQM_TEXCOORD:
				Com_Memcpy(iqmData->texcoords, (byte*)header + vertexarray->offset, n * sizeof(float));
				break;
			case IQM_COLOR:
				if (iqmData->colors)
					Com_Memcpy(iqmData->colors, (byte*)header + vertexarray->offset, n * sizeof(byte));
				break;
			case IQM_BLENDINDEXES:
			case IQM_BLENDWEIGHTS:
				break; // handled via deduplication below
			}
		}

		// Build deduped influence table
		if (allocateInfluences) {
			int totalInfluences = 0;

			surface = iqmData->surfaces;
			for (i = 0; i < (int)header->num_meshes; i++, surface++) {
				surface->first_influence = totalInfluences;
				surface->num_influences  = 0;

				for (j = 0; j < surface->num_vertexes; j++) {
					int vtx = surface->first_vertex + j;
					for (k = 0; k < surface->num_influences; k++) {
						int inf = surface->first_influence + k;
						if (*(int*)&iqmData->influenceBlendIndexes[4*inf] !=
						    *(int*)&blendIndexes[4*vtx]) continue;
						if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
							if (iqmData->influenceBlendWeights.f[4*inf+0] == blendWeights.f[4*vtx+0] &&
							    iqmData->influenceBlendWeights.f[4*inf+1] == blendWeights.f[4*vtx+1] &&
							    iqmData->influenceBlendWeights.f[4*inf+2] == blendWeights.f[4*vtx+2] &&
							    iqmData->influenceBlendWeights.f[4*inf+3] == blendWeights.f[4*vtx+3]) break;
						} else {
							if (*(int*)&iqmData->influenceBlendWeights.b[4*inf] ==
							    *(int*)&blendWeights.b[4*vtx]) break;
						}
					}

					iqmData->influences[vtx] = surface->first_influence + k;

					if (k == surface->num_influences) {
						int inf = surface->first_influence + k;
						iqmData->influenceBlendIndexes[4*inf+0] = blendIndexes[4*vtx+0];
						iqmData->influenceBlendIndexes[4*inf+1] = blendIndexes[4*vtx+1];
						iqmData->influenceBlendIndexes[4*inf+2] = blendIndexes[4*vtx+2];
						iqmData->influenceBlendIndexes[4*inf+3] = blendIndexes[4*vtx+3];
						if (vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
							iqmData->influenceBlendWeights.f[4*inf+0] = blendWeights.f[4*vtx+0];
							iqmData->influenceBlendWeights.f[4*inf+1] = blendWeights.f[4*vtx+1];
							iqmData->influenceBlendWeights.f[4*inf+2] = blendWeights.f[4*vtx+2];
							iqmData->influenceBlendWeights.f[4*inf+3] = blendWeights.f[4*vtx+3];
						} else {
							iqmData->influenceBlendWeights.b[4*inf+0] = blendWeights.b[4*vtx+0];
							iqmData->influenceBlendWeights.b[4*inf+1] = blendWeights.b[4*vtx+1];
							iqmData->influenceBlendWeights.b[4*inf+2] = blendWeights.b[4*vtx+2];
							iqmData->influenceBlendWeights.b[4*inf+3] = blendWeights.b[4*vtx+3];
						}
						totalInfluences++;
						surface->num_influences++;
					}
				}
			}
		}
	}

	// -----------------------------------------------------------------------
	// Fill joint hierarchy
	// -----------------------------------------------------------------------
	if (header->num_joints) {
		str   = iqmData->jointNames;
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for (i = 0; i < (int)header->num_joints; i++, joint++) {
			char* name  = (char*)header + header->ofs_text + joint->name;
			int   nlen  = (int)strlen(name) + 1;
			Com_Memcpy(str, name, nlen);
			str += nlen;
		}

		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for (i = 0; i < (int)header->num_joints; i++, joint++) {
			iqmData->jointParents[i] = joint->parent;
		}

		mat    = iqmData->bindJoints;
		matInv = iqmData->invBindJoints;
		joint  = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for (i = 0; i < (int)header->num_joints; i++, joint++) {
			float baseFrame[12], invBaseFrame[12];

			QuatNormalize2(joint->rotate, joint->rotate);
			JointToMatrix(joint->rotate, joint->scale, joint->translate, baseFrame);
			Matrix34Invert(baseFrame, invBaseFrame);

			if (joint->parent >= 0) {
				Matrix34Multiply(iqmData->bindJoints + 12*joint->parent, baseFrame, mat);
				mat += 12;
				Matrix34Multiply(invBaseFrame, iqmData->invBindJoints + 12*joint->parent, matInv);
				matInv += 12;
			} else {
				Com_Memcpy(mat,    baseFrame,    sizeof(baseFrame));    mat    += 12;
				Com_Memcpy(matInv, invBaseFrame, sizeof(invBaseFrame)); matInv += 12;
			}
		}
	}

	// -----------------------------------------------------------------------
	// Fill pose transforms
	// -----------------------------------------------------------------------
	if (header->num_poses) {
		transform = iqmData->poses;
		framedata = (unsigned short*)((byte*)header + header->ofs_frames);
		for (i = 0; i < (int)header->num_frames; i++) {
			pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
			for (j = 0; j < (int)header->num_poses; j++, pose++, transform++) {
				vec3_t t; quat_t r; vec3_t s;
				t[0] = pose->channeloffset[0];
				if (pose->mask & 0x001) t[0] += *framedata++ * pose->channelscale[0];
				t[1] = pose->channeloffset[1];
				if (pose->mask & 0x002) t[1] += *framedata++ * pose->channelscale[1];
				t[2] = pose->channeloffset[2];
				if (pose->mask & 0x004) t[2] += *framedata++ * pose->channelscale[2];
				r[0] = pose->channeloffset[3];
				if (pose->mask & 0x008) r[0] += *framedata++ * pose->channelscale[3];
				r[1] = pose->channeloffset[4];
				if (pose->mask & 0x010) r[1] += *framedata++ * pose->channelscale[4];
				r[2] = pose->channeloffset[5];
				if (pose->mask & 0x020) r[2] += *framedata++ * pose->channelscale[5];
				r[3] = pose->channeloffset[6];
				if (pose->mask & 0x040) r[3] += *framedata++ * pose->channelscale[6];
				s[0] = pose->channeloffset[7];
				if (pose->mask & 0x080) s[0] += *framedata++ * pose->channelscale[7];
				s[1] = pose->channeloffset[8];
				if (pose->mask & 0x100) s[1] += *framedata++ * pose->channelscale[8];
				s[2] = pose->channeloffset[9];
				if (pose->mask & 0x200) s[2] += *framedata++ * pose->channelscale[9];
				VectorCopy(t, transform->translate);
				QuatNormalize2(r, transform->rotate);
				VectorCopy(s, transform->scale);
			}
		}
	}

	// -----------------------------------------------------------------------
	// Fill bounds
	// -----------------------------------------------------------------------
	if (header->ofs_bounds) {
		mat    = iqmData->bounds;
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for (i = 0; i < (int)header->num_frames; i++, bounds++) {
			mat[0] = bounds->bbmin[0]; mat[1] = bounds->bbmin[1]; mat[2] = bounds->bbmin[2];
			mat[3] = bounds->bbmax[0]; mat[4] = bounds->bbmax[1]; mat[5] = bounds->bbmax[2];
			mat += 6;
		}
	} else if (header->num_meshes && header->num_frames == 0) {
		float mins[3] = { 1e9f, 1e9f, 1e9f };
		float maxs[3] = {-1e9f,-1e9f,-1e9f};
		for (i = 0; i < (int)header->num_vertexes; i++) {
			const float* p = &iqmData->positions[i*3];
			if (p[0] < mins[0]) mins[0] = p[0]; if (p[0] > maxs[0]) maxs[0] = p[0];
			if (p[1] < mins[1]) mins[1] = p[1]; if (p[1] > maxs[1]) maxs[1] = p[1];
			if (p[2] < mins[2]) mins[2] = p[2]; if (p[2] > maxs[2]) maxs[2] = p[2];
		}
		if (iqmData->bounds) {
			iqmData->bounds[0] = mins[0]; iqmData->bounds[1] = mins[1]; iqmData->bounds[2] = mins[2];
			iqmData->bounds[3] = maxs[0]; iqmData->bounds[4] = maxs[1]; iqmData->bounds[5] = maxs[2];
		}
	}

	model->type    = MetalModelType::IQM;
	model->numLods = 1;

	ri.Printf(PRINT_DEVELOPER,
	    "MetalModel_LoadIQM: %s – %d verts, %d tris, %d joints, %d frames\n",
	    mod_name, iqmData->num_vertexes, iqmData->num_triangles,
	    iqmData->num_joints, iqmData->num_frames);

	return qtrue;
}

/*
=================
MetalModel_RegisterIQM

Load an IQM model by filename into the given MetalModel.
=================
*/
qhandle_t MetalModel_RegisterIQM(const char* name, MetalModel* model,
                                  refimport_t& imports) {
	union { unsigned* u; void* v; } buf;
	int size;

	size = imports.FS_ReadFile(name, &buf.v);
	if (!buf.v) {
		imports.Printf(PRINT_WARNING,
		    "MetalModel_RegisterIQM: couldn't load %s\n", name);
		return 0;
	}

	qboolean ok = MetalModel_LoadIQM(model, buf.v, size, name);
	imports.FS_FreeFile(buf.v);

	if (!ok) {
		model->type = MetalModelType::BAD;
		return 0;
	}

	return model->index;
}
