#ifndef RENDER_EXPORT_H_
#define RENDER_EXPORT_H_

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "../renderercommon/tr_public.h"


// Total 30 exported function

void RE_Shutdown( qboolean destroyWindow );
void RE_BeginRegistration( glconfig_t *glconfig );

qhandle_t RE_RegisterModel( const char *name );
qhandle_t RE_RegisterSkin( const char *name );
qhandle_t RE_RegisterShader( const char *name );
qhandle_t RE_RegisterShaderNoMip( const char *name );

void RE_LoadWorldMap( const char *mapname );
void RE_SetWorldVisData( const byte *vis );
void RE_EndRegistration( void );
void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );

int RE_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_RenderScene( const refdef_t *fd );
void RE_SetColor( const float *rgba );

void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const unsigned char *data, int client, qboolean dirty);
void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );


// MARKERS, POLYGON PROJECTION ON WORLD POLYGONS
int RE_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

int	RE_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );

void RE_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );
void RE_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);

void RE_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

qboolean RE_GetEntityToken( char *buffer, int size );

qboolean RE_inPVS( const vec3_t p1, const vec3_t p2 );
void RE_TakeVideoFrame( int width, int height, unsigned char *captureBuffer, unsigned char *encodeBuffer, qboolean motionJpeg );

#endif
