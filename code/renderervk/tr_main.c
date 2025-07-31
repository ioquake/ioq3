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
// tr_main.c -- main control flow for each frame

#include "tr_local.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "tr_shader.h"

#include "vk_shade_geometry.h"

#include "vk_image.h"
#include "matrix_multiplication.h"
#include "ref_import.h"

#include "R_PrintMat.h"
#include "R_PortalPlane.h"
#include "R_DebugGraphics.h"


// x: 1x3
// x^T: 3x1
// mat: 3x3
// out: 1*3
// out = x * mat = (mat^T * x^T)^T 
static inline void R_LocalVecToWorld (const float in[3], const float mat[3][3], float out[3])
{
	out[0] = in[0] * mat[0][0] + in[1] * mat[1][0] + in[2] * mat[2][0];
	out[1] = in[0] * mat[0][1] + in[1] * mat[1][1] + in[2] * mat[2][1];
	out[2] = in[0] * mat[0][2] + in[1] * mat[1][2] + in[2] * mat[2][2];
}


/*
static void R_LocalNormalToWorld (const vec3_t local, const orientationr_t * const pRT, vec3_t world)
{
	world[0] = local[0] * pRT->axis[0][0] + local[1] * pRT->axis[1][0] + local[2] * pRT->axis[2][0];
	world[1] = local[0] * pRT->axis[0][1] + local[1] * pRT->axis[1][1] + local[2] * pRT->axis[2][1];
	world[2] = local[0] * pRT->axis[0][2] + local[1] * pRT->axis[1][2] + local[2] * pRT->axis[2][2];
}
*/

// x: 1x3
// x^T: 3x1
// mat: 3x3
// out: 1*3
// out = (x * mat^T) = (mat * x^T)^T
static inline void R_WorldVectorToLocal (const float in[3], const float mat[3][3], float out[3])
{
//	out[0] = DotProduct(in, mat[0]);
//	out[1] = DotProduct(in, mat[1]);
//	out[2] = DotProduct(in, mat[2]);
    out[0] = in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2];
    out[1] = in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2];
    out[2] = in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2];
}


static void R_WorldPointToLocal (const vec3_t world, const orientationr_t * const pRT, float out[3])
{
    float delta[3];
    VectorSubtract( world, pRT->origin, delta );
    R_WorldVectorToLocal(delta, pRT->axis, out);
}

/*
static void R_MirrorVector (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int		i;

	VectorClear( out );
	for ( i = 0 ; i < 3 ; i++ )
    {
		float d = DotProduct(in, surface->axis[i]);
		VectorMA( out, d, camera->axis[i], out );
	}
}
*/

static inline void R_MirrorVector (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
    vec3_t local;
    R_WorldVectorToLocal(in, surface->axis, local);
    R_LocalVecToWorld(local, camera->axis, out);
}

static void R_MirrorPoint (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
    // ri.Printf(PRINT_ALL, "R_MirrorPoint\n");
	vec3_t	vectmp;
	VectorSubtract( in, surface->origin, vectmp );
    
	// vec3_t transformed;
    vec3_t local;
    R_WorldVectorToLocal(vectmp, surface->axis, local);
    R_LocalVecToWorld(local, camera->axis, vectmp);
	VectorAdd( vectmp, camera->origin, out );
}



/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end

typedef struct {
	float		modelMatrix[16] QALIGN(16);
	float		axis[3][3];		// orientation in world
    float		origin[3];		// in world coordinates
	float		viewOrigin[3];	// viewParms->or.origin in local coordinates
} orientationr_t;

