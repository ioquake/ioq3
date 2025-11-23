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


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"

#include "../renderercommon/tr_types.h"

#include "tr_image.h"

#include "VKimpl.h"




// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	vec3_t		lightDir;		// normalized direction towards light
	vec3_t		ambientLight;	// color normalized to 0-255
	unsigned char ambientLightRGBA[4]; // 32 bit rgba packed
    vec3_t		directedLight;
} trRefEntity_t;


typedef struct {
	float		modelMatrix[16] QALIGN(16);
	float		axis[3][3];		// orientation in world
    float		origin[3];			// in world coordinates
	float		viewOrigin[3];		// viewParms->or.origin in local coordinates
} orientationr_t;

//===============================================================================




#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;

typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_FOG,				// standard fog
	CGEN_CONST				// fixed color
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	genFunc_t	func;

	float base;
	float amplitude;
	float phase;
	float frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
} deformStage_t;


typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;


#define	MAX_IMAGE_ANIMATIONS	8

typedef struct {
	image_t*        image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t	*texMods;

	int				videoMapHandle;
	qboolean		isLightmap;
	qboolean		isVideoMap;
} textureBundle_t;

#define NUM_TEXTURE_BUNDLES 2

typedef struct {
	qboolean		active;
	
	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	unsigned char	constantColor[4];			// for CGEN_CONST and AGEN_CONST

	unsigned int	stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	qboolean		isDetail;

	// VULKAN
	VkPipeline		vk_pipeline;
	VkPipeline		vk_portal_pipeline;
	VkPipeline		vk_mirror_pipeline;

} shaderStage_t;

struct shaderCommands_s;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6], *innerbox[6];
} skyParms_t;

typedef struct {
	vec3_t	color;
	float	depthForOpaque;
} fogParms_t;


typedef struct shader_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			lightmapIndex;			// for a shader to match, both name and lightmapIndex must match

	int			index;					// this shader == tr.shaders[index]
	int			sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]

	float		sort;					// lower numbered shaders draw before higher numbered

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean	explicitlyDefined;		// found in a .shader file

	int			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	int			contentFlags;

	qboolean	entityMergable;			// merge across entites optimizable (smoke, blood)

	qboolean	isSky;
	skyParms_t	sky;
	fogParms_t	fogParms;

	float		portalRange;			// distance to fog out at

	int			multitextureEnv;		// 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean	polygonOffset;			// set for decals and other items that must be offset 
	qboolean	noMipMaps;				// for console fonts, 2D elements, etc.
	qboolean	noPicMip;				// for images that must always be full resolution

	fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals

	qboolean	needsNormal;			// not all shaders will need all data to be gathered
	qboolean	needsST1;
	qboolean	needsST2;
	qboolean	needsColor;

	int			numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	int			numUnfoggedPasses;
	shaderStage_t	*stages[MAX_SHADER_STAGES];		

    float clampTime;                                  // time this shader is clamped to
    float timeOffset;                                 // current time offset for this shader

    struct shader_s *remappedShader;                  // current shader this one is remapped too

	struct	shader_s	*next;
} shader_t;

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
/*
    int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	float		viewaxis[3][3];		// transformation matrix

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];


	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
*/
    refdef_t    rd;
    qboolean	AreamaskModified;	// qtrue if areamask changed since last scene
	float		floatTime;			// tr.refdef.time / 1000.0
	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s	*polys;

	int			numDrawSurfs;
	struct drawSurf_s	*drawSurfs;
} trRefdef_t;


//=================================================================================

// max surfaces per-skin
// This is an arbitry limit. Vanilla Q3 only supported 32 surfaces in skins but failed to
// enforce the maximum limit when reading skin files. It was possile to use more than 32
// surfaces which accessed out of bounds memory past end of skin->surfaces hunk block.
#define MAX_SKIN_SURFACES	256


// skins allow models to be retextured without modifying the model file
typedef struct {
	char		name[MAX_QPATH];
	shader_t	*shader;
} skinSurface_t;

