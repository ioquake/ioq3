#include "shader_parser.h"

#include "renderercommon/tr_common.h"
#include "renderergl2/tr_local.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct imageNode_s {
    image_t *image;
    struct imageNode_s *next;
} imageNode_t;

static imageNode_t *gImagePool;
static shader_parser_string_list_t gLoadedSources;
static qboolean gEnvironmentInitialized = qfalse;

shader_parser_config_t gShaderParserConfig;
shader_parser_stats_t gShaderParserStats;

backEndData_t *backEndData;
int max_polys = 1;
int max_polyverts = 1;

refimport_t ri;
glconfig_t glConfig;
qboolean textureFilterAnisotropic = qfalse;
int maxAnisotropy = 0;
float displayAspect = 0.0f;
qboolean haveClampToEdge = qfalse;
trGlobals_t tr;
backEndState_t backEnd;
glstate_t glState;
glRefConfig_t glRefConfig;

#define DEFINE_STUB_CVAR(identifier, stringLiteral, numericValue)            \
    static cvar_t identifier##_storage = {                                   \
        .name = #identifier,                                                 \
        .string = (char *)(stringLiteral),                                   \
        .resetString = (char *)(stringLiteral),                              \
        .latchedString = NULL,                                               \
        .flags = 0,                                                          \
        .modified = qfalse,                                                  \
        .modificationCount = 0,                                              \
        .value = (numericValue),                                             \
        .integer = (int)(numericValue),                                      \
        .validate = qfalse,                                                  \
        .integral = qfalse,                                                  \
        .min = 0.0f,                                                         \
        .max = 0.0f,                                                         \
        .description = NULL,                                                 \
        .next = NULL,                                                        \
        .prev = NULL,                                                        \
        .hashNext = NULL,                                                    \
        .hashPrev = NULL,                                                    \
        .hashIndex = 0                                                       \
    };                                                                       \
    cvar_t *identifier = &identifier##_storage

DEFINE_STUB_CVAR(r_ignoreDstAlpha, "1", 1.0f);
DEFINE_STUB_CVAR(r_genNormalMaps, "0", 0.0f);
DEFINE_STUB_CVAR(r_baseNormalX, "1.0", 1.0f);
DEFINE_STUB_CVAR(r_baseNormalY, "1.0", 1.0f);
DEFINE_STUB_CVAR(r_baseParallax, "0.05", 0.05f);
DEFINE_STUB_CVAR(r_parallaxMapping, "0", 0.0f);
DEFINE_STUB_CVAR(r_pbr, "0", 0.0f);
DEFINE_STUB_CVAR(r_baseSpecular, "0.04", 0.04f);
DEFINE_STUB_CVAR(r_baseGloss, "0.3", 0.3f);
DEFINE_STUB_CVAR(r_greyscale, "0", 0.0f);
DEFINE_STUB_CVAR(r_normalMapping, "1", 1.0f);
DEFINE_STUB_CVAR(r_specularMapping, "1", 1.0f);
DEFINE_STUB_CVAR(r_deluxeMapping, "1", 1.0f);
DEFINE_STUB_CVAR(r_sunShadows, "1", 1.0f);
DEFINE_STUB_CVAR(r_sunlightMode, "1", 1.0f);
DEFINE_STUB_CVAR(r_mergeLightmaps, "1", 1.0f);
DEFINE_STUB_CVAR(r_detailTextures, "1", 1.0f);
DEFINE_STUB_CVAR(r_vertexLight, "0", 0.0f);
DEFINE_STUB_CVAR(r_uiFullScreen, "0", 0.0f);
DEFINE_STUB_CVAR(r_printShaders, "0", 0.0f);

#undef DEFINE_STUB_CVAR

