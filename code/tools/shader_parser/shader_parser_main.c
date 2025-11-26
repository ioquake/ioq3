#include "shader_parser.h"
#include "shader_parser_metal.h"

#include "qcommon/qcommon.h"
#include "renderercommon/tr_common.h"
#include "renderergl2/tr_local.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_INDENT 2

typedef enum {
    ARG_RESULT_OK,
    ARG_RESULT_HELP,
    ARG_RESULT_ERROR
} shader_parser_arg_result_t;

static void ShaderParser_PrintUsage(const char *programName);
static qboolean ShaderParser_AddString(shader_parser_string_list_t *list, const char *value);
static void ShaderParser_DestroyStringList(shader_parser_string_list_t *list);
static shader_parser_arg_result_t ShaderParser_ParseArguments(int argc, char **argv);
static qboolean ShaderParser_EnsureDefaultScriptDirs(void);
static qboolean ShaderParser_EnumerateShaderNames(shader_parser_string_list_t *outNames);
static int QDECL ShaderParser_StringCompare(const void *lhs, const void *rhs);
qboolean ShaderParser_NameMatchesFilters(const char *name);
static void ShaderParser_Indent(FILE *stream, int indent);
static void ShaderParser_WriteJsonString(FILE *stream, const char *value);
static const char *ShaderParser_BlendFactorToString(unsigned stateBits, qboolean srcFactor);
static const char *ShaderParser_TcGenToString(texCoordGen_t tcGen);
static int ShaderParser_StateAlphaFunc(unsigned stateBits);
static void ShaderParser_WriteStageJson(FILE *stream, const shaderStage_t *stage, int indent);
static void ShaderParser_WriteShaderJson(FILE *stream, const shader_t *shader, int indent);
static qboolean ShaderParser_WriteJsonGL2(const shader_parser_string_list_t *names, FILE *stream);
static FILE *ShaderParser_OpenOutput(void);
static void ShaderParser_CloseOutput(FILE *stream);
static void ShaderParser_ResetConfig(void);

int main(int argc, char **argv) {
    ShaderParser_ResetConfig();

    shader_parser_arg_result_t argResult = ShaderParser_ParseArguments(argc, argv);
    if (argResult == ARG_RESULT_HELP) {
        ShaderParser_PrintUsage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argResult == ARG_RESULT_ERROR) {
        ShaderParser_PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!ShaderParser_EnsureDefaultScriptDirs()) {
        fprintf(stderr, "shader_parser: no script directories provided\n");
        return EXIT_FAILURE;
    }

    ShaderParser_InitEnvironment();
    R_InitShaders();

    shader_parser_string_list_t shaderNames = {0};
    if (!ShaderParser_EnumerateShaderNames(&shaderNames)) {
        ShaderParser_ShutdownEnvironment();
        ShaderParser_ResetConfig();
        return EXIT_FAILURE;
    }

    FILE *output = ShaderParser_OpenOutput();
    if (!output) {
        ShaderParser_DestroyStringList(&shaderNames);
        ShaderParser_ShutdownEnvironment();
        ShaderParser_ResetConfig();
        return EXIT_FAILURE;
    }

    qboolean writeOk = qfalse;
    if (gShaderParserConfig.backend == SHADER_PARSER_BACKEND_METAL) {
        writeOk = ShaderParser_WriteJsonMetal(&shaderNames, output);
    } else {
        writeOk = ShaderParser_WriteJsonGL2(&shaderNames, output);
    }

    if (!writeOk) {
        ShaderParser_CloseOutput(output);
        ShaderParser_DestroyStringList(&shaderNames);
        ShaderParser_ShutdownEnvironment();
        ShaderParser_ResetConfig();
        return EXIT_FAILURE;
    }

    ShaderParser_CloseOutput(output);
    ShaderParser_DestroyStringList(&shaderNames);
    ShaderParser_ShutdownEnvironment();
    ShaderParser_ResetConfig();
    return EXIT_SUCCESS;
}

static void ShaderParser_PrintUsage(const char *programName) {
    fprintf(stderr,
            "Usage: %s [options]\n"
            "  -s, --scripts <dir>   Add a directory that contains .shader files (can repeat)\n"
            "  -f, --filter  <glob>  Limit output to shader names matching the glob (can repeat)\n"
            "  -b, --backend <name>  Select backend: gl2 (default) or metal\n"
            "  -o, --output  <path>  Write JSON to the provided file (default: stdout)\n"
            "  -v, --verbose         Enable verbose renderer logging\n"
            "  -h, --help            Show this help message\n",
            programName ? programName : "shader_parser");
}

