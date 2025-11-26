#include "shader_parser_metal.h"

#include "../../renderermetal/tr_shader.h"

#include <cstdio>
#include <string>

static void ShaderParser_WriteJsonString(FILE *stream, const char *value) {
    fputc('"', stream);
    if (value) {
        for (const char *cursor = value; *cursor; ++cursor) {
            const unsigned char ch = static_cast<unsigned char>(*cursor);
            if (ch == '"' || ch == '\\') {
                fputc('\\', stream);
                fputc(static_cast<int>(ch), stream);
            } else if (ch == '\n') {
                fputs("\\n", stream);
            } else if (ch == '\r') {
                fputs("\\r", stream);
            } else if (ch == '\t') {
                fputs("\\t", stream);
            } else if (ch < 0x20) {
                fprintf(stream, "\\u%04x", ch);
            } else {
                fputc(static_cast<int>(ch), stream);
            }
        }
    }
    fputc('"', stream);
}

static const char *MetalBlendModeToString(MetalShaderBlendMode mode) {
    switch (mode) {
        case MetalShaderBlendMode::Opaque:
            return "opaque";
        case MetalShaderBlendMode::Alpha:
            return "alpha";
        case MetalShaderBlendMode::Additive:
            return "additive";
        case MetalShaderBlendMode::Filter:
            return "filter";
    }
    return "opaque";
}