static void ShaderParser_FreeStringList(shader_parser_string_list_t *list) {
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

static qboolean ShaderParser_StringEquals(const char *lhs, const char *rhs) {
    return Q_stricmp(lhs, rhs) == 0;
}

static void ShaderParser_RecordLoadedFile(const char *path) {
    for (size_t i = 0; i < gLoadedSources.count; ++i) {
        if (ShaderParser_StringEquals(gLoadedSources.items[i], path)) {
            gShaderParserStats.loadedFileCount = gLoadedSources.count;
            return;
        }
    }

    char **next = realloc(gLoadedSources.items, sizeof(char *) * (gLoadedSources.count + 1));
    if (!next) {
        return;
    }

    gLoadedSources.items = next;
    gLoadedSources.items[gLoadedSources.count] = strdup(path);
    if (!gLoadedSources.items[gLoadedSources.count]) {
        return;
    }

    gLoadedSources.count++;
    gShaderParserStats.loadedFileCount = gLoadedSources.count;
}

const shader_parser_string_list_t *ShaderParser_GetLoadedSourceFiles(void) {
    return &gLoadedSources;
}

static void ShaderParser_ClearLoadedSources(void) {
    ShaderParser_FreeStringList(&gLoadedSources);
}

static void ShaderParser_FreeImagePool(void) {
    imageNode_t *node = gImagePool;
    while (node) {
        imageNode_t *next = node->next;
        free(node->image);
        free(node);
        node = next;
    }
    gImagePool = NULL;
}

static image_t *ShaderParser_CreateImage(const char *name) {
    image_t *img = calloc(1, sizeof(image_t));
    if (!img) {
        return NULL;
    }

    Q_strncpyz(img->imgName, name, sizeof(img->imgName));

    imageNode_t *node = calloc(1, sizeof(imageNode_t));
    if (!node) {
        free(img);
        return NULL;
    }

    node->image = img;
    node->next = gImagePool;
    gImagePool = node;

    return img;
}

static void ShaderParser_FreeHunkBlocks(void) {
    // Intentionally left blank. The standalone tool allocates hunk memory
    // once per run and lets the OS reclaim it on exit, which avoids double
    // frees when renderer code releases buffers manually.
}

static void *ShaderParser_HunkAllocInternal(int size) {
    if (size <= 0) {
        return NULL;
    }

    return calloc(1, (size_t)size);
}

static void ShaderParser_InitBackEndData(void) {
    if (backEndData) {
        return;
    }

    backEndData = calloc(1, sizeof(backEndData_t));
    if (!backEndData) {
        return;
    }

    backEndData->polys = calloc((size_t)max_polys, sizeof(srfPoly_t));
    backEndData->polyVerts = calloc((size_t)max_polyverts, sizeof(polyVert_t));
}

static void ShaderParser_InitStubImages(void) {
    tr.identityLight = 1.0f;
    tr.identityLightByte = 255;
    tr.overbrightBits = 0;

    tr.defaultImage = ShaderParser_CreateImage("<default>");
    tr.whiteImage = ShaderParser_CreateImage("<white>");
    tr.identityLightImage = ShaderParser_CreateImage("<identity>");
    tr.dlightImage = ShaderParser_CreateImage("<dlight>");

    for (int i = 0; i < 32; ++i) {
        char label[MAX_QPATH];
        Com_sprintf(label, sizeof(label), "<scratch_%d>", i);
        tr.scratchImage[i] = ShaderParser_CreateImage(label);
    }
}

static qboolean ShaderParser_ResolveScriptsPath(const char *virtualPath, char *resolved, size_t resolvedSize) {
    const char *relative = virtualPath;
    if (!Q_stricmpn(virtualPath, "scripts/", 8)) {
        relative = virtualPath + 8;
    }

    if (relative[0] == '\0') {
        return qfalse;
    }

    Q_strncpyz(resolved, relative, resolvedSize);
    return qtrue;
}

static image_t *ShaderParser_GetFallbackImage(void) {
    if (tr.whiteImage) {
        return tr.whiteImage;
    }

    return ShaderParser_CreateImage("<fallback>");
}

static char *ShaderParser_StringContains(char *str1, char *str2, int casesensitive) {
    int len, i, j;

    len = (int)strlen(str1) - (int)strlen(str2);
    for (i = 0; i <= len; i++, str1++) {
        for (j = 0; str2[j]; j++) {
            if (casesensitive) {
                if (str1[j] != str2[j]) {
                    break;
                }
            } else {
                if (toupper(str1[j]) != toupper(str2[j])) {
                    break;
                }
            }
        }
        if (!str2[j]) {
            return str1;
        }
    }

    return NULL;
}

int Com_Filter(char *filter, char *name, int casesensitive) {
    char buf[MAX_TOKEN_CHARS];
    char *ptr;
    int i;
    int found;

    while (*filter) {
        if (*filter == '*') {
            filter++;
            for (i = 0; *filter; i++) {
                if (*filter == '*' || *filter == '?' || *filter == '[') {
                    break;
                }
                buf[i] = *filter;
                filter++;
            }
            buf[i] = '\0';
            if (strlen(buf)) {
                ptr = ShaderParser_StringContains(name, buf, casesensitive);
                if (!ptr) {
                    return qfalse;
                }
                name = ptr + strlen(buf);
            }
        } else if (*filter == '?') {
            filter++;
            name++;
        } else if (*filter == '[' && *(filter + 1) == '[') {
            filter++;
        } else if (*filter == '[') {
            filter++;
            found = qfalse;
            while (*filter && !found) {
                if (*filter == ']' && *(filter + 1) != ']') {
                    break;
                }
                if (*(filter + 1) == '-' && *(filter + 2) && (*(filter + 2) != ']' || *(filter + 3) == ']')) {
                    if (casesensitive) {
                        if (*name >= *filter && *name <= *(filter + 2)) {
                            found = qtrue;
                        }
                    } else {
                        if (toupper(*name) >= toupper(*filter) &&
                            toupper(*name) <= toupper(*(filter + 2))) {
                            found = qtrue;
                        }
                    }
                    filter += 3;
                } else {
                    if (casesensitive) {
                        if (*filter == *name) {
                            found = qtrue;
                        }
                    } else {
                        if (toupper(*filter) == toupper(*name)) {
                            found = qtrue;
                        }
                    }
                    filter++;
                }
            }
            if (!found) {
                return qfalse;
            }
            while (*filter) {
                if (*filter == ']' && *(filter + 1) != ']') {
                    break;
                }
                filter++;
            }
            filter++;
            name++;
        } else {
            if (casesensitive) {
                if (*filter != *name) {
                    return qfalse;
                }
            } else {
                if (toupper(*filter) != toupper(*name)) {
                    return qfalse;
                }
            }
            filter++;
            name++;
        }
    }

    return qtrue;
}

static unsigned ShaderParser_CountMatchingFiles(const char *directory, const char *extension, shader_parser_string_list_t *aggregate) {
    DIR *dir = opendir(directory);
    if (!dir) {
        return 0;
    }

    struct dirent *entry;
    unsigned added = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        size_t len = strlen(entry->d_name);
        size_t extLen = extension ? strlen(extension) : 0;
        if (extension && (len < extLen || Q_stricmp(entry->d_name + len - extLen, extension))) {
            continue;
        }

        qboolean duplicate = qfalse;
        for (size_t i = 0; i < aggregate->count; ++i) {
            if (!Q_stricmp(aggregate->items[i], entry->d_name)) {
                duplicate = qtrue;
                break;
            }
        }

        if (duplicate) {
            continue;
        }

        char **next = realloc(aggregate->items, sizeof(char *) * (aggregate->count + 1));
        if (!next) {
            continue;
        }

        aggregate->items = next;
        aggregate->items[aggregate->count] = strdup(entry->d_name);
        if (!aggregate->items[aggregate->count]) {
            continue;
        }

        aggregate->count++;
        added++;
    }

    closedir(dir);
    return added;
}

