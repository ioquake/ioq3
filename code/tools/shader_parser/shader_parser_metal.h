#ifndef SHADER_PARSER_METAL_H
#define SHADER_PARSER_METAL_H

#include <stdio.h>

#include "shader_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

qboolean ShaderParser_WriteJsonMetal(const shader_parser_string_list_t *names, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* SHADER_PARSER_METAL_H */