static const char *MetalBlendFactorToString(MetalBlendFactor factor) {
    switch (factor) {
        case MetalBlendFactor::Zero:
            return "GL_ZERO";
        case MetalBlendFactor::One:
            return "GL_ONE";
        case MetalBlendFactor::SrcColor:
            return "GL_SRC_COLOR";
        case MetalBlendFactor::OneMinusSrcColor:
            return "GL_ONE_MINUS_SRC_COLOR";
        case MetalBlendFactor::DstColor:
            return "GL_DST_COLOR";
        case MetalBlendFactor::OneMinusDstColor:
            return "GL_ONE_MINUS_DST_COLOR";
        case MetalBlendFactor::SrcAlpha:
            return "GL_SRC_ALPHA";
        case MetalBlendFactor::OneMinusSrcAlpha:
            return "GL_ONE_MINUS_SRC_ALPHA";
        case MetalBlendFactor::DstAlpha:
            return "GL_DST_ALPHA";
        case MetalBlendFactor::OneMinusDstAlpha:
            return "GL_ONE_MINUS_DST_ALPHA";
        case MetalBlendFactor::Unknown:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

static const char *MetalTCGenToString(MetalTCGen type) {
    switch (type) {
        case MetalTCGen::Texture:
            return "texture";
        case MetalTCGen::Lightmap:
            return "lightmap";
        case MetalTCGen::Environment:
            return "environment";
        case MetalTCGen::Vector:
            return "vector";
        case MetalTCGen::Fog:
            return "fog";
        case MetalTCGen::Identity:
            return "identity";
    }
    return "texture";
}

static const char *MetalRGBGenToString(MetalRGBGen type) {
    switch (type) {
        case MetalRGBGen::Identity:
            return "identity";
        case MetalRGBGen::IdentityLighting:
            return "identityLighting";
        case MetalRGBGen::Entity:
            return "entity";
        case MetalRGBGen::OneMinusEntity:
            return "oneMinusEntity";
        case MetalRGBGen::Vertex:
            return "vertex";
        case MetalRGBGen::OneMinusVertex:
            return "oneMinusVertex";
        case MetalRGBGen::Wave:
            return "wave";
        case MetalRGBGen::LightingDiffuse:
            return "lightingDiffuse";
        case MetalRGBGen::LightingSpecular:
            return "lightingSpecular";
        case MetalRGBGen::Const:
            return "const";
        case MetalRGBGen::Bad:
            return "bad";
    }
    return "identity";
}

static const char *MetalAlphaGenToString(MetalAlphaGen type) {
    switch (type) {
        case MetalAlphaGen::Identity:
            return "identity";
        case MetalAlphaGen::Skip:
            return "skip";
        case MetalAlphaGen::Entity:
            return "entity";
        case MetalAlphaGen::OneMinusEntity:
            return "oneMinusEntity";
        case MetalAlphaGen::Vertex:
            return "vertex";
        case MetalAlphaGen::OneMinusVertex:
            return "oneMinusVertex";
        case MetalAlphaGen::LightingSpecular:
            return "lightingSpecular";
        case MetalAlphaGen::Wave:
            return "wave";
        case MetalAlphaGen::Portal:
            return "portal";
        case MetalAlphaGen::Const:
            return "const";
    }
    return "identity";
}

static void ShaderParser_WriteStageJson(FILE *stream, const MetalShaderStageInfo &stage, int indent) {
    fprintf(stream, "%*s{\n", indent, "");
    fprintf(stream, "%*s\"blendMode\": \"%s\",\n", indent + 2, "", MetalBlendModeToString(stage.blendMode));
    fprintf(stream, "%*s\"srcBlend\": \"%s\",\n", indent + 2, "", MetalBlendFactorToString(stage.srcBlendFactor));
    fprintf(stream, "%*s\"dstBlend\": \"%s\",\n", indent + 2, "", MetalBlendFactorToString(stage.dstBlendFactor));
    fprintf(stream, "%*s\"depthWrite\": %s,\n", indent + 2, "", stage.depthWrite ? "true" : "false");
    fprintf(stream, "%*s\"alphaFunc\": %d,\n", indent + 2, "", stage.alphaFunc);
    fprintf(stream, "%*s\"tcGen\": \"%s\",\n", indent + 2, "", MetalTCGenToString(stage.tcGen.type));
    fprintf(stream, "%*s\"rgbGen\": \"%s\",\n", indent + 2, "", MetalRGBGenToString(stage.rgbGen.type));
    fprintf(stream, "%*s\"alphaGen\": \"%s\",\n", indent + 2, "", MetalAlphaGenToString(stage.alphaGen.type));

    fprintf(stream, "%*s\"images\": [", indent + 2, "");
    for (size_t i = 0; i < stage.imagePaths.size(); ++i) {
        if (i > 0) {
            fprintf(stream, ", ");
        }
        ShaderParser_WriteJsonString(stream, stage.imagePaths[i].c_str());
    }
    fprintf(stream, "],\n");

    fprintf(stream, "%*s\"tcMods\": %zu,\n", indent + 2, "", stage.tcMods.size());
    fprintf(stream, "%*s\"usesLightmap\": %s,\n", indent + 2, "",
            stage.usesLightmap ? "true" : "false");
    fprintf(stream, "%*s\"usesWhiteImage\": %s\n", indent + 2, "",
            stage.usesWhiteImage ? "true" : "false");
    fprintf(stream, "%*s}", indent, "");
}

qboolean ShaderParser_WriteJsonMetal(const shader_parser_string_list_t *names, FILE *stream) {
    if (!names || !stream) {
        return qfalse;
    }

    fprintf(stream, "{\n");
    fprintf(stream, "  \"backend\": \"metal\",\n");
    fprintf(stream, "  \"stats\": {\n");
    fprintf(stream, "    \"scriptFiles\": %zu,\n", gShaderParserStats.scriptFileCount);
    fprintf(stream, "    \"loadedFiles\": %zu\n", gShaderParserStats.loadedFileCount);
    fprintf(stream, "  },\n");
    fprintf(stream, "  \"shaders\": [\n");

    size_t emitted = 0;
    for (size_t i = 0; i < names->count; ++i) {
        const char *name = names->items[i];
        if (!name || !ShaderParser_NameMatchesFilters(name)) {
            continue;
        }

        MetalShaderScriptInfo info;
        if (!MetalShaderScriptGetInfo(std::string(name), info)) {
            continue;
        }

        if (emitted > 0) {
            fprintf(stream, ",\n");
        }

        fprintf(stream, "    {\n");
        fprintf(stream, "      \"name\": ");
        ShaderParser_WriteJsonString(stream, name);
        fprintf(stream, ",\n");
        fprintf(stream, "      \"forceOpaque\": %s,\n", info.forceOpaque ? "true" : "false");
        fprintf(stream, "      \"alphaFunc\": %d,\n", info.alphaFunc);
        fprintf(stream, "      \"fog\": {\n");
        fprintf(stream, "        \"enabled\": %s,\n", info.hasFogParms ? "true" : "false");
        fprintf(stream, "        \"color\": [%.6f, %.6f, %.6f],\n",
                info.fogColor[0], info.fogColor[1], info.fogColor[2]);
        fprintf(stream, "        \"depthForOpaque\": %.6f\n", info.fogDepthForOpaque);
        fprintf(stream, "      },\n");

        fprintf(stream, "      \"stageCount\": %zu,\n", info.stages.size());
        fprintf(stream, "      \"stages\": [\n");
        for (size_t stageIndex = 0; stageIndex < info.stages.size(); ++stageIndex) {
            if (stageIndex > 0) {
                fprintf(stream, ",\n");
            }
            ShaderParser_WriteStageJson(stream, info.stages[stageIndex], 8);
        }
        if (!info.stages.empty()) {
            fprintf(stream, "\n      ");
        }
        fprintf(stream, "]\n");
        fprintf(stream, "    }");

        emitted++;
    }

    if (emitted > 0) {
        fprintf(stream, "\n");
    }
    fprintf(stream, "  ]\n");
    fprintf(stream, "}\n");
    return qtrue;
}