static int QDECL ShaderParser_StringSortCompare(const void *lhs, const void *rhs) {
    const char *const *left = lhs;
    const char *const *right = rhs;
    return Q_stricmp(*left, *right);
}

static char **ShaderParser_ListScriptFiles(const char *path, const char *extension, int *numFiles) {
    if (!path || !extension || !numFiles) {
        return NULL;
    }

    shader_parser_string_list_t aggregate = {0};

    for (size_t i = 0; i < gShaderParserConfig.scriptDirs.count; ++i) {
        ShaderParser_CountMatchingFiles(gShaderParserConfig.scriptDirs.items[i], extension, &aggregate);
    }

    *numFiles = (int)aggregate.count;
    gShaderParserStats.scriptFileCount = aggregate.count;

    if (aggregate.count == 0) {
        ShaderParser_FreeStringList(&aggregate);
        return NULL;
    }

    qsort(aggregate.items, aggregate.count, sizeof(char *), ShaderParser_StringSortCompare);

    char **result = calloc(aggregate.count + 1, sizeof(char *));
    if (!result) {
        ShaderParser_FreeStringList(&aggregate);
        return NULL;
    }

    for (size_t i = 0; i < aggregate.count; ++i) {
        result[i] = aggregate.items[i];
    }

    free(aggregate.items);
    return result;
}

