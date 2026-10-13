#ifndef TR_LIGHT_H_
#define TR_LIGHT_H_


// can't be increased without changing bit packing for drawsurfs

typedef struct dlight_s {
	float	origin[3];
	float	color[3];	// range from 0.0 to 1.0, should be color normalized
	float	transformed[3];		// origin in local coordinate system
    float	radius;
	int		additive;		// texture detail is lost tho when the lightmap is dark
} dlight_t;



void R_DlightBmodel( bmodel_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, const orientationr_t * const or );



#endif