static qboolean ShaderParser_AddString(shader_parser_string_list_t *list, const char *value) {
    if (!list || !value) {
        return qfalse;
    }

    size_t length = strlen(value);
    if (length == 0) {
        return qtrue;
    }

    char **next = realloc(list->items, sizeof(char *) * (list->count + 1));
    if (!next) {
        fprintf(stderr, "shader_parser: out of memory while growing string list\n");
        return qfalse;
    }

    list->items = next;
    list->items[list->count] = strdup(value);
    if (!list->items[list->count]) {
        fprintf(stderr, "shader_parser: out of memory while duplicating string\n");
        return qfalse;
    }

    list->count++;
    return qtrue;
}

static void ShaderParser_DestroyStringList(shader_parser_string_list_t *list) {
    if (!list || !list->items) {
        return;
    }

    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
}

static shader_parser_arg_result_t ShaderParser_ParseArguments(int argc, char **argv) {
    shader_parser_arg_result_t result = ARG_RESULT_OK;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            return ARG_RESULT_HELP;
        } else if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
            gShaderParserConfig.verbose = qtrue;
        } else if (!strcmp(arg, "-s") || !strcmp(arg, "--scripts")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "shader_parser: missing value for %s\n", arg);
                return ARG_RESULT_ERROR;
            }
            if (!ShaderParser_AddString(&gShaderParserConfig.scriptDirs, argv[++i])) {
                return ARG_RESULT_ERROR;
            }
        } else if (!strcmp(arg, "-f") || !strcmp(arg, "--filter")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "shader_parser: missing value for %s\n", arg);
                return ARG_RESULT_ERROR;
            }
            if (!ShaderParser_AddString(&gShaderParserConfig.shaderFilters, argv[++i])) {
                return ARG_RESULT_ERROR;
            }
        } else if (!strcmp(arg, "-b") || !strcmp(arg, "--backend")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "shader_parser: missing value for %s\n", arg);
                return ARG_RESULT_ERROR;
            }
            const char *backend = argv[++i];
            if (!Q_stricmp(backend, "gl2")) {
                gShaderParserConfig.backend = SHADER_PARSER_BACKEND_GL2;
            } else if (!Q_stricmp(backend, "metal")) {
                gShaderParserConfig.backend = SHADER_PARSER_BACKEND_METAL;
            } else {
                fprintf(stderr, "shader_parser: unknown backend '%s' (expected gl2 or metal)\n", backend);
                return ARG_RESULT_ERROR;
            }
        } else if (!strcmp(arg, "-o") || !strcmp(arg, "--output")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "shader_parser: missing value for %s\n", arg);
                return ARG_RESULT_ERROR;
            }
            gShaderParserConfig.outputPath = argv[++i];
        } else {
            fprintf(stderr, "shader_parser: unknown argument '%s'\n", arg);
            result = ARG_RESULT_ERROR;
        }
    }

    return result;
}

static qboolean ShaderParser_EnsureDefaultScriptDirs(void) {
    if (gShaderParserConfig.scriptDirs.count > 0) {
        return qtrue;
    }

    const char *defaults[] = {
        "baseq3/scripts",
        "missionpack/scripts"
    };

    for (size_t i = 0; i < ARRAY_LEN(defaults); ++i) {
        ShaderParser_AddString(&gShaderParserConfig.scriptDirs, defaults[i]);
    }

    return gShaderParserConfig.scriptDirs.count > 0;
}

static qboolean ShaderParser_AddUnique(shader_parser_string_list_t *list, const char *value) {
    for (size_t i = 0; i < list->count; ++i) {
        if (!Q_stricmp(list->items[i], value)) {
            return qtrue;
        }
    }

    return ShaderParser_AddString(list, value);
}

