#ifndef MATRIX_MULTIPLICATION_H_
#define MATRIX_MULTIPLICATION_H_


void Mat4Copy( const float in[64], float out[16] );
void Mat3x3Copy( float dst[3][3], const float src[3][3] );
void VectorLerp( float a[3], float b[3], float lerp, float out[3]);
void VectorNorm( float v[3] );
float VectorLen( const float v[3] );

void TransformModelToClip( const float src[3], const float *modelMatrix, const float *projectionMatrix, float eye[4], float dst[4] );
void TransformModelToClip_SSE( const float src[3], const float pMatModel[16], const float pMatProj[16], float dst[4] );
void Mat4Identity( float out[4] );
void MatrixMultiply4x4_SSE(const float A[16], const float B[16], float out[16]);
void Mat4x1Transform_SSE( const float A[16], const float x[4], float out[4] );
void Mat4Transform( const float in1[16], const float in2[4], float out[4] );
void Mat4Translation( float vec[3], float out[4] );
void Mat3x3Identity( float pMat[3][3] );
void VectorPerp( const vec3_t src, vec3_t dst );
float MakeTwoPerpVectors(const float forward[3], float right[3], float up[3]);

void Vec3Transform(const float Mat[16], const float v[3], float out[3]);

#endif
