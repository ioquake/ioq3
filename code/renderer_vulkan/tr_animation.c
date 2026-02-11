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

#include "tr_local.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "vk_shade_geometry.h"
#include "ref_import.h"
#include "tr_light.h"
#include "tr_shader.h"
/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the 
orientation of the bone in the base frame to the orientation in this
frame.

*/


extern int R_ComputeLOD( trRefEntity_t *ent );

static int R_MDRCullModel( mdrHeader_t *header, trRefEntity_t *ent )
{
	vec3_t bounds[2];
	int	i;

	int frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );
	
	// compute frame pointers
	mdrFrame_t* newFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.frame);
	mdrFrame_t* oldFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.oldframe);

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		if ( ent->e.frame == ent->e.oldframe )
		{
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
				// Ummm... yeah yeah I know we don't really have an md3 here.. but we pretend
				// we do. After all, the purpose of mdrs are not that different, are they?
				
				case CULL_OUT:
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;

				case CULL_IN:
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;

				case CULL_CLIP:
					tr.pc.c_sphere_cull_md3_clip++;
					break;
			}
		}
		else
		{
			int sphereCullB;

			int sphereCull = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame )
			{
				sphereCullB = sphereCull;
			}
			else
			{
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB )
			{
				if ( sphereCull == CULL_OUT )
				{
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				}
				else if ( sphereCull == CULL_IN )
				{
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				}
				else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}
	
	// calculate a bounding box in the current coordinate system
	for (i = 0 ; i < 3 ; i++)
    {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
		case CULL_IN:
			tr.pc.c_box_cull_md3_in++;
			return CULL_IN;
		case CULL_CLIP:
			tr.pc.c_box_cull_md3_clip++;
			return CULL_CLIP;
		case CULL_OUT:
		default:
			tr.pc.c_box_cull_md3_out++;
			return CULL_OUT;
	}
}

static int R_MDRComputeFogNum( mdrHeader_t *header, trRefEntity_t *ent )
{
	vec3_t localOrigin;

	if ( tr.refdef.rd.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}
	
	int frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	// FIXME: non-normalized axis issues
	mdrFrame_t* mdrFrame = ( mdrFrame_t * ) ( ( byte * ) header + header->ofsFrames + frameSize * ent->e.frame);
	VectorAdd( ent->e.origin, mdrFrame->localOrigin, localOrigin );

    int	i, j;
	for ( i = 1 ; i < tr.world->numfogs ; i++ )
    {
		fog_t* fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ )
        {
			if ( localOrigin[j] - mdrFrame->radius >= fog->bounds[1][j] )
				break;
		
			if ( localOrigin[j] + mdrFrame->radius <= fog->bounds[0][j] )
				break;

		}

		if ( j == 3 )
			return i;
	}

	return 0;
}

// much stuff in there is just copied from R_AddMd3Surfaces in tr_mesh.c

void R_MDRAddAnimSurfaces( trRefEntity_t *ent )
{
	shader_t* shader;
	int	i, j;

	mdrHeader_t* header = (mdrHeader_t *) tr.currentModel->modelData;
	qboolean personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;
	
	if( ent->e.renderfx & RF_WRAP_FRAMES )
	{
		ent->e.frame %= header->numFrames;
		ent->e.oldframe %= header->numFrames;
	}
	
	
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, 
    // so when the surfaces are rendered,
    // they don't need to be range checked again.

	if ((ent->e.frame >= header->numFrames) || (ent->e.frame < 0) ||
		(ent->e.oldframe >= header->numFrames) || (ent->e.oldframe < 0) )
	{
		ri.Printf( PRINT_ALL, "R_MDRAddAnimSurfaces: no such frame %d to %d for '%s'\n", ent->e.oldframe, ent->e.frame, tr.currentModel->name );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	int cull = R_MDRCullModel(header, ent);
	if ( cull == CULL_OUT ) {
		return;
	}	

	// figure out the current LOD of the model we're rendering, and set the lod pointer respectively.
    int lodnum = 0;

    if ( tr.currentModel->numLods > 1 )
	    lodnum = R_ComputeLOD( ent );

	// check whether this model has as that many LODs at all. If not, try the closest thing we got.
	if(header->numLODs <= 0)
		return;
	if(header->numLODs <= lodnum)
		lodnum = header->numLODs - 1;

	mdrLOD_t* lod = (mdrLOD_t *)( (unsigned char *)header + header->ofsLODs);
	for(i = 0; i < lodnum; i++)
	{
		lod = (mdrLOD_t *) ((unsigned char *)lod + lod->ofsEnd);
	}
	
	// set up lighting
	if ( !personalModel || r_shadows->integer > 1 )
	{
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	// fogNum?
	int fogNum = R_MDRComputeFogNum( header, ent );

	mdrSurface_t* surface = (mdrSurface_t *)( (unsigned char *)lod + lod->ofsSurfaces );

	for ( i = 0 ; i < lod->numSurfaces ; i++ )
	{
		if(ent->e.customShader)
			shader = R_GetShaderByHandle(ent->e.customShader);
		else if((ent->e.customSkin > 0) && (ent->e.customSkin < tr.numSkins))
		{
			skin_t* skin = tr.skins[ent->e.customSkin];
			shader = tr.defaultShader;
			
			for(j = 0; j < skin->numSurfaces; j++)
			{
                if (0 == strcmp(skin->pSurfaces[j].name, surface->name))
				{
					shader = skin->pSurfaces[j].shader;
					break;
				}
			}
		}
		else if(surface->shaderIndex > 0)
			shader = R_GetShaderByHandle( surface->shaderIndex );
		else
			shader = tr.defaultShader;

		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( (personalModel == 0) && (r_shadows->integer == 2) && (fogNum == 0)
			&& !(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			&& (shader->sort == SS_OPAQUE) )
		{
			R_AddDrawSurf( (void *)surface, tr.shadowShader, 0, qfalse );
		}

		// projection shadows work fine with personal models
		if ( (r_shadows->integer == 3) && (fogNum == 0)
			&& (ent->e.renderfx & RF_SHADOW_PLANE ) && (shader->sort == SS_OPAQUE) )
		{
			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, qfalse );
		}

		if (!personalModel)
			R_AddDrawSurf( (void *)surface, shader, fogNum, qfalse );

		surface = (mdrSurface_t *)( (byte *)surface + surface->ofsEnd );
	}
}