=================
*/
void R_RotateForEntity(const trRefEntity_t* const ent, const viewParms_t* const viewParms, orientationr_t* const or)
{

	if ( ent->e.reType != RT_MODEL )
    {
		*or = viewParms->world;
		return;
	}

	//VectorCopy( ent->e.origin, or->origin );
	//VectorCopy( ent->e.axis[0], or->axis[0] );
	//VectorCopy( ent->e.axis[1], or->axis[1] );
	//VectorCopy( ent->e.axis[2], or->axis[2] );
    memcpy(or->origin, ent->e.origin, 12);
    memcpy(or->axis, ent->e.axis, 36);

	float glMatrix[16] QALIGN(16);

	glMatrix[0] = or->axis[0][0];
	glMatrix[1] = or->axis[0][1];
	glMatrix[2] = or->axis[0][2];
	glMatrix[3] = 0;

    glMatrix[4] = or->axis[1][0];
	glMatrix[5] = or->axis[1][1];
	glMatrix[6] = or->axis[1][2];
	glMatrix[7] = 0;
    
    glMatrix[8] = or->axis[2][0];
	glMatrix[9] = or->axis[2][1];
	glMatrix[10] = or->axis[2][2];
	glMatrix[11] = 0;

	glMatrix[12] = or->origin[0];
	glMatrix[13] = or->origin[1];
	glMatrix[14] = or->origin[2];
	glMatrix[15] = 1;

	MatrixMultiply4x4_SSE( glMatrix, viewParms->world.modelMatrix, or->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
    
    R_WorldPointToLocal(viewParms->or.origin, or, or->viewOrigin);
    
    if ( ent->e.nonNormalizedAxes )
    {
         if ( ent->e.nonNormalizedAxes )
        {
            const float * v = ent->e.axis[0];
            float axisLength = v[0] * v[0] + v[0] * v[0] + v[2] * v[2]; 
            if ( axisLength ) {
                axisLength = 1.0f / sqrtf(axisLength);
            }

            or->viewOrigin[0] *= axisLength;
            or->viewOrigin[1] *= axisLength;
            or->viewOrigin[2] *= axisLength;
        }
    }
/*  
    vec3_t delta;

	VectorSubtract( viewParms->or.origin, or->origin, delta );

    R_WorldVectorToLocal(delta, or->axis, or->viewOrigin);
    
	// compensate for scale in the axes if necessary
    float axisLength = 1.0f;
	if ( ent->e.nonNormalizedAxes )
    {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( axisLength ) {
			axisLength = 1.0f / axisLength;
		}
	}

	or->viewOrigin[0] = DotProduct( delta, or->axis[0] ) * axisLength;
	or->viewOrigin[1] = DotProduct( delta, or->axis[1] ) * axisLength;
	or->viewOrigin[2] = DotProduct( delta, or->axis[2] ) * axisLength;

*/
    // printMat1x3f("viewOrigin", or->viewOrigin);
    // printMat4x4f("modelMatrix", or->modelMatrix);

}

/*
=================
typedef struct {
	float		modelMatrix[16] QALIGN(16);
	float		axis[3][3];		// orientation in world
    float		origin[3];			// in world coordinates
	float		viewOrigin[3];		// viewParms->or.origin in local coordinates
} orientationr_t;


Sets up the modelview matrix for a given viewParm

IN: tr.viewParms
OUT: tr.or
=================
*/
static void R_RotateForViewer ( viewParms_t * const pViewParams, orientationr_t * const pEntityPose) 
{
    //const viewParms_t * const pViewParams = &tr.viewParms;
    // for current entity
    // orientationr_t * const pEntityPose = &tr.or;
    
    const static float s_flipMatrix[16] QALIGN(16) = {
        // convert from our coordinate system (looking down X)
        // to OpenGL's coordinate system (looking down -Z)
        0, 0, -1, 0,
        -1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 0, 1
    };
    

	float o0, o1, o2;

    pEntityPose->origin[0] = pEntityPose->origin[1] = pEntityPose->origin[2] = 0;
    Mat3x3Identity(pEntityPose->axis);


    // transform by the camera placement
	// VectorCopy( tr.viewParms.or.origin, tr.or.viewOrigin );
	// VectorCopy( tr.viewParms.or.origin, origin );

    pEntityPose->viewOrigin[0] = o0 = pViewParams->or.origin[0];
    pEntityPose->viewOrigin[1] = o1 = pViewParams->or.origin[1];
    pEntityPose->viewOrigin[2] = o2 = pViewParams->or.origin[2];

    
    float viewerMatrix[16] QALIGN(16);
	viewerMatrix[0] = pViewParams->or.axis[0][0];
	viewerMatrix[1] = pViewParams->or.axis[1][0];
	viewerMatrix[2] = pViewParams->or.axis[2][0];
	viewerMatrix[3] = 0;

	viewerMatrix[4] = pViewParams->or.axis[0][1];
	viewerMatrix[5] = pViewParams->or.axis[1][1];
	viewerMatrix[6] = pViewParams->or.axis[2][1];
	viewerMatrix[7] = 0;

	viewerMatrix[8] = pViewParams->or.axis[0][2];
	viewerMatrix[9] = pViewParams->or.axis[1][2];
	viewerMatrix[10] = pViewParams->or.axis[2][2];
	viewerMatrix[11] = 0;

	viewerMatrix[12] = - o0 * viewerMatrix[0] - o1 * viewerMatrix[4] - o2 * viewerMatrix[8];
	viewerMatrix[13] = - o0 * viewerMatrix[1] - o1 * viewerMatrix[5] - o2 * viewerMatrix[9];
	viewerMatrix[14] = - o0 * viewerMatrix[2] - o1 * viewerMatrix[6] - o2 * viewerMatrix[10];
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	MatrixMultiply4x4_SSE( viewerMatrix, s_flipMatrix, pEntityPose->modelMatrix );

	pViewParams->world = *pEntityPose;
}


/*
=================
Setup that culling frustum planes for the current view
=================
*/
static void R_SetupFrustum (viewParms_t * const pViewParams)
{
	
    {
        float ang = pViewParams->fovX * (float)(M_PI / 360.0f);
        float xs = sin( ang );
        float xc = cos( ang );

        float temp1[3];
        float temp2[3];

        VectorScale( pViewParams->or.axis[0], xs, temp1 );
        VectorScale( pViewParams->or.axis[1], xc, temp2);

        VectorAdd(temp1, temp2, pViewParams->frustum[0].normal);
		pViewParams->frustum[0].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[0].normal);
        pViewParams->frustum[0].type = PLANE_NON_AXIAL;

        VectorSubtract(temp1, temp2, pViewParams->frustum[1].normal);
		pViewParams->frustum[1].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[1].normal);
        pViewParams->frustum[1].type = PLANE_NON_AXIAL;
    }

   
    {
        float ang = pViewParams->fovY * (float)(M_PI / 360.0f);
        float xs = sin( ang );
        float xc = cos( ang );
        float temp1[3];
        float temp2[3];

        VectorScale( pViewParams->or.axis[0], xs, temp1);
        VectorScale( pViewParams->or.axis[2], xc, temp2);

        VectorAdd(temp1, temp2, pViewParams->frustum[2].normal);
		pViewParams->frustum[2].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[2].normal);
        pViewParams->frustum[2].type = PLANE_NON_AXIAL;

        VectorSubtract(temp1, temp2, pViewParams->frustum[3].normal);
		pViewParams->frustum[3].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[3].normal);
		pViewParams->frustum[3].type = PLANE_NON_AXIAL;
    }


    uint32_t i = 0;
	for (i=0; i < 4; i++)
    {
		// SetPlaneSignbits( &pViewParams->frustum[i] );
        // cplane_t* out = &pViewParams->frustum[i];
        int	bits = 0, j;

        // for fast box on planeside test

        for (j=0 ; j<3 ; j++)
        {
            if (pViewParams->frustum[i].normal[j] < 0) {
                bits |= 1<<j;
            }
        }

        pViewParams->frustum[i].signbits = bits;
	}
}