static qboolean ShaderParser_EnumerateShaderNames(shader_parser_string_list_t *outNames) {
    if (!outNames) {
        return qfalse;
    }

    int numFiles = 0;
    char **shaderFiles = ri.FS_ListFiles("scripts", ".shader", &numFiles);
    if (!shaderFiles || numFiles == 0) {
        fprintf(stderr, "shader_parser: no .shader files found\n");
        ri.FS_FreeFileList(shaderFiles);
        return qfalse;
    }

    for (int i = 0; i < numFiles; ++i) {
        if (!shaderFiles[i]) {
            continue;
        }

        char virtualPath[MAX_OSPATH];
        Com_sprintf(virtualPath, sizeof(virtualPath), "scripts/%s", shaderFiles[i]);

        char *fileBuffer = NULL;
        long fileLen = ri.FS_ReadFile(virtualPath, (void **)&fileBuffer);
        if (fileLen <= 0 || !fileBuffer) {
            continue;
        }

        char *cursor = fileBuffer;
        while (1) {
            char *token = COM_ParseExt(&cursor, qtrue);
            if (!token[0]) {
                break;
            }

            if (!ShaderParser_AddUnique(outNames, token)) {
                ri.FS_FreeFile(fileBuffer);
                ri.FS_FreeFileList(shaderFiles);
                return qfalse;
            }

            if (!SkipBracedSection(&cursor, 0)) {
                break;
            }
        }

        ri.FS_FreeFile(fileBuffer);
    }

    ri.FS_FreeFileList(shaderFiles);

    if (outNames->count > 1) {
        qsort(outNames->items, outNames->count, sizeof(char *), ShaderParser_StringCompare);
    }

    return qtrue;
}

static int QDECL ShaderParser_StringCompare(const void *lhs, const void *rhs) {
    const char *const *left = lhs;
    const char *const *right = rhs;
    return Q_stricmp(*left, *right);
}

qboolean ShaderParser_NameMatchesFilters(const char *name) {
    if (!name) {
        return qfalse;
    }

    if (gShaderParserConfig.shaderFilters.count == 0) {
        return qtrue;
    }

    for (size_t i = 0; i < gShaderParserConfig.shaderFilters.count; ++i) {
        const char *filter = gShaderParserConfig.shaderFilters.items[i];
        if (!filter) {
            continue;
        }

        char filterBuffer[MAX_QPATH];
        char nameBuffer[MAX_QPATH];
        Q_strncpyz(filterBuffer, filter, sizeof(filterBuffer));
        Q_strncpyz(nameBuffer, name, sizeof(nameBuffer));

        if (Com_Filter(filterBuffer, nameBuffer, qfalse)) {
            return qtrue;
        }
    }

    return qfalse;
}

static const char *ShaderParser_BlendFactorToString(unsigned stateBits, qboolean srcFactor) {
    unsigned value = srcFactor ? (stateBits & GLS_SRCBLEND_BITS)
                               : (stateBits & GLS_DSTBLEND_BITS);
    switch (value) {
        case GLS_SRCBLEND_ZERO:
        case GLS_DSTBLEND_ZERO:
            return "GL_ZERO";
        case GLS_SRCBLEND_ONE:
        case GLS_DSTBLEND_ONE:
            return "GL_ONE";
        case GLS_SRCBLEND_DST_COLOR:
            return "GL_DST_COLOR";
        case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
            return "GL_ONE_MINUS_DST_COLOR";
        case GLS_SRCBLEND_SRC_ALPHA:
            return "GL_SRC_ALPHA";
        case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
            return "GL_ONE_MINUS_SRC_ALPHA";
        case GLS_SRCBLEND_DST_ALPHA:
            return "GL_DST_ALPHA";
        case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
            return "GL_ONE_MINUS_DST_ALPHA";
        case GLS_DSTBLEND_SRC_COLOR:
            return "GL_SRC_COLOR";
        case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
            return "GL_ONE_MINUS_SRC_COLOR";
        case GLS_DSTBLEND_SRC_ALPHA:
            return "GL_SRC_ALPHA";
        case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
            return "GL_ONE_MINUS_SRC_ALPHA";
        case GLS_DSTBLEND_DST_ALPHA:
            return "GL_DST_ALPHA";
        case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
            return "GL_ONE_MINUS_DST_ALPHA";
        default:
            return srcFactor ? "GL_ONE" : "GL_ZERO";
    }
}

static const char *ShaderParser_TcGenToString(texCoordGen_t tcGen) {
    switch (tcGen) {
        case TCGEN_LIGHTMAP:
            return "lightmap";
        case TCGEN_ENVIRONMENT_MAPPED:
            return "environment";
        case TCGEN_FOG:
            return "fog";
        case TCGEN_VECTOR:
            return "vector";
        case TCGEN_IDENTITY:
            return "identity";
        case TCGEN_TEXTURE:
        default:
            return "texture";
    }
}

static int ShaderParser_StateAlphaFunc(unsigned stateBits) {
    switch (stateBits & GLS_ATEST_BITS) {
        case GLS_ATEST_GT_0:
            return 1;
        case GLS_ATEST_LT_80:
            return 2;
        case GLS_ATEST_GE_80:
            return 3;
        default:
            return 0;
    }
}