static void ShaderParser_FreeListedFiles(char **list) {
    if (!list) {
        return;
    }

    for (size_t i = 0; list[i]; ++i) {
        free(list[i]);
    }

    free(list);
}

static qboolean ShaderParser_BuildRealPath(const char *dir, const char *relative, char *buffer, size_t bufferSize) {
    size_t required = strlen(dir) + 1 + strlen(relative) + 1;
    if (required > bufferSize) {
        return qfalse;
    }

    Com_sprintf(buffer, bufferSize, "%s/%s", dir, relative);
    return qtrue;
}

static FILE *ShaderParser_OpenScriptFile(const char *relative, long *outSize, char *fullPath, size_t fullPathSize) {
    for (size_t i = 0; i < gShaderParserConfig.scriptDirs.count; ++i) {
        if (!ShaderParser_BuildRealPath(gShaderParserConfig.scriptDirs.items[i], relative, fullPath, fullPathSize)) {
            continue;
        }

        FILE *handle = fopen(fullPath, "rb");
        if (!handle) {
            continue;
        }

        if (fseek(handle, 0, SEEK_END) != 0) {
            fclose(handle);
            continue;
        }

        long size = ftell(handle);
        if (size < 0) {
            fclose(handle);
            continue;
        }

        rewind(handle);

        if (outSize) {
            *outSize = size;
        }

        return handle;
    }

    return NULL;
}

static void QDECL ShaderParser_RI_Printf(int printLevel, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (printLevel == PRINT_DEVELOPER && !gShaderParserConfig.verbose) {
        va_end(args);
        return;
    }

    FILE *target = (printLevel == PRINT_WARNING || printLevel == PRINT_ERROR) ? stderr : stdout;
    vfprintf(target, fmt, args);
    va_end(args);
}

static Q_NO_RETURN void QDECL ShaderParser_RI_Error(int code, const char *fmt, ...) {
    (void)code;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Renderer error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

static int ShaderParser_RI_Milliseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

static int ShaderParser_RI_CmdArgc(void) {
    return 0;
}

static char *ShaderParser_RI_CmdArgv(int arg) {
    (void)arg;
    static char empty[] = "";
    return empty;
}

static long ShaderParser_RI_FS_ReadFile(const char *name, void **outBuffer) {
    if (!name) {
        return -1;
    }

    char relative[MAX_QPATH];
    if (!ShaderParser_ResolveScriptsPath(name, relative, sizeof(relative))) {
        return -1;
    }

    char fullPath[PATH_MAX];
    long fileSize = 0;
    FILE *file = ShaderParser_OpenScriptFile(relative, &fileSize, fullPath, sizeof(fullPath));
    if (!file) {
        return -1;
    }

    if (!outBuffer) {
        fclose(file);
        return fileSize;
    }

    char *buffer = malloc((size_t)fileSize + 1);
    if (!buffer) {
        fclose(file);
        return -1;
    }

    size_t readSize = fread(buffer, 1, (size_t)fileSize, file);
    fclose(file);

    if (readSize != (size_t)fileSize) {
        return -1;
    }

    buffer[fileSize] = '\0';
    *outBuffer = buffer;

    char canonical[PATH_MAX];
    const char *recordPath = canonical;
    if (!realpath(fullPath, canonical)) {
        recordPath = fullPath;
    }
    ShaderParser_RecordLoadedFile(recordPath);

    return fileSize;
}

static void ShaderParser_RI_FS_FreeFile(void *buffer) {
    free(buffer);
}

static char **ShaderParser_RI_FS_ListFiles(const char *path, const char *extension, int *numFiles) {
    if (!path || Q_stricmp(path, "scripts") || gShaderParserConfig.scriptDirs.count == 0) {
        if (numFiles) {
            *numFiles = 0;
        }
        return NULL;
    }

    return ShaderParser_ListScriptFiles(path, extension, numFiles);
}

static void ShaderParser_RI_FS_FreeFileList(char **list) {
    ShaderParser_FreeListedFiles(list);
}

static int ShaderParser_RI_FS_FileIsInPAK(const char *name, int *checksum) {
    (void)name;
    (void)checksum;
    return 0;
}

static void ShaderParser_RI_FS_WriteFile(const char *path, const void *buffer, int size) {
    (void)path;
    (void)buffer;
    (void)size;
}

static qboolean ShaderParser_RI_FS_FileExists(const char *path) {
    (void)path;
    return qfalse;
}

image_t *R_FindImageFile(const char *name, imgType_t type, imgFlags_t flags) {
    (void)type;
    (void)flags;

    if (!name || !*name) {
        return ShaderParser_GetFallbackImage();
    }

    image_t *img = ShaderParser_CreateImage(name);
    if (!img) {
        return ShaderParser_GetFallbackImage();
    }

    return img;
}

void RB_StageIteratorGeneric(void) {}

void RB_StageIteratorSky(void) {}

void R_DecomposeSort(unsigned sort, int *entityNum, shader_t **shader,
        int *fogNum, int *dlightMap, int *pshadowMap) {
    (void)sort;

    if (entityNum) {
        *entityNum = 0;
    }
    if (shader) {
        *shader = NULL;
    }
    if (fogNum) {
        *fogNum = 0;
    }
    if (dlightMap) {
        *dlightMap = 0;
    }
    if (pshadowMap) {
        *pshadowMap = 0;
    }
}

void R_InitSkyTexCoords(float cloudLayerHeight) {
    (void)cloudLayerHeight;
}

static int ShaderParser_RI_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits) {
    (void)arg0;
    (void)xpos;
    (void)ypos;
    (void)width;
    (void)height;
    (void)bits;
    return -1;
}

