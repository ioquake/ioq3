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

// tr_animation.cpp - MDR skeletal animation support for the Metal renderer.
// Ported from code/renderergl2/tr_animation.c and tr_model.c (R_LoadMDR).

#include "tr_model.h"
#include "tr_local.h"

extern "C" {
#include "../qcommon/qfiles.h"
}

#include <cstring>

#define LL(x) x=LittleLong(x)

// SF_MDR: surface type identifier stored in mdrSurface_t.ident.
// Value matches renderergl2/tr_local.h surfaceType_t enum (SF_MDR == 7).
#define SF_MDR 7

// MDR frame size (bytes) for a frame with the given number of bones.
// mdrFrame_t contains bones[1], so we add (numBones-1) extra bones.
#define MDR_FRAME_SIZE(numBones) \
	((int)(sizeof(mdrFrame_t) + (size_t)((numBones) - 1) * sizeof(mdrBone_t)))

// ---------------------------------------------------------------------------
// Bone matrix decompression constants (matching renderergl2/tr_local.h)
// ---------------------------------------------------------------------------

#define MC_BITS_X    (16)
#define MC_BITS_Y    (16)
#define MC_BITS_Z    (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X    (1.0f / 64)
#define MC_SCALE_Y    (1.0f / 64)
#define MC_SCALE_Z    (1.0f / 64)
#define MC_SCALE_VECT (1.0f / (float)((1 << (MC_BITS_VECT - 1)) - 2))

// MC_UnCompress: decompress a 24-byte packed bone matrix into float[3][4].
// Each field is stored as an unsigned short (little-endian) with a fixed bias.
void MC_UnCompress(float mat[3][4], const unsigned char* comp)
{
	int val;

	val  = (int)((unsigned short*)comp)[0];
	val -= 1 << (MC_BITS_X - 1);
	mat[0][3] = (float)val * MC_SCALE_X;

	val  = (int)((unsigned short*)comp)[1];
	val -= 1 << (MC_BITS_Y - 1);
	mat[1][3] = (float)val * MC_SCALE_Y;

	val  = (int)((unsigned short*)comp)[2];
	val -= 1 << (MC_BITS_Z - 1);
	mat[2][3] = (float)val * MC_SCALE_Z;

	val  = (int)((unsigned short*)comp)[3];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[0][0] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[4];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[0][1] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[5];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[0][2] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[6];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[1][0] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[7];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[1][1] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[8];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[1][2] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[9];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[2][0] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[10];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[2][1] = (float)val * MC_SCALE_VECT;

	val  = (int)((unsigned short*)comp)[11];
	val -= 1 << (MC_BITS_VECT - 1);
	mat[2][2] = (float)val * MC_SCALE_VECT;
}