static void ShaderParser_Indent(FILE *stream, int indent) {
    for (int i = 0; i < indent; ++i) {
        fputc(' ', stream);
    }
}

static void ShaderParser_WriteJsonString(FILE *stream, const char *value) {
    fputc('"', stream);
    if (value) {
        for (const char *cursor = value; *cursor; ++cursor) {
            const unsigned char ch = (unsigned char)*cursor;
            if (ch == '"' || ch == '\\') {
                fputc('\\', stream);
                fputc((int)ch, stream);
            } else if (ch == '\n') {
                fputs("\\n", stream);
            } else if (ch == '\r') {
                fputs("\\r", stream);
            } else if (ch == '\t') {
                fputs("\\t", stream);
            } else if (ch < 0x20) {
                fprintf(stream, "\\u%04x", ch);
            } else {
                fputc((int)ch, stream);
            }
        }
    }
    fputc('"', stream);
}

static int ShaderParser_BundleImageCount(const textureBundle_t *bundle) {
    if (!bundle) {
        return 0;
    }

    if (bundle->numImageAnimations > 0) {
        return bundle->numImageAnimations;
    }

    return bundle->image[0] ? 1 : 0;
}

static void ShaderParser_WriteBundleJson(FILE *stream, const textureBundle_t *bundle, int slot, int indent) {
    const int imageCount = ShaderParser_BundleImageCount(bundle);
    if (!bundle || imageCount <= 0) {
        return;
    }

    ShaderParser_Indent(stream, indent);
    fprintf(stream, "{\n");
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"slot\": %d,\n", slot);
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"images\": [");

    for (int i = 0; i < imageCount; ++i) {
        if (i > 0) {
            fprintf(stream, ", ");
        }
        const image_t *image = bundle->image[i];
        ShaderParser_WriteJsonString(stream, image ? image->imgName : "");
    }

    fprintf(stream, "]\n");
    ShaderParser_Indent(stream, indent);
    fprintf(stream, "}");
}

static void ShaderParser_WriteStageJson(FILE *stream, const shaderStage_t *stage, int indent) {
    ShaderParser_Indent(stream, indent);
    fprintf(stream, "{\n");
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"active\": %s,\n", stage->active ? "true" : "false");
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"type\": %d,\n", stage->type);
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"stateBits\": %u,\n", stage->stateBits);
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"srcBlend\": \"%s\",\n",
        ShaderParser_BlendFactorToString(stage->stateBits, qtrue));
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"dstBlend\": \"%s\",\n",
        ShaderParser_BlendFactorToString(stage->stateBits, qfalse));
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"depthWrite\": %s,\n",
        (stage->stateBits & GLS_DEPTHMASK_TRUE) ? "true" : "false");
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"alphaFunc\": %d,\n", ShaderParser_StateAlphaFunc(stage->stateBits));

    const textureBundle_t *primaryBundle = &stage->bundle[TB_DIFFUSEMAP];
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"tcGen\": \"%s\",\n",
        ShaderParser_TcGenToString(primaryBundle->tcGen));
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"tcModCount\": %d,\n", primaryBundle->numTexMods);
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    qboolean usesLightmap = qfalse;
    if (primaryBundle->isLightmap) {
        usesLightmap = qtrue;
    } else if (ShaderParser_BundleImageCount(&stage->bundle[TB_LIGHTMAP]) > 0) {
        usesLightmap = qtrue;
    }

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"usesLightmap\": %s,\n", usesLightmap ? "true" : "false");
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"rgbGen\": %d,\n", stage->rgbGen);
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"alphaGen\": %d,\n", stage->alphaGen);
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"isDetail\": %s,\n", stage->isDetail ? "true" : "false");
    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"bundles\": [\n");

    qboolean emitted = qfalse;
    for (int slot = 0; slot < NUM_TEXTURE_BUNDLES; ++slot) {
        const textureBundle_t *bundle = &stage->bundle[slot];
        if (ShaderParser_BundleImageCount(bundle) <= 0) {
            continue;
        }

        if (emitted) {
            fprintf(stream, ",\n");
        }

        ShaderParser_WriteBundleJson(stream, bundle, slot, indent + JSON_INDENT * 2);
        emitted = qtrue;
    }

    if (emitted) {
        fprintf(stream, "\n");
        ShaderParser_Indent(stream, indent + JSON_INDENT);
        fprintf(stream, "]\n");
    } else {
        fprintf(stream, "]\n");
    }

    ShaderParser_Indent(stream, indent);
    fprintf(stream, "}");
}