/*
=============
R_PlaneForSurface
=============
*/
void R_PlaneForSurface (surfaceType_t *surfType, cplane_t *plane)
{
	srfTriangles_t	*tri;
	srfPoly_t		*poly;
	drawVert_t		*v1, *v2, *v3;
	vec4_t			plane4;

	if (!surfType) {
		memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch (*surfType)
    {
	case SF_FACE:
		*plane = ((srfSurfaceFace_t *)surfType)->plane;
		return;
	case SF_TRIANGLES:
		tri = (srfTriangles_t *)surfType;
		v1 = tri->verts + tri->indexes[0];
		v2 = tri->verts + tri->indexes[1];
		v3 = tri->verts + tri->indexes[2];
		PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	case SF_POLY:
		poly = (srfPoly_t *)surfType;
		PlaneFromPoints( plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	default:
		memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;		
		return;
	}
}

/*
=================
entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns qtrue if it should be mirrored
=================
*/
static qboolean R_GetPortalOrientations( drawSurf_t *drawSurf, int entityNum, 
							 orientation_t *surface, orientation_t *camera,
							 vec3_t pvsOrigin, qboolean *mirror )
{
	int			i;
	cplane_t	originalPlane, plane;
	vec3_t		transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		// R_LocalNormalToWorld( originalPlane.normal, &tr.or, plane.normal );
        R_LocalVecToWorld(originalPlane.normal, tr.or.axis, plane.normal);
        plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.or.origin );
	} else {
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[0] );
	//VectorPerp( plane.normal, surface->axis[1] );
	PerpendicularVector( surface->axis[1], surface->axis[0] );
    CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ )
    {
		trRefEntity_t* e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		float d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] && 
			e->e.oldorigin[1] == e->e.origin[1] && 
			e->e.oldorigin[2] == e->e.origin[2] ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
			VectorCopy( surface->axis[1], camera->axis[1] );
			VectorCopy( surface->axis[2], camera->axis[2] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );
			
		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		memcpy(camera->axis, e->e.axis, 36);
		VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
		VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

		// optionally rotate
		if ( e->e.oldframe ) {
			// if a speed is specified
			if ( e->e.frame ) {
				// continuous rotate
				d = (tr.refdef.rd.time/1000.0f) * e->e.frame;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.rd.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			}
		}
		else if ( e->e.skinNum ) {
			d = e->e.skinNum;
			VectorCopy( camera->axis[1], transformed );
			RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}