// ---------------------------------------------------------------------------
// MetalModel_LoadMDR
//
// Load an MDR model from an in-memory buffer into model->mdrData.
// The destination is Hunk_Alloc'd so it persists for the game session.
// This follows R_LoadMDR in renderergl2/tr_model.c very closely.
// ---------------------------------------------------------------------------
qboolean MetalModel_LoadMDR(MetalModel* model, void* buffer, int filesize, const char* mod_name)
{
	int i, j, k, l;
	mdrHeader_t   *pinmodel, *mdr;
	mdrFrame_t    *frame;
	mdrLOD_t      *lod, *curlod;
	mdrSurface_t  *surf, *cursurf;
	mdrTriangle_t *tri, *curtri;
	mdrVertex_t   *v, *curv;
	mdrWeight_t   *weight, *curweight;
	mdrTag_t      *tag, *curtag;
	int size;

	pinmodel = (mdrHeader_t*)buffer;

	pinmodel->version = LittleLong(pinmodel->version);
	if (pinmodel->version != MDR_VERSION) {
		ri.Printf(PRINT_WARNING,
			"MetalModel_LoadMDR: %s has wrong version (%i should be %i)\n",
			mod_name, pinmodel->version, MDR_VERSION);
		return qfalse;
	}

	size = LittleLong(pinmodel->ofsEnd);
	if (size > filesize) {
		ri.Printf(PRINT_WARNING,
			"MetalModel_LoadMDR: Header of %s is broken. Wrong filesize declared!\n",
			mod_name);
		return qfalse;
	}

	LL(pinmodel->numFrames);
	LL(pinmodel->numBones);
	LL(pinmodel->ofsFrames);

	// Negative ofsFrames signals compressed bone matrices.
	// mdrFrame_t is larger than mdrCompFrame_t, so we must inflate the allocation.
	if (pinmodel->ofsFrames < 0) {
		size += pinmodel->numFrames * (int)sizeof(frame->name);
		size += pinmodel->numFrames * pinmodel->numBones *
		        (int)(sizeof(mdrBone_t) - sizeof(mdrCompBone_t));
	}

	// Simple structural sanity check
	if (pinmodel->numBones < 0 ||
	    (int)(sizeof(*mdr) +
	          pinmodel->numFrames * (sizeof(*frame) +
	          (pinmodel->numBones - 1) * sizeof(*frame->bones))) > size) {
		ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has broken structure.\n", mod_name);
		return qfalse;
	}

	mdr = (mdrHeader_t*)ri.Hunk_Alloc(size, h_low);
	model->mdrData = mdr;

	// Copy header
	mdr->ident    = LittleLong(pinmodel->ident);
	mdr->version  = pinmodel->version;
	Q_strncpyz(mdr->name, pinmodel->name, sizeof(mdr->name));
	mdr->numFrames = pinmodel->numFrames;
	mdr->numBones  = pinmodel->numBones;
	mdr->numLODs   = LittleLong(pinmodel->numLODs);
	mdr->numTags   = LittleLong(pinmodel->numTags);

	model->numLods = mdr->numLODs;

	if (mdr->numFrames < 1) {
		ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has no frames\n", mod_name);
		return qfalse;
	}

	// Frames start immediately after the header
	frame = (mdrFrame_t*)(mdr + 1);
	mdr->ofsFrames = (int)((byte*)frame - (byte*)mdr);

	if (pinmodel->ofsFrames < 0) {
		// Compressed bone format
		mdrCompFrame_t* cframe =
			(mdrCompFrame_t*)((byte*)pinmodel - pinmodel->ofsFrames);

		for (i = 0; i < mdr->numFrames; i++) {
			for (j = 0; j < 3; j++) {
				frame->bounds[0][j]   = LittleFloat(cframe->bounds[0][j]);
				frame->bounds[1][j]   = LittleFloat(cframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(cframe->localOrigin[j]);
			}
			frame->radius  = LittleFloat(cframe->radius);
			frame->name[0] = '\0';

			for (j = 0; j < mdr->numBones; j++) {
				for (k = 0; k < (int)(sizeof(cframe->bones[j].Comp) / 2); k++) {
					((unsigned short*)(cframe->bones[j].Comp))[k] =
						LittleShort(((unsigned short*)(cframe->bones[j].Comp))[k]);
				}
				MC_UnCompress(frame->bones[j].matrix, cframe->bones[j].Comp);
			}

			cframe = (mdrCompFrame_t*)&cframe->bones[j];
			frame  = (mdrFrame_t*)&frame->bones[j];
		}
	} else {
		// Uncompressed bone format
		mdrFrame_t* curframe = (mdrFrame_t*)((byte*)pinmodel + pinmodel->ofsFrames);

		for (i = 0; i < mdr->numFrames; i++) {
			for (j = 0; j < 3; j++) {
				frame->bounds[0][j]   = LittleFloat(curframe->bounds[0][j]);
				frame->bounds[1][j]   = LittleFloat(curframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j]);
			}
			frame->radius = LittleFloat(curframe->radius);
			Q_strncpyz(frame->name, curframe->name, sizeof(frame->name));

			for (j = 0; j < (int)(mdr->numBones * (int)sizeof(mdrBone_t) / 4); j++) {
				((float*)frame->bones)[j] = LittleFloat(((float*)curframe->bones)[j]);
			}

			curframe = (mdrFrame_t*)&curframe->bones[mdr->numBones];
			frame    = (mdrFrame_t*)&frame->bones[mdr->numBones];
		}
	}

	// LODs immediately follow all frame data
	lod = (mdrLOD_t*)frame;
	mdr->ofsLODs = (int)((byte*)lod - (byte*)mdr);

	curlod = (mdrLOD_t*)((byte*)pinmodel + LittleLong(pinmodel->ofsLODs));

	for (l = 0; l < mdr->numLODs; l++) {
		if ((byte*)(lod + 1) > (byte*)mdr + size) {
			ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has broken structure.\n", mod_name);
			return qfalse;
		}

		lod->numSurfaces = LittleLong(curlod->numSurfaces);
		surf             = (mdrSurface_t*)(lod + 1);
		lod->ofsSurfaces = (int)((byte*)surf - (byte*)lod);
		cursurf          = (mdrSurface_t*)((byte*)curlod + LittleLong(curlod->ofsSurfaces));

		for (i = 0; i < lod->numSurfaces; i++) {
			if ((byte*)(surf + 1) > (byte*)mdr + size) {
				ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}

			surf->ident    = SF_MDR;
			Q_strncpyz(surf->name,   cursurf->name,   sizeof(surf->name));
			Q_strncpyz(surf->shader, cursurf->shader, sizeof(surf->shader));

			surf->ofsHeader    = (int)((byte*)mdr - (byte*)surf);
			surf->numVerts     = LittleLong(cursurf->numVerts);
			surf->numTriangles = LittleLong(cursurf->numTriangles);

			if (surf->numVerts >= SHADER_MAX_VERTEXES) {
				ri.Printf(PRINT_WARNING,
					"MetalModel_LoadMDR: %s has more than %i verts on %s (%i).\n",
					mod_name, SHADER_MAX_VERTEXES - 1,
					surf->name[0] ? surf->name : "a surface", surf->numVerts);
				return qfalse;
			}
			if (surf->numTriangles * 3 >= SHADER_MAX_INDEXES) {
				ri.Printf(PRINT_WARNING,
					"MetalModel_LoadMDR: %s has more than %i triangles on %s (%i).\n",
					mod_name, (SHADER_MAX_INDEXES / 3) - 1,
					surf->name[0] ? surf->name : "a surface", surf->numTriangles);
				return qfalse;
			}

			Q_strlwr(surf->name);
			surf->shaderIndex = 0; // Populated after load by MetalRenderer::registerMDRShaders

			// Copy vertices (variable size per vertex due to weights)
			v              = (mdrVertex_t*)(surf + 1);
			surf->ofsVerts = (int)((byte*)v - (byte*)surf);
			curv           = (mdrVertex_t*)((byte*)cursurf + LittleLong(cursurf->ofsVerts));

			for (j = 0; j < surf->numVerts; j++) {
				LL(curv->numWeights);
				if (curv->numWeights < 0 ||
				    (byte*)(v + 1) + (curv->numWeights - 1) * (int)sizeof(*weight) >
				        (byte*)mdr + size) {
					ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has broken structure.\n",
						mod_name);
					return qfalse;
				}

				v->normal[0]    = LittleFloat(curv->normal[0]);
				v->normal[1]    = LittleFloat(curv->normal[1]);
				v->normal[2]    = LittleFloat(curv->normal[2]);
				v->texCoords[0] = LittleFloat(curv->texCoords[0]);
				v->texCoords[1] = LittleFloat(curv->texCoords[1]);
				v->numWeights   = curv->numWeights;

				weight    = &v->weights[0];
				curweight = &curv->weights[0];
				for (k = 0; k < v->numWeights; k++) {
					weight->boneIndex  = LittleLong(curweight->boneIndex);
					weight->boneWeight = LittleFloat(curweight->boneWeight);
					weight->offset[0]  = LittleFloat(curweight->offset[0]);
					weight->offset[1]  = LittleFloat(curweight->offset[1]);
					weight->offset[2]  = LittleFloat(curweight->offset[2]);
					weight++;
					curweight++;
				}

				v    = (mdrVertex_t*)weight;
				curv = (mdrVertex_t*)curweight;
			}

			// Triangles follow vertices
			tri                = (mdrTriangle_t*)v;
			surf->ofsTriangles = (int)((byte*)tri - (byte*)surf);
			curtri             = (mdrTriangle_t*)((byte*)cursurf + LittleLong(cursurf->ofsTriangles));

			if (surf->numTriangles < 0 ||
			    (byte*)(tri + surf->numTriangles) > (byte*)mdr + size) {
				ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}

			for (j = 0; j < surf->numTriangles; j++) {
				tri->indexes[0] = LittleLong(curtri->indexes[0]);
				tri->indexes[1] = LittleLong(curtri->indexes[1]);
				tri->indexes[2] = LittleLong(curtri->indexes[2]);
				tri++;
				curtri++;
			}

			surf->ofsEnd = (int)((byte*)tri - (byte*)surf);
			surf         = (mdrSurface_t*)tri;
			cursurf      = (mdrSurface_t*)((byte*)cursurf + LittleLong(cursurf->ofsEnd));
		}

		lod->ofsEnd = (int)((byte*)surf - (byte*)lod);
		lod         = (mdrLOD_t*)surf;
		curlod      = (mdrLOD_t*)((byte*)curlod + LittleLong(curlod->ofsEnd));
	}

	// Tags follow all LODs
	tag          = (mdrTag_t*)lod;
	mdr->ofsTags = (int)((byte*)tag - (byte*)mdr);
	curtag       = (mdrTag_t*)((byte*)pinmodel + LittleLong(pinmodel->ofsTags));

	if (mdr->numTags < 0 || (byte*)(tag + mdr->numTags) > (byte*)mdr + size) {
		ri.Printf(PRINT_WARNING, "MetalModel_LoadMDR: %s has broken structure.\n", mod_name);
		return qfalse;
	}

	for (i = 0; i < mdr->numTags; i++) {
		tag->boneIndex = LittleLong(curtag->boneIndex);
		Q_strncpyz(tag->name, curtag->name, sizeof(tag->name));
		tag++;
		curtag++;
	}

	mdr->ofsEnd  = (int)((byte*)tag - (byte*)mdr);
	model->type  = MetalModelType::MDR;
	return qtrue;
}

// MetalModel_RegisterMDR: read an MDR file and load it into model.
qhandle_t MetalModel_RegisterMDR(const char* name, MetalModel* model, refimport_t& imports)
{
	union { unsigned* u; void* v; } buf;
	int filesize;

	filesize = imports.FS_ReadFile(name, &buf.v);
	if (!buf.u) {
		imports.Printf(PRINT_DEVELOPER,
			"MetalModel_RegisterMDR: couldn't load %s\n", name);
		return 0;
	}

	int ident = LittleLong(*(unsigned*)buf.u);
	if (ident != MDR_IDENT) {
		imports.Printf(PRINT_WARNING,
			"MetalModel_RegisterMDR: unknown fileid for %s\n", name);
		imports.FS_FreeFile(buf.v);
		return 0;
	}

	qboolean loaded = MetalModel_LoadMDR(model, buf.u, filesize, name);
	imports.FS_FreeFile(buf.v);

	if (!loaded) {
		model->type = MetalModelType::BAD;
		return 0;
	}

	return model->index;
}
