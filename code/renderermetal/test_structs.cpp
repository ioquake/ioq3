#include <stdio.h>
#include <stddef.h>

typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec3_t[3];

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} drawVert_t;

int main() {
    printf("sizeof(drawVert_t) = %zu\n", sizeof(drawVert_t));
    printf("offset(xyz) = %zu\n", offsetof(drawVert_t, xyz));
    printf("offset(st) = %zu\n", offsetof(drawVert_t, st));
    printf("offset(lightmap) = %zu\n", offsetof(drawVert_t, lightmap));
    printf("offset(normal) = %zu\n", offsetof(drawVert_t, normal));
    printf("offset(color) = %zu\n", offsetof(drawVert_t, color));
    return 0;
}