static qboolean IsMirror( const drawSurf_t *drawSurf, int entityNum )
{
	int			i;
	cplane_t	originalPlane, plane;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD ) 
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		// R_LocalNormalToWorld( originalPlane.normal, &tr.or, plane.normal );
        R_LocalVecToWorld(originalPlane.normal, tr.or.axis, plane.normal);
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.or.origin );
	} 
	else 
	{
		plane = originalPlane;
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) 
	{
		trRefEntity_t* e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		float d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] && 
			e->e.oldorigin[1] == e->e.origin[1] && 
			e->e.oldorigin[2] == e->e.origin[2] ) 
		{
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}


/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf, vec4_t clipDest[128] )
{
	float shortest = 100000000;
	int entityNum;
	int numTriangles;
	shader_t *shader;
	int		fogNum;
	int dlighted;
	vec4_t clip;
	int i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	R_RotateForViewer(&tr.viewParms, &tr.or);

	R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );
	RB_BeginSurface( shader, fogNum );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	assert( tess.numVertexes < 128 );

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		int j;
		unsigned int pointFlags = 0;

        TransformModelToClip_SSE(tess.xyz[i], tr.or.modelMatrix, tr.viewParms.projectionMatrix, clip);
		for ( j = 0; j < 3; j++ )
		{
			if ( clip[j] >= clip[3] )
			{
				pointFlags |= (1 << (j*2));
			}
			else if ( clip[j] <= -clip[3] )
			{
				pointFlags |= ( 1 << (j*2+1));
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd )
	{
        tess.numIndexes = 0;
		return qtrue;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for ( i = 0; i < tess.numIndexes; i += 3 )
	{
		vec3_t normal;
		float len;

		VectorSubtract( tess.xyz[tess.indexes[i]], tr.viewParms.or.origin, normal );

		len = VectorLengthSquared( normal );			// lose the sqrt
		if ( len < shortest )
		{
			shortest = len;
		}

		if ( DotProduct( normal, tess.normal[tess.indexes[i]] ) >= 0 )
		{
			numTriangles--;
		}
	}
    tess.numIndexes = 0;
	if ( !numTriangles )
	{
		return qtrue;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( drawSurf, entityNum ) )
	{
		return qfalse;
	}

	if ( shortest > (tess.shader->portalRange*tess.shader->portalRange) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
========================
R_MirrorViewBySurface

Returns qtrue if another view has been rendered
========================
*/
static qboolean R_MirrorViewBySurface (drawSurf_t *drawSurf, int entityNum)
{
	vec4_t			clipDest[128];
	orientation_t	surface, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal) {
		ri.Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if ( r_noportals->integer) {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) ) {
        //ri.Printf(PRINT_ALL, "isSurfOffscreen: 1\n");
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	viewParms_t oldParms = tr.viewParms;
	
    viewParms_t newParms = tr.viewParms;
    newParms.isPortal = qtrue;
    
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera, 
		newParms.pvsOrigin, &newParms.isMirror ) )
    {
		return qfalse;		// bad portal, no portalentity
	}

	R_MirrorPoint (oldParms.or.origin, &surface, &camera, newParms.or.origin );

	// VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	// newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );
    R_SetupPortalPlane(camera.axis, camera.origin);

	R_MirrorVector (oldParms.or.axis[0], &surface, &camera, newParms.or.axis[0]);
	R_MirrorVector (oldParms.or.axis[1], &surface, &camera, newParms.or.axis[1]);
	R_MirrorVector (oldParms.or.axis[2], &surface, &camera, newParms.or.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view
	// render the mirror view
	R_RenderView (&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;

	if ( tr.refdef.rd.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
=================
qsort replacement

=================
*/
void SWAP_DRAW_SURF(void* a, void* b)
{
    char buf[sizeof(drawSurf_t)];
    memcpy(buf, a, sizeof(drawSurf_t));
    memcpy(a, b, sizeof(drawSurf_t));
    memcpy(b, buf, sizeof(drawSurf_t));
}


/* this parameter defines the cutoff between using quick sort and
   insertion sort for arrays; arrays with lengths shorter or equal to the
   below value use insertion sort */

#define CUTOFF 8            /* testing shows that this is good value */

static void shortsort( drawSurf_t *lo, drawSurf_t *hi )
{
    drawSurf_t	*p, *max;

    while (hi > lo) {
        max = lo;
        for (p = lo + 1; p <= hi; p++ ) {
            if ( p->sort > max->sort ) {
                max = p;
            }
        }
        SWAP_DRAW_SURF(max, hi);
        hi--;
    }
}


/* sort the array between lo and hi (inclusive)
FIXME: this was lifted and modified from the microsoft lib source...
 */

void qsortFast (
    void *base,
    unsigned num,
    unsigned width
    )
{
    char *lo, *hi;              /* ends of sub-array currently sorting */
    char *mid;                  /* points to middle of subarray */
    char *loguy, *higuy;        /* traveling pointers for partition step */
    unsigned size;              /* size of the sub-array */
    char *lostk[30], *histk[30];
    int stkptr;                 /* stack for saving sub-array to be processed */

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2 || width == 0)
        return;                 /* nothing to do */

    stkptr = 0;                 /* initialize stack */

    lo = (char*) base;
    hi = (char *)base + width * (num-1);        /* initialize limits */

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    size = (hi - lo) / width + 1;        /* number of el's to sort */

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= CUTOFF) {
         shortsort((drawSurf_t *)lo, (drawSurf_t *)hi);
    }
    else {
        /* First we pick a partititioning element.  The efficiency of the
           algorithm demands that we find one that is approximately the
           median of the values, but also that we select one fast.  Using
           the first one produces bad performace if the array is already
           sorted, so we use the middle one, which would require a very
           wierdly arranged array for worst case performance.  Testing shows
           that a median-of-three algorithm does not, in general, increase
           performance. */

        mid = lo + (size / 2) * width;      /* find middle element */
        SWAP_DRAW_SURF(mid, lo);               /* swap it to beginning of array */

        /* We now wish to partition the array into three pieces, one
           consisiting of elements <= partition element, one of elements
           equal to the parition element, and one of element >= to it.  This
           is done below; comments indicate conditions established at every
           step. */

        loguy = lo;
        higuy = hi + width;

        /* Note that higuy decreases and loguy increases on every iteration,
           so loop must terminate. */
        for (;;) {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do  {
                loguy += width;
            } while (loguy <= hi &&  
				( ((drawSurf_t *)loguy)->sort <= ((drawSurf_t *)lo)->sort ) );

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do  {
                higuy -= width;
            } while (higuy > lo && 
				( ((drawSurf_t *)higuy)->sort >= ((drawSurf_t *)lo)->sort ) );

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            SWAP_DRAW_SURF(loguy, higuy);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        SWAP_DRAW_SURF(lo, higuy);     /* put partition element in place */

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + width < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo + width < higuy) {
                hi = higuy - width;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
       Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
    else
        return;                 /* all subarrays done */
}


//==========================================================================================
/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap )
{
	// instead of checking for overflow, we just mask the index so it wraps around
	int index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT) 
		| tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap ) {
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_ENTITYNUM_SHIFT ) & 1023;
	*dlightMap = sort & 3;
}


static void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
        ri.Printf(PRINT_WARNING, " numDrawSurfs overflowed. \n");

	}

	// sort the drawsurfs by sort type, then orientation, then shader
	qsortFast (drawSurfs, numDrawSurfs, sizeof(drawSurf_t) );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0 ; i < numDrawSurfs ; i++ )
    {
		R_DecomposeSort( (drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			ri.Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( (drawSurfs+i), entityNum) ) {
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}



void R_AddEntitySurfaces (viewParms_t * const pViewParam)
{
    // entities that will have procedurally generated surfaces will just
    // point at this for their sorting surface
    static surfaceType_t	entitySurface = SF_ENTITY;
	if ( !r_drawentities->integer ) {
		return;
	}

	for ( tr.currentEntityNum = 0; 
	      tr.currentEntityNum < tr.refdef.num_entities; 
		  tr.currentEntityNum++ )
    {
		shader_t* shader;

        trRefEntity_t* ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = qfalse;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in 
		// mirrors, because the true body position will already be drawn
		//
		if ( (ent->e.renderfx & RF_FIRST_PERSON) && pViewParam->isPortal)
        {
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType )
        {
		case RT_PORTALSURFACE:
			break;		// don't draw anything
		case RT_SPRITE:
		case RT_BEAM:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_RINGS:
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ( (ent->e.renderfx & RF_THIRD_PERSON) && !pViewParam->isPortal)
            {
				continue;
			}
			shader = R_GetShaderByHandle( ent->e.customShader );
			R_AddDrawSurf( &entitySurface, shader, R_SpriteFogNum( ent ), 0 );
			break;

		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity( ent, pViewParam, &tr.or );

			tr.currentModel = R_GetModelByHandle( ent->e.hModel );
			if (!tr.currentModel)
            {
				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
			}
            else
            {
				switch ( tr.currentModel->type )
                {
				case MOD_MESH:
					R_AddMD3Surfaces( ent );
					break;
				case MOD_MDR:
					R_MDRAddAnimSurfaces( ent );
					break;
				case MOD_IQM:
					R_AddIQMSurfaces( ent );
				case MOD_BRUSH:
					R_AddBrushModelSurfaces( ent );
					break;
				case MOD_BAD:		// null model axis
					if ( (ent->e.renderfx & RF_THIRD_PERSON) && !pViewParam->isPortal) {
						break;
					}
					shader = R_GetShaderByHandle( ent->e.customShader );
					R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
					break;
				default:
					ri.Error( ERR_DROP, "Add entity surfaces: Bad modeltype" );
					break;
				}
			}
			break;
		default:
			ri.Error( ERR_DROP, "Add entity surfaces: Bad reType" );
		}
	}
}


static void R_SetupProjection( viewParms_t * const pViewParams)
{
	float zFar;

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are added, 
    // because they use the projection matrix for lod calculation

	// dynamically compute far clip plane distance
	// if not rendering the world (icons, menus, etc), set a 2k far clip plane

	if ( tr.refdef.rd.rdflags & RDF_NOWORLDMODEL )
    {
		pViewParams->zFar = zFar = 2048.0f;
	}
    else
    {
        float o[3];

        o[0] = pViewParams->or.origin[0];
        o[1] = pViewParams->or.origin[1];
        o[2] = pViewParams->or.origin[2];

        float farthestCornerDistance = 0;
        uint32_t i;

        // set far clipping planes dynamically
        for ( i = 0; i < 8; i++ )
        {
            float v[3];
     
            v[0] = ((i & 1) ? pViewParams->visBounds[0][0] : pViewParams->visBounds[1][0]) - o[0];
            v[1] = ((i & 2) ? pViewParams->visBounds[0][1] : pViewParams->visBounds[1][1]) - o[1];
            v[2] = ((i & 4) ? pViewParams->visBounds[0][2] : pViewParams->visBounds[1][2]) - o[0];

            float distance = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
            
            if( distance > farthestCornerDistance )
            {
                farthestCornerDistance = distance;
            }
        }
        
        pViewParams->zFar = zFar = sqrtf(farthestCornerDistance);
    }
	
	// set up projection matrix
	// update q3's proj matrix (opengl) to vulkan conventions: z - [0, 1] instead of [-1, 1] and invert y direction
    
    // Vulkan clip space has inverted Y and half Z.	
    float zNear	= r_znear->value;
	float p10 = -zFar / (zFar - zNear);

    float py = tan(pViewParams->fovY * (M_PI / 360.0f));
    float px = tan(pViewParams->fovX * (M_PI / 360.0f));

	pViewParams->projectionMatrix[0] = 1.0f / px;
	pViewParams->projectionMatrix[1] = 0;
	pViewParams->projectionMatrix[2] = 0;
	pViewParams->projectionMatrix[3] = 0;
    
    pViewParams->projectionMatrix[4] = 0;
	pViewParams->projectionMatrix[5] = -1.0f / py;
	pViewParams->projectionMatrix[6] = 0;
	pViewParams->projectionMatrix[7] = 0;

    pViewParams->projectionMatrix[8] = 0;	// normally 0
	pViewParams->projectionMatrix[9] =  0;
	pViewParams->projectionMatrix[10] = p10;
	pViewParams->projectionMatrix[11] = -1.0f;

    pViewParams->projectionMatrix[12] = 0;
	pViewParams->projectionMatrix[13] = 0;
	pViewParams->projectionMatrix[14] = zNear * p10;
	pViewParams->projectionMatrix[15] = 0;
}



/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView (viewParms_t *parms)
{
	int	firstDrawSurf;

	tr.viewCount++;

	tr.viewParms = *parms;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer (&tr.viewParms, &tr.or);
    // Setup that culling frustum planes for the current view
	R_SetupFrustum (&tr.viewParms);

	R_AddWorldSurfaces ();

	R_AddPolygonSurfaces();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection matrix for LOD calculation
	R_SetupProjection (&tr.viewParms);

	R_AddEntitySurfaces (&tr.viewParms);

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

    if ( r_debugSurface->integer )
    {
        // draw main system development information (surface outlines, etc)
		R_DebugGraphics();
	}
}
