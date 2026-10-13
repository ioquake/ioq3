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
// tr_mesh.c: triangle model functions

#include "tr_local.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "vk_shade_geometry.h"

#include "ref_import.h"
#include "tr_light.h"


static int R_CullModel( md3Header_t *header, trRefEntity_t *ent )
{
	vec3_t		bounds[2];
	int			i;

	// compute frame pointers
	md3Frame_t* newFrame = ( md3Frame_t * ) ( ( byte * ) header + header->ofsFrames ) + ent->e.frame;
	md3Frame_t* oldFrame = ( md3Frame_t * ) ( ( byte * ) header + header->ofsFrames ) + ent->e.oldframe;

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		if ( ent->e.frame == ent->e.oldframe )
		{
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
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
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
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
	for (i = 0 ; i < 3 ; i++) {
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



int R_ComputeLOD( trRefEntity_t *ent )
{
 
    int lod = 0;

    float radius;
    // radius are guarentee large than 0;

    // multiple LODs exist, so compute projected bounding sphere
    // and use that as a criteria for selecting LOD
    if(tr.currentModel->type == MOD_MDR)
    {
        mdrHeader_t * mdr = (mdrHeader_t *) tr.currentModel->modelData;
        int frameSize = (size_t) (&((mdrFrame_t *)0)->bones[mdr->numBones]);

        mdrFrame_t * mdrframe = (mdrFrame_t *) ((byte *) mdr + mdr->ofsFrames + frameSize * ent->e.frame);

        radius = RadiusFromBounds(mdrframe->bounds[0], mdrframe->bounds[1]);
    }
    else
    {
        md3Frame_t * frame = ( md3Frame_t * ) ( ( ( unsigned char * ) tr.currentModel->md3[0] ) + tr.currentModel->md3[0]->ofsFrames );

        frame += ent->e.frame;

        radius = RadiusFromBounds( frame->bounds[0], frame->bounds[1] );
    }

    float tmpVec[3];
    VectorSubtract(ent->e.origin, tr.viewParms.or.origin, tmpVec);
    float dist = DotProduct( tr.viewParms.or.axis[0], tmpVec);
    if ( dist > 0 )
    {

        // vec3_t	p;
        // p[0] = 0;
        // p[1] = r ;
        // p[2] = -dist;
        // p[3] = 1;

        //  pMatProj = tr.viewParms.projectionMatrix
        //  float projected[4];
        //	projected[0] = p[0] * pMatProj[0] + p[1] * pMatProj[4] + p[2] * pMatProj[8] + pMatProj[12];
        //  projected[1] = p[0] * pMatProj[1] - p[1] * pMatProj[5] + p[2] * pMatProj[9] + pMatProj[13];
        //	projected[2] = p[0] * pMatProj[2] + p[1] * pMatProj[6] + p[2] * pMatProj[10] + pMatProj[14];
        //  projected[3] = p[0] * pMatProj[3] + p[1] * pMatProj[7] + p[2] * pMatProj[11] + pMatProj[15];
        //  perspective devide
        //  pr = projected[1] / projected[3];

        float p1 = - radius * tr.viewParms.projectionMatrix[5] - dist * tr.viewParms.projectionMatrix[9] + tr.viewParms.projectionMatrix[13];
        float p3 =   radius * tr.viewParms.projectionMatrix[7] - dist * tr.viewParms.projectionMatrix[11] + tr.viewParms.projectionMatrix[15];

        float projectedRadius = p1 / p3;

	    //ri.Printf( PRINT_ALL, "%f: \n", projectedRadius);
        
        lod = (1.0f - projectedRadius * 6 ) * tr.currentModel->numLods;

         
        if ( lod < 0 )
        {
            lod = 0;
        }
        else if ( lod >= tr.currentModel->numLods )
        {
            lod = tr.currentModel->numLods - 1;
        }

    }


	return lod;
}

/*
=================
R_ComputeFogNum

=================
*/
int R_ComputeFogNum( md3Header_t *header, trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;
	md3Frame_t		*md3Frame;
	vec3_t			localOrigin;

	if ( tr.refdef.rd.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	// FIXME: non-normalized axis issues
	md3Frame = ( md3Frame_t * ) ( ( byte * ) header + header->ofsFrames ) + ent->e.frame;
	VectorAdd( ent->e.origin, md3Frame->localOrigin, localOrigin );
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( localOrigin[j] - md3Frame->radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( localOrigin[j] + md3Frame->radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}



void R_AddMD3Surfaces( trRefEntity_t *ent )
{
	int				i;
	md3Header_t		*header = NULL;
	md3Surface_t	*surface = NULL;
	md3Shader_t		*md3Shader = NULL;
	shader_t		*shader = NULL;
	int				cull;
	int				lod = 0;
	int				fogNum;

	// don't add third_person objects if not in a portal
	qboolean personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->md3[0]->numFrames;
		ent->e.oldframe %= tr.currentModel->md3[0]->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	

	if ( (ent->e.frame >= tr.currentModel->md3[0]->numFrames) 
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= tr.currentModel->md3[0]->numFrames)
		|| (ent->e.oldframe < 0) )
	{
			ri.Printf( PRINT_ALL, "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
				ent->e.oldframe, ent->e.frame,
				tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	//
	// compute LOD
	// model has only 1 LOD level, skip computations and bias
    if ( tr.currentModel->numLods > 1 )
	    lod = R_ComputeLOD( ent );

	header = tr.currentModel->md3[lod];

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel ( header, ent );
	if ( cull == CULL_OUT ) {
		return;
	}

    
	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// draw all surfaces
	//
	surface = (md3Surface_t *)( (byte *)header + header->ofsSurfaces );
	for ( i = 0 ; i < header->numSurfaces ; i++ )
    {
		if ( ent->e.customShader )
        {
			shader = R_GetShaderByHandle( ent->e.customShader );
		}
        else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
        {
			skin_t *skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
            
            int		j;

			for ( j = 0 ; j < skin->numSurfaces ; j++ )
            {
				// the names have both been lowercased
				if ( !strcmp( skin->pSurfaces[j].name, surface->name ) ) {
					shader = skin->pSurfaces[j].shader;
					break;
				}
			}
			if (shader == tr.defaultShader) {
				ri.Printf( PRINT_DEVELOPER, "no shader for surface %s in skin %s\n", surface->name, skin->name);
			}
			else if (shader->defaultShader) {
				ri.Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
			}
		} else if ( surface->numShaders <= 0 ) {
			shader = tr.defaultShader;
		} else {
			md3Shader = (md3Shader_t *) ( (byte *)surface + surface->ofsShaders );
			md3Shader += ent->e.skinNum % surface->numShaders;
			shader = tr.shaders[ md3Shader->shaderIndex ];
		}


		// we will add shadows even if the main object isn't visible in the view

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel )
        {
	        // see if we are in a fog volume
	        fogNum = R_ComputeFogNum( header, ent );
			R_AddDrawSurf( (void *)surface, shader, fogNum, qfalse );
		}

		surface = (md3Surface_t *)( (byte *)surface + surface->ofsEnd );
	}
}