typedef struct skin_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			numSurfaces;
	skinSurface_t* pSurfaces;    // dynamically allocated array of surfaces
} skin_t;


typedef struct {
	int			originalBrushNumber;
	vec3_t		bounds[2];

	unsigned char colorRGBA[4];			// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	qboolean	hasSurface;
	float		surface[4];
} fog_t;

typedef struct {
	orientationr_t	or;
	orientationr_t	world;
	vec3_t		pvsOrigin;			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
//	cplane_t	portalPlane;		// clip anything behind this if mirroring
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	float		fovX, fovY;
	float		projectionMatrix[16] QALIGN(16);
	cplane_t	frustum[4];
	vec3_t		visBounds[2];
	float		zFar;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_MDR,
	SF_IQM,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct drawSurf_s {
	unsigned		sort;			// bit combination for fast compares
	surfaceType_t * surface;		// any of surface*_t
} drawSurf_t;

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s {
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;


typedef struct srfFlare_s {
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
} srfFlare_t;

typedef struct srfGridMesh_s {
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits;

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;
	drawVert_t		verts[1];		// variable sized
} srfGridMesh_t;



#define	VERTEXSIZE	8
typedef struct {
	surfaceType_t	surfaceType;
	cplane_t	plane;

	// dynamic lighting information
	int			dlightBits;

	// triangle definitions (no normals at points)
	int			numPoints;
	int			numIndices;
	int			ofsIndices;
	float		points[1][VERTEXSIZE];	// variable sized
										// there is a variable length list of indices here also
} srfSurfaceFace_t;


// misc_models in maps are turned into direct geometry by q3map
typedef struct {
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits;

	// culling information (FIXME: use this!)
	vec3_t			bounds[2];
	vec3_t			localOrigin;
	float			radius;

	// triangle definitions
	int				numIndexes;
	int				*indexes;

	int				numVerts;
	drawVert_t		*verts;
} srfTriangles_t;




/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

typedef struct msurface_s {
	int					viewCount;		// if == tr.viewCount, already added
	struct shader_s		*shader;
	int					fogIndex;

	surfaceType_t		*data;			// any of srf*_t
} msurface_t;



#define	CONTENTS_NODE		-1
typedef struct mnode_s {
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_s	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	// leaf specific
	int			cluster;
	int			area;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
} mnode_t;

typedef struct {
	vec3_t		bounds[2];		// for culling
	msurface_t	*firstSurface;
	int			numSurfaces;
} bmodel_t;

typedef struct {
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	bmodel_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	int			numfogs;
	fog_t		*fogs;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	byte		*lightGridData;


	int			numClusters;
	int			clusterBytes;
	const unsigned char	*vis;			// may be passed in by CM_LoadMap to save space

	byte		*novis;			// clusterBytes of 0xff

	char		*entityString;
	char		*entityParsePoint;
} world_t;

//======================================================================



#define	MAX_DRAWIMAGES			2048
#define	MAX_LIGHTMAPS			256

#define	MAX_SKINS				1024
#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

21 - 31	: sorted shader index
11 - 20	: entity index
2 - 6	: fog index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
0 - 1	: dlightmap index

	TTimo - 1.32
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
0-1   : dlightmap index
*/
#define	QSORT_FOGNUM_SHIFT	2
#define	QSORT_ENTITYNUM_SHIFT	7
#define	QSORT_SHADERNUM_SHIFT	17




/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;


#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)




float R_NoiseGet4f( float x, float y, float z, float t );
void  R_NoiseInit( void );

void R_RenderView( viewParms_t *parms );

void R_AddMD3Surfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap );
void ScanAndLoadShaderFiles( void );
shader_t * GeneratePermanentShader( void );
qboolean ParseShader( char **text );



#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
//void R_LocalNormalToWorld (vec3_t local, vec3_t world);
//void R_LocalPointToWorld (vec3_t local, vec3_t world);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );

void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *or );


void R_InitScene(void);
void R_InitNextFrame(void);


