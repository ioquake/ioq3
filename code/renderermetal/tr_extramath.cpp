// tr_extramath.cpp -- Metal renderer math helpers aligned with GL2 behavior

#include "tr_extramath.h"

extern "C" {
#include "../renderercommon/tr_public.h"
}

#include <algorithm>
#include <cstdint>

extern refimport_t ri;

void Mat4Zero(mat4_t out) {
	std::fill(out, out + 16, 0.0f);
}

void Mat4Identity(mat4_t out) {
	Mat4Zero(out);
	out[0] = out[5] = out[10] = out[15] = 1.0f;
}

void Mat4Copy(const mat4_t in, mat4_t out) {
	std::copy(in, in + 16, out);
}

void Mat4Multiply(const mat4_t in1, const mat4_t in2, mat4_t out) {
	// Column-major matrix multiplication for Metal
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			out[row + col * 4] =
				in1[row + 0] * in2[0 + col * 4] +
				in1[row + 4] * in2[1 + col * 4] +
				in1[row + 8] * in2[2 + col * 4] +
				in1[row + 12] * in2[3 + col * 4];
		}
	}
}

void Mat4Transform(const mat4_t in1, const vec4_t in2, vec4_t out) {
	for (int row = 0; row < 4; ++row) {
		out[row] = in1[row + 0] * in2[0] + in1[row + 4] * in2[1] + in1[row + 8] * in2[2] + in1[row + 12] * in2[3];
	}
}

qboolean Mat4Compare(const mat4_t a, const mat4_t b) {
	for (int i = 0; i < 16; ++i) {
		if (a[i] != b[i]) {
			return qfalse;
		}
	}
	return qtrue;
}

void Mat4Dump(const mat4_t in) {
	if (!ri.Printf) {
		return;
	}
	for (int row = 0; row < 4; ++row) {
		ri.Printf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n", in[row + 0], in[row + 4], in[row + 8], in[row + 12]);
	}
}

void Mat4Translation(vec3_t vec, mat4_t out) {
	Mat4Identity(out);
	out[12] = vec[0];
	out[13] = vec[1];
	out[14] = vec[2];
}

void Mat4Ortho(float left, float right, float bottom, float top, float znear, float zfar, mat4_t out) {
	Mat4Zero(out);
	out[0] = 2.0f / (right - left);
	out[5] = 2.0f / (top - bottom);
	out[10] = 2.0f / (zfar - znear);
	out[12] = -(right + left) / (right - left);
	out[13] = -(top + bottom) / (top - bottom);
	out[14] = -(zfar + znear) / (zfar - znear);
	out[15] = 1.0f;
}

