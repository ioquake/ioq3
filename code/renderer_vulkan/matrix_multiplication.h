/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2017-2025 Artem Kharytoniuk
Copyright (C) 2019-2024 Sui Jingfeng and the vkQuake3 contributors

This file is part of the Vulkan renderer of ioquake3, ported from vkQuake3
(https://github.com/suijingfeng/vkQuake3), whose Vulkan code is a C
conversion of the Vulkan backend of Quake-III-Arena-Kenny-Edition
(https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).

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

#ifndef MATRIX_MULTIPLICATION_H_
#define MATRIX_MULTIPLICATION_H_


void Mat4Copy( const float in[16], float out[16] );
void Mat3x3Copy( float dst[3][3], const float src[3][3] );
void VectorLerp( float a[3], float b[3], float lerp, float out[3]);
void VectorNorm( float v[3] );
float VectorLen( const float v[3] );

void TransformModelToClip( const float src[3], const float *modelMatrix, const float *projectionMatrix, float eye[4], float dst[4] );
void TransformModelToClip_SSE( const float src[3], const float pMatModel[16], const float pMatProj[16], float dst[4] );
void Mat4Identity( float out[16] );
void MatrixMultiply4x4_SSE(const float A[16], const float B[16], float out[16]);
void Mat4x1Transform_SSE( const float A[16], const float x[4], float out[4] );
void Mat4Transform( const float in1[16], const float in2[4], float out[4] );
void Mat4Translation( float vec[3], float out[16] );
void Mat3x3Identity( float pMat[3][3] );
void VectorPerp( const vec3_t src, vec3_t dst );
float MakeTwoPerpVectors(const float forward[3], float right[3], float up[3]);

void Vec3Transform(const float Mat[16], const float v[3], float out[3]);

#endif