static void ShaderParser_WriteShaderJson(FILE *stream, const shader_t *shader, int indent) {
    ShaderParser_Indent(stream, indent);
    fprintf(stream, "{\n");

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"name\": ");
    ShaderParser_WriteJsonString(stream, shader->name);
    fprintf(stream, ",\n");

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"explicit\": %s,\n", shader->explicitlyDefined ? "true" : "false");

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"surfaceFlags\": %d,\n", shader->surfaceFlags);

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"contentFlags\": %d,\n", shader->contentFlags);

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"cullType\": %d,\n", shader->cullType);

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"polygonOffset\": %s,\n", shader->polygonOffset ? "true" : "false");

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"sort\": %.6f,\n", shader->sort);

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"numStages\": %d,\n", shader->numUnfoggedPasses);

    ShaderParser_Indent(stream, indent + JSON_INDENT);
    fprintf(stream, "\"stages\": [\n");

    qboolean emitted = qfalse;
    for (int i = 0; i < shader->numUnfoggedPasses && i < MAX_SHADER_STAGES; ++i) {
        const shaderStage_t *stage = shader->stages[i];
        if (!stage) {
            continue;
        }

        if (emitted) {
            fprintf(stream, ",\n");
        }

        ShaderParser_WriteStageJson(stream, stage, indent + JSON_INDENT * 2);
        emitted = qtrue;
    }

    if (emitted) {
        fprintf(stream, "\n");
        ShaderParser_Indent(stream, indent + JSON_INDENT);
        fprintf(stream, "]\n");
    } else {
        fprintf(stream, "]\n");
    }

    ShaderParser_Indent(stream, indent);
    fprintf(stream, "}");
}

static qboolean ShaderParser_WriteJsonGL2(const shader_parser_string_list_t *names, FILE *stream) {
    if (!names || !stream) {
        return qfalse;
    }

    fprintf(stream, "{\n");

    ShaderParser_Indent(stream, JSON_INDENT);
    fprintf(stream, "\"stats\": {\n");
    ShaderParser_Indent(stream, JSON_INDENT * 2);
    fprintf(stream, "\"scriptFiles\": %zu,\n", gShaderParserStats.scriptFileCount);
    ShaderParser_Indent(stream, JSON_INDENT * 2);
    fprintf(stream, "\"loadedFiles\": %zu\n", gShaderParserStats.loadedFileCount);
    ShaderParser_Indent(stream, JSON_INDENT);
    fprintf(stream, "},\n");

    ShaderParser_Indent(stream, JSON_INDENT);
    fprintf(stream, "\"shaders\": [\n");

    size_t emitted = 0;
    for (size_t i = 0; i < names->count; ++i) {
        const char *name = names->items[i];
        if (!name || !ShaderParser_NameMatchesFilters(name)) {
            continue;
        }

        shader_t *shader = R_FindShader(name, LIGHTMAP_NONE, qtrue);
        if (!shader) {
            continue;
        }

        if (emitted > 0) {
            fprintf(stream, ",\n");
        }

        ShaderParser_WriteShaderJson(stream, shader, JSON_INDENT * 2);
        emitted++;
    }

    if (emitted > 0) {
        fprintf(stream, "\n");
    }

    ShaderParser_Indent(stream, JSON_INDENT);
    fprintf(stream, "]\n");
    fprintf(stream, "}\n");
    return qtrue;
}

static FILE *ShaderParser_OpenOutput(void) {
    if (!gShaderParserConfig.outputPath || !strcmp(gShaderParserConfig.outputPath, "-")) {
        return stdout;
    }

    FILE *stream = fopen(gShaderParserConfig.outputPath, "wb");
    if (!stream) {
        fprintf(stderr, "shader_parser: failed to open '%s' for writing: %s\n",
                gShaderParserConfig.outputPath, strerror(errno));
        return NULL;
    }

    return stream;
}

static void ShaderParser_CloseOutput(FILE *stream) {
    if (!stream || stream == stdout) {
        return;
    }

    fclose(stream);
}

static void ShaderParser_ResetConfig(void) {
    ShaderParser_DestroyStringList(&gShaderParserConfig.scriptDirs);
    ShaderParser_DestroyStringList(&gShaderParserConfig.shaderFilters);
    gShaderParserConfig.outputPath = NULL;
    gShaderParserConfig.verbose = qfalse;
    gShaderParserConfig.backend = SHADER_PARSER_BACKEND_GL2;
}
