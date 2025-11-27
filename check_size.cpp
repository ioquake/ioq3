#include <stdio.h>
#include <stddef.h>

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef unsigned char byte;

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} drawVert_t;

int main() {
    printf("sizeof(drawVert_t) = %zu\n", sizeof(drawVert_t));
    printf("offsetof(xyz) = %zu\n", offsetof(drawVert_t, xyz));
    printf("offsetof(st) = %zu\n", offsetof(drawVert_t, st));
    printf("offsetof(lightmap) = %zu\n", offsetof(drawVert_t, lightmap));
    printf("offsetof(normal) = %zu\n", offsetof(drawVert_t, normal));
    printf("offsetof(color) = %zu\n", offsetof(drawVert_t, color));
    return 0;
}