void Mat4View(vec3_t axes[3], vec3_t origin, mat4_t out) {
	// Replicate GL1/GL2 R_RotateForViewer exactly with same memory layout
	// GL uses column-major storage, same as Metal
	
	float viewerMatrix[16];
	
	// Row 0: axes[0]
	viewerMatrix[0] = axes[0][0];
	viewerMatrix[4] = axes[0][1];
	viewerMatrix[8] = axes[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	// Row 1: axes[1]
	viewerMatrix[1] = axes[1][0];
	viewerMatrix[5] = axes[1][1];
	viewerMatrix[9] = axes[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	// Row 2: axes[2]
	viewerMatrix[2] = axes[2][0];
	viewerMatrix[6] = axes[2][1];
	viewerMatrix[10] = axes[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	// Row 3: 0, 0, 0, 1
	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;
	
	// s_flipMatrix exactly as defined in GL code (column-major, same as OpenGL convention)
	// This converts from Quake coords (looking down X) to OpenGL coords (looking down -Z)
	static const float flipMatrix[16] = {
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};
	
	// Multiply using the same algorithm as myGlMultMatrix
	// out[i*4+j] = sum over k of: viewerMatrix[i*4+k] * flipMatrix[k*4+j]
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			out[i * 4 + j] =
				viewerMatrix[i * 4 + 0] * flipMatrix[0 * 4 + j] +
				viewerMatrix[i * 4 + 1] * flipMatrix[1 * 4 + j] +
				viewerMatrix[i * 4 + 2] * flipMatrix[2 * 4 + j] +
				viewerMatrix[i * 4 + 3] * flipMatrix[3 * 4 + j];
		}
	}
}

void Mat4SimpleInverse(const mat4_t in, mat4_t out) {
	vec3_t v;
	float invLen;

	VectorCopy(in + 0, v);
	invLen = 1.0f / DotProduct(v, v);
	VectorScale(v, invLen, v);
	out[0] = v[0]; out[4] = v[1]; out[8] = v[2]; out[12] = -DotProduct(v, &in[12]);

	VectorCopy(in + 4, v);
	invLen = 1.0f / DotProduct(v, v);
	VectorScale(v, invLen, v);
	out[1] = v[0]; out[5] = v[1]; out[9] = v[2]; out[13] = -DotProduct(v, &in[12]);

	VectorCopy(in + 8, v);
	invLen = 1.0f / DotProduct(v, v);
	VectorScale(v, invLen, v);
	out[2] = v[0]; out[6] = v[1]; out[10] = v[2]; out[14] = -DotProduct(v, &in[12]);

	out[3] = out[7] = out[11] = 0.0f;
	out[15] = 1.0f;
}

void VectorLerp(vec3_t a, vec3_t b, float lerp, vec3_t c) {
	c[0] = a[0] * (1.0f - lerp) + b[0] * lerp;
	c[1] = a[1] * (1.0f - lerp) + b[1] * lerp;
	c[2] = a[2] * (1.0f - lerp) + b[2] * lerp;
}

qboolean SpheresIntersect(vec3_t origin1, float radius1, vec3_t origin2, float radius2) {
	const float radiusSum = radius1 + radius2;
	vec3_t diff;
	VectorSubtract(origin1, origin2, diff);
	return (DotProduct(diff, diff) <= radiusSum * radiusSum) ? qtrue : qfalse;
}

void BoundingSphereOfSpheres(vec3_t origin1, float radius1, vec3_t origin2, float radius2, vec3_t origin3, float* radius3) {
	vec3_t diff;
	VectorScale(origin1, 0.5f, origin3);
	VectorMA(origin3, 0.5f, origin2, origin3);
	VectorSubtract(origin1, origin2, diff);
	*radius3 = VectorLength(diff) * 0.5f + MAX(radius1, radius2);
}

int NextPowerOfTwo(int in) {
	int out = 1;
	while (out < in) {
		out <<= 1;
	}
	return out;
}

unsigned short FloatToHalf(float in) {
	union {
		float f;
		uint32_t ui;
		struct { unsigned int fraction:23; unsigned int exponent:8; unsigned int sign:1; } pack;
	} f32;
	union {
		uint16_t ui;
		struct { unsigned int fraction:10; unsigned int exponent:5; unsigned int sign:1; } pack;
	} f16;

	f32.f = in;
	f16.pack.exponent = CLAMP(static_cast<int>(f32.pack.exponent) - 112, 0, 31);
	f16.pack.fraction = f32.pack.fraction >> 13;
	f16.pack.sign = f32.pack.sign;
	return f16.ui;
}

float HalfToFloat(unsigned short in) {
	union {
		float f;
		uint32_t ui;
		struct { unsigned int fraction:23; unsigned int exponent:8; unsigned int sign:1; } pack;
	} f32;
	union {
		uint16_t ui;
		struct { unsigned int fraction:10; unsigned int exponent:5; unsigned int sign:1; } pack;
	} f16;

	f16.ui = in;
	f32.pack.exponent = static_cast<int>(f16.pack.exponent) + 112;
	f32.pack.fraction = f16.pack.fraction << 13;
	f32.pack.sign = f16.pack.sign;
	return f32.f;
}