void	R_ImageList_f( void );
void	R_SkinList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516


void	R_InitImages( void );
void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );


//
// tr_shader.c
//
// qhandle_t RE_RegisterShaderLightMap( const char *name, int lightmapIndex );


shader_t* R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
shader_t* R_GetShaderByHandle( qhandle_t hShader );
//shader_t* R_FindShaderByName( const char *name );

void R_InitShaders( void );
void R_ShaderList_f( void );
void R_ClearShaderHashTable(void);
void R_SetTheShader( const char *name, int lightmapIndex );
void R_UpdateShaderHashTable(shader_t* newShader);

void R_SetDefaultShader( void );
shader_t *FinishShader( void );
void R_CreateDefaultShadingCmds(const char* name, image_t* image);



/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/
typedef byte color4ub_t[4];

typedef struct stageVars
{
	color4ub_t	colors[SHADER_MAX_VERTEXES];
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

typedef struct shaderCommands_s 
{
	unsigned int indexes[SHADER_MAX_INDEXES];
	vec4_t		xyz[SHADER_MAX_VERTEXES];
	vec4_t		normal[SHADER_MAX_VERTEXES];
	vec2_t		texCoords[SHADER_MAX_VERTEXES][2];
	color4ub_t	vertexColors[SHADER_MAX_VERTEXES];
	int			vertexDlightBits[SHADER_MAX_VERTEXES];

	stageVars_t	svars;

	color4ub_t	constantColor255[SHADER_MAX_VERTEXES];

	shader_t	*shader;
    float   shaderTime;
	int			fogNum;

	int			dlightBits;	// or together of all vertexDlightBits

	int			numIndexes;
	int			numVertexes;

	// info extracted from current shader
	int			numPasses;
	shaderStage_t	**xstages;
} shaderCommands_t;


void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );




/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( void );

/*
============================================================

SKIES

============================================================
*/
void R_InitSkyTexCoords( float cloudLayerHeight );

/*
============================================================

CURVE TESSELATION

============================================================
*/

#define PATCH_STITCHING

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
								drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid );



/*
=============================================================

ANIMATED MODELS

=============================================================
*/
void R_MDRAddAnimSurfaces( trRefEntity_t *ent );
void R_AddAnimSurfaces( trRefEntity_t *ent );
void R_AddIQMSurfaces( trRefEntity_t *ent );

/*
=============================================================
=============================================================
*/


void	RB_DeformTessGeometry( void );

void	RB_CalcEnvironmentTexCoords( float *dstTexCoords );
void	RB_CalcFogTexCoords( float *dstTexCoords );
void	RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
void	RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
void	RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
void	RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
void	RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );
void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
void	RB_CalcModulateAlphasByFog( unsigned char *dstColors );
void	RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
void	RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcWaveColor( const waveForm_t *wf, unsigned char (*dstColors)[4] );
void	RB_CalcAlphaFromEntity( unsigned char *dstColors );
void	RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
void	RB_CalcColorFromEntity( unsigned char (*dstColors)[4] );
void	RB_CalcColorFromOneMinusEntity( unsigned char (*dstColors)[4] );
void	RB_CalcSpecularAlpha( unsigned char *alphas );
void	RB_CalcDiffuseColor( unsigned char (*colors)[4] );



/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct {
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct {
	int		commandId;
} drawBufferCommand_t;


typedef struct {
	int		commandId;
} swapBuffersCommand_t;

typedef struct {
	int		commandId;
} endFrameCommand_t;

typedef struct {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;


typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT,
    RC_VIDEOFRAME
} renderCommand_t;





/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );


void R_IssueRenderCommands( qboolean runPerformanceCounters );
void FixRenderCommandList( int newShader );
void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );


/*
============================================================

SCENE GENERATION

============================================================
*/


// font stuff
void R_InitFreeType(void);
void R_DoneFreeType(void);


extern void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

extern shaderCommands_t	tess;




#endif //TR_LOCAL_H
