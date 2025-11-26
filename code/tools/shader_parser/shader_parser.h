#ifndef SHADER_PARSER_H
#define SHADER_PARSER_H

#include <stddef.h>

#include "qcommon/q_shared.h"

typedef enum {
    SHADER_PARSER_BACKEND_GL2 = 0,
    SHADER_PARSER_BACKEND_METAL
} shader_parser_backend_t;

typedef struct {
    char **items;
    size_t count;
} shader_parser_string_list_t;

typedef struct {
    shader_parser_backend_t backend;
    shader_parser_string_list_t scriptDirs;
    shader_parser_string_list_t shaderFilters;
    const char *outputPath;
    qboolean verbose;
} shader_parser_config_t;

typedef struct {
    size_t scriptFileCount;
    size_t loadedFileCount;
} shader_parser_stats_t;

#ifdef __cplusplus
extern "C" {
#endif

extern shader_parser_config_t gShaderParserConfig;
extern shader_parser_stats_t gShaderParserStats;

void ShaderParser_InitEnvironment( void );
void ShaderParser_ShutdownEnvironment( void );
const shader_parser_string_list_t *ShaderParser_GetLoadedSourceFiles( void );
qboolean ShaderParser_NameMatchesFilters( const char *name );

#ifdef __cplusplus
}
#endif

#endif /* SHADER_PARSER_H */
