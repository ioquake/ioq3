#ifndef TR_MODEL_H_
#define TR_MODEL_H_

#include "../renderercommon/iqm.h"




typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MDR,
	MOD_IQM
} modtype_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;		// model = tr.models[model->index]

	int			dataSize;	// just for listing purposes
	bmodel_t	*bmodel;		// only if type == MOD_BRUSH
	md3Header_t	*md3[MD3_MAX_LODS];	// only if type == MOD_MESH
	void	*modelData;			// only if type == (MOD_MDR | MOD_IQM)	

	int			 numLods;
} model_t;


#define	MAX_MOD_KNOWN	1024

void        R_ModelInit( void );
model_t*    R_GetModelByHandle( qhandle_t hModel );
void        R_Modellist_f( void );

//====================================================

qhandle_t R_RegisterMD3(const char *name, model_t *mod);

qboolean R_LoadMDR( model_t *mod, void *buffer, int filesize, const char *mod_name ); 
qhandle_t R_RegisterMDR(const char *name, model_t *mod);

qboolean R_LoadIQM (model_t *mod, void *buffer, int filesize, const char *name );
qhandle_t R_RegisterIQM(const char *name, model_t *mod);


//====================================================
// IQM

// inter-quake-model
typedef struct {
	int		num_vertexes;
	int		num_triangles;
	int		num_frames;
	int		num_surfaces;
	int		num_joints;
	int		num_poses;
	struct srfIQModel_s	*surfaces;

	float		*positions;
	float		*texcoords;
	float		*normals;
	float		*tangents;
	byte		*blendIndexes;
	union {
		float	*f;
		byte	*b;
	} blendWeights;
	byte		*colors;
	int		*triangles;

	// depending upon the exporter, blend indices and weights might be int/float
	// as opposed to the recommended byte/byte, for example Noesis exports
	// int/float whereas the official IQM tool exports byte/byte
	byte blendWeightsType; // IQM_UBYTE or IQM_FLOAT

	int		*jointParents;
	float		*jointMats;
	float		*poseMats;
	float		*bounds;
	char		*names;
} iqmData_t;

// inter-quake-model surface
typedef struct srfIQModel_s {
	surfaceType_t	surfaceType;
	char		name[MAX_QPATH];
	shader_t	*shader;
	iqmData_t	*data;
	int		first_vertex, num_vertexes;
	int		first_triangle, num_triangles;
} srfIQModel_t;


void R_AddIQMSurfaces( trRefEntity_t *ent );
void RB_IQMSurfaceAnim( surfaceType_t *surface );
void ComputePoseMats( iqmData_t *data, int frame, int oldframe, float backlerp, float *mat ); 



#endif
