#ifndef TR_SHADER_METAL_H
#define TR_SHADER_METAL_H

#include <array>
#include <string>
#include <vector>

enum class MetalWaveFunc {
    None = 0,
    Sin,
    Triangle,
    Square,
    Sawtooth,
    InverseSawtooth,
    Noise
};

struct MetalWaveForm {
    MetalWaveFunc func = MetalWaveFunc::None;
    float base = 0.0f;
    float amplitude = 0.0f;
    float phase = 0.0f;
    float frequency = 0.0f;
};

enum class MetalRGBGen {
    Identity = 0,
    IdentityLighting,
    Entity,
    OneMinusEntity,
    Vertex,
    OneMinusVertex,
    Wave,
    LightingDiffuse,
    LightingSpecular,
    Const,
    Bad
};

struct MetalRGBGenConfig {
    MetalRGBGen type = MetalRGBGen::Identity;
    MetalWaveForm wave;
    std::array<float, 4> constColor = {1.0f, 1.0f, 1.0f, 1.0f};
};

enum class MetalAlphaGen {
    Identity = 0,
    Skip,
    Entity,
    OneMinusEntity,
    Vertex,
    OneMinusVertex,
    LightingSpecular,
    Wave,
    Portal,
    Const
};

struct MetalAlphaGenConfig {
    MetalAlphaGen type = MetalAlphaGen::Identity;
    MetalWaveForm wave;
    float portalRange = 0.0f;
    float constAlpha = 1.0f;
};

enum class MetalTCGen {
    Texture = 0,
    Lightmap,
    Environment,
    Vector,
    Fog,
    Identity
};

struct MetalTCGenConfig {
    MetalTCGen type = MetalTCGen::Texture;
    std::array<float, 3> sVector = {1.0f, 0.0f, 0.0f};
    std::array<float, 3> tVector = {0.0f, 1.0f, 0.0f};
};

enum class MetalTCModType {
    None = 0,
    Scroll,
    Scale,
    Rotate,
    Stretch,
    Turbulence,
    Transform,
    EntityTranslate
};

struct MetalTCMod {
    MetalTCModType type = MetalTCModType::None;
    MetalWaveForm wave;
    std::array<float, 6> args = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

enum class MetalShaderBlendMode {
    Opaque = 0,
    Alpha,
    Additive,
    Filter
};

enum class MetalBlendFactor {
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    Unknown
};

enum class MetalAdjustColorsForFog {
    None = 0,
    ModulateRGB,
    ModulateAlpha,
    ModulateRGBA
};

inline MetalAdjustColorsForFog MetalClassifyFogAdjustment(MetalBlendFactor src, MetalBlendFactor dst) {
    if ((src == MetalBlendFactor::One && dst == MetalBlendFactor::One) ||
        (src == MetalBlendFactor::Zero && dst == MetalBlendFactor::OneMinusSrcColor)) {
        return MetalAdjustColorsForFog::ModulateRGB;
    }
    if (src == MetalBlendFactor::SrcAlpha && dst == MetalBlendFactor::OneMinusSrcAlpha) {
        return MetalAdjustColorsForFog::ModulateAlpha;
    }
    if (src == MetalBlendFactor::One && dst == MetalBlendFactor::OneMinusSrcAlpha) {
        return MetalAdjustColorsForFog::ModulateRGBA;
    }
    return MetalAdjustColorsForFog::None;
}

struct MetalShaderStageInfo {
    std::vector<std::string> imagePaths;
    MetalShaderBlendMode blendMode = MetalShaderBlendMode::Opaque;
    MetalBlendFactor srcBlendFactor = MetalBlendFactor::One;
    MetalBlendFactor dstBlendFactor = MetalBlendFactor::Zero;
    bool blendFuncExplicit = false;
    bool depthWrite = true;
    bool depthWriteExplicit = false;
    int alphaFunc = 0;
    bool usesLightmap = false;
    bool usesWhiteImage = false;
    bool isStandaloneLightmapPass = false;
    bool clampMap = false;  // Use clamp-to-edge instead of repeat
    float animFrequency = 0.0f;
    MetalRGBGenConfig rgbGen;
    MetalAlphaGenConfig alphaGen;
    MetalTCGenConfig tcGen;
    std::vector<MetalTCMod> tcMods;
    bool hasGlow = false;
    MetalAdjustColorsForFog adjustColorsForFog = MetalAdjustColorsForFog::None;
};

enum class MetalDeformType {
    None = 0,
    Wave,
    Bulge,
    Autosprite,
    Autosprite2,
    Unsupported
};

struct MetalDeformInfo {
    MetalDeformType type = MetalDeformType::None;
    MetalWaveForm wave;
    float spread = 0.0f;
    float bulgeWidth = 0.0f;
    float bulgeHeight = 0.0f;
    float bulgeSpeed = 0.0f;
    bool requiresCPU = false;
};

struct MetalShaderScriptInfo {
    std::vector<std::string> imagePaths;
    std::vector<int> imageStageIndices;
    bool forceOpaque = false;
    int alphaFunc = 0;
    bool hasFogParms = false;
    std::array<float, 3> fogColor = {0.0f, 0.0f, 0.0f};
    float fogDepthForOpaque = 0.0f;

    std::vector<MetalShaderBlendMode> stageBlendModes;
    std::vector<bool> stageDepthWrite;
    std::vector<bool> stageDepthWriteExplicit;
    std::vector<MetalShaderStageInfo> stages;
    MetalDeformInfo deform;
    bool hasDeform = false;

    bool isSky = false;
    float cloudHeight = 0.0f;
    std::array<std::string, 6> outerboxTextures;
    std::array<std::string, 6> innerboxTextures;
};

bool MetalShaderScriptLookup(const std::string &shaderName, std::string &outTexturePath);
bool MetalShaderScriptCollectImages(const std::string &shaderName, std::vector<std::string> &outImages, bool &outForceOpaque, int &outAlphaFunc);
bool MetalShaderScriptGetInfo(const std::string &shaderName, MetalShaderScriptInfo &outInfo);
void MetalShaderScriptClearCache();

extern bool g_metalShaderDebugDrawLogging;

#endif // TR_SHADER_METAL_H
