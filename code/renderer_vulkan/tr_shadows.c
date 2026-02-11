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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_globals.h"
#include "vk_shade_geometry.h"
#include "vk_instance.h"
#include "vk_pipelines.h"
#include "vk_image.h"
#include "tr_cvar.h"
#include "tr_backend.h"
/*

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

typedef struct {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES/3];
static vec4_t		extrudedEdges[SHADER_MAX_VERTEXES * 4];
static int			numExtrudedEdges;

static void R_AddEdgeDef( int i1, int i2, int facing ) {
	int		c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;		// overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = facing;

	numEdgeDefs[ i1 ]++;
}

static void R_ExtrudeShadowEdges( void ) {
	int		i;
	int		c, c2;
	int		j, k;
	int		i2;

	numExtrudedEdges = 0;

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		c = numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			qboolean sil_edge = qtrue;
			i2 = edgeDefs[ i ][ j ].i2;
			c2 = numEdgeDefs[ i2 ];
			for ( k = 0 ; k < c2 ; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i && edgeDefs[ i2 ][ k ].facing) {
					sil_edge = qfalse;
					break;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if ( sil_edge ) {
				VectorCopy(tess.xyz[ i ],						extrudedEdges[numExtrudedEdges * 4 + 0]);
				VectorCopy(tess.xyz[ i + tess.numVertexes ],	extrudedEdges[numExtrudedEdges * 4 + 1]);
				VectorCopy(tess.xyz[ i2 ],						extrudedEdges[numExtrudedEdges * 4 + 2]);
				VectorCopy(tess.xyz[ i2 + tess.numVertexes ],	extrudedEdges[numExtrudedEdges * 4 + 3]);
				numExtrudedEdges++;
			}
		}
	}
}



// VULKAN
static void vk_renderShadowEdges(VkPipeline vk_pipeline)
{

	int i = 0;
	while (i < numExtrudedEdges) {
		int count = numExtrudedEdges - i;
		if (count > (SHADER_MAX_VERTEXES - 1) / 4)
			count = (SHADER_MAX_VERTEXES - 1) / 4;

		memcpy(tess.xyz, extrudedEdges[i*4], 4 * count * sizeof(vec4_t));
		tess.numVertexes = count * 4;
        int k = 0;
		
        for (k = 0; k < count; k++)
        {
			tess.indexes[k * 6 + 0] = k * 4 + 0;
			tess.indexes[k * 6 + 1] = k * 4 + 2;
			tess.indexes[k * 6 + 2] = k * 4 + 1;

			tess.indexes[k * 6 + 3] = k * 4 + 2;
			tess.indexes[k * 6 + 4] = k * 4 + 3;
			tess.indexes[k * 6 + 5] = k * 4 + 1;
		}
		tess.numIndexes = count * 6;

		for (k = 0; k < tess.numVertexes; k++)
        {
			VectorSet(tess.svars.colors[k], 50, 50, 50);
			tess.svars.colors[k][3] = 255;
		}
        
        vk_UploadXYZI(tess.xyz, tess.numVertexes, tess.indexes, tess.numIndexes);
        updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, getptr_modelview_matrix());

        vk_shade_geometry(vk_pipeline, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);


		i += count;
	}
}

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd( void ) {
	int		i;
	int		numTris;
	vec3_t	lightDir;

	// we can only do this if we have enough space in the vertex buffers
	if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
		return;
	}


	VectorCopy( backEnd.currentEntity->lightDir, lightDir );

	// project vertexes away from light direction
	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		VectorMA( tess.xyz[i], -512, lightDir, tess.xyz[i+tess.numVertexes] );
	}

	// decide which triangles face the light
	memset( numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ )
    {
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		d = DotProduct( normal, lightDir );
		if ( d > 0 ) {
			facing[ i ] = 1;
		} else {
			facing[ i ] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	// draw the silhouette edges

	updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

	R_ExtrudeShadowEdges();

	// mirrors have the culling order reversed

	// VULKAN
	vk_renderShadowEdges(g_stdPipelines.shadow_volume_pipelines[0][backEnd.viewParms.isMirror]);
	vk_renderShadowEdges(g_stdPipelines.shadow_volume_pipelines[1][backEnd.viewParms.isMirror]);

}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish( void )
{
	if ( r_shadows->integer != 2 ) {
		return;
	}

	updateCurDescriptor( tr.whiteImage->descriptor_set, 0);

	// VULKAN

    tess.indexes[0] = 0;
    tess.indexes[1] = 1;
    tess.indexes[2] = 2;
    tess.indexes[3] = 0;
    tess.indexes[4] = 2;
    tess.indexes[5] = 3;
    tess.numIndexes = 6;

    VectorSet(tess.xyz[0], -100,  100, -10);
    VectorSet(tess.xyz[1],  100,  100, -10);
    VectorSet(tess.xyz[2],  100, -100, -10);
    VectorSet(tess.xyz[3], -100, -100, -10);
    int i = 0;

    for (i = 0; i < 4; i++)
    {
        VectorSet(tess.svars.colors[i], 153, 153, 153);
        tess.svars.colors[i][3] = 255;
    }
    tess.numVertexes = 4;

	//PushModelView();

    // Com_Memcpy(tmp, vk_world.modelview_transform, 64);

    float tmp[16] = { 1, 0 , 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
     
    vk_UploadXYZI(tess.xyz, tess.numVertexes, tess.indexes, tess.numIndexes);
    updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, tmp);
    vk_shade_geometry(g_stdPipelines.shadow_finish_pipeline, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);

    tess.numIndexes = 0;
    tess.numVertexes = 0;
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform( void )
{
	vec3_t	ground;
	vec3_t	light;
	vec3_t	lightDir;

	float* xyz = ( float * ) tess.xyz;

	ground[0] = backEnd.or.axis[0][2];
	ground[1] = backEnd.or.axis[1][2];
	ground[2] = backEnd.or.axis[2][2];

	float groundDist = backEnd.or.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	
    float d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 )
    {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	int	i;
	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 )
    {
		float h = DotProduct( xyz, ground ) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