static void *ShaderParser_RI_HunkAlloc(int size, ha_pref preference) {
    (void)preference;
    return ShaderParser_HunkAllocInternal(size);
}

void QDECL Com_Printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void QDECL Com_DPrintf(const char *fmt, ...) {
    if (!gShaderParserConfig.verbose) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void QDECL Com_Error(int code, const char *fmt, ...) {
    (void)code;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

void Sys_Error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

void ShaderParser_InitEnvironment(void) {
    if (gEnvironmentInitialized) {
        return;
    }

    Com_Memset(&tr, 0, sizeof(tr));
    Com_Memset(&glConfig, 0, sizeof(glConfig));
    glConfig.hardwareType = GLHW_GENERIC;

    ShaderParser_InitBackEndData();
    ShaderParser_InitStubImages();
    ShaderParser_ClearLoadedSources();
    ShaderParser_FreeHunkBlocks();

    memset(&ri, 0, sizeof(ri));
    ri.Printf = ShaderParser_RI_Printf;
    ri.Error = ShaderParser_RI_Error;
    ri.Milliseconds = ShaderParser_RI_Milliseconds;
    ri.Hunk_Alloc = ShaderParser_RI_HunkAlloc;
    ri.Cmd_Argc = ShaderParser_RI_CmdArgc;
    ri.Cmd_Argv = ShaderParser_RI_CmdArgv;
    ri.FS_ReadFile = ShaderParser_RI_FS_ReadFile;
    ri.FS_FreeFile = ShaderParser_RI_FS_FreeFile;
    ri.FS_ListFiles = ShaderParser_RI_FS_ListFiles;
    ri.FS_FreeFileList = ShaderParser_RI_FS_FreeFileList;
    ri.FS_FileIsInPAK = ShaderParser_RI_FS_FileIsInPAK;
    ri.FS_WriteFile = ShaderParser_RI_FS_WriteFile;
    ri.FS_FileExists = ShaderParser_RI_FS_FileExists;
    ri.CIN_PlayCinematic = ShaderParser_RI_CIN_PlayCinematic;

    gShaderParserStats.scriptFileCount = 0;
    gShaderParserStats.loadedFileCount = 0;
    gEnvironmentInitialized = qtrue;
}

void ShaderParser_ShutdownEnvironment(void) {
    ShaderParser_FreeHunkBlocks();
    ShaderParser_ClearLoadedSources();
    ShaderParser_FreeImagePool();

    if (backEndData) {
        free(backEndData->polys);
        free(backEndData->polyVerts);
        free(backEndData);
        backEndData = NULL;
    }

    gEnvironmentInitialized = qfalse;
}
