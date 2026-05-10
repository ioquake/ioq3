#include "tr_shader.h"
#include "tr_utils.h"

extern "C" {
#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_common.h"
}

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

bool g_metalShaderDebugDrawLogging = false;

namespace {

constexpr int kMaxShaderFiles = 4096;
constexpr bool kIgnoreDstAlpha = true;

static MetalBlendFactor AdjustBlendForDstAlpha(MetalBlendFactor factor) {
    if (!kIgnoreDstAlpha) {
        return factor;
    }

    if (factor == MetalBlendFactor::DstAlpha) {
        return MetalBlendFactor::One;
    }
    if (factor == MetalBlendFactor::OneMinusDstAlpha) {
        return MetalBlendFactor::Zero;
    }
    return factor;
}

struct ShaderRecord {
    MetalShaderScriptInfo info;
};

MetalShaderBlendMode MetalClassifyBlendMode(MetalBlendFactor src, MetalBlendFactor dst);

class TokenStream {
public:
    explicit TokenStream(char *buffer) : cursor_(buffer) {}

    [[nodiscard]] bool eof() const {
        return !cursor_ || !cursor_[0];
    }

    [[nodiscard]] std::string next(bool crossLine = true) {
        if (eof()) {
            return {};
        }

        char *token = COM_ParseExt(&cursor_, crossLine ? qtrue : qfalse);
        if (!token) {
            return {};
        }
        return std::string(token);
    }

    [[nodiscard]] char **rawCursor() { return &cursor_; }

private:
    char *cursor_ = nullptr;
};

struct StageBuilder {
    MetalShaderStageInfo info;
    bool rgbGenExplicit = false;
    bool alphaGenExplicit = false;
    bool tcGenExplicit = false;
    bool rgbGenExactVertex = false;
    bool hasUnsupportedAlphaMap = false;

    void setBlendFactors(MetalBlendFactor src, MetalBlendFactor dst) {
        src = AdjustBlendForDstAlpha(src);
        dst = AdjustBlendForDstAlpha(dst);
        info.srcBlendFactor = src;
        info.dstBlendFactor = dst;
        info.blendFuncExplicit = true;
        info.blendMode = MetalClassifyBlendMode(src, dst);
        info.adjustColorsForFog = MetalClassifyFogAdjustment(src, dst);

        if (!info.depthWriteExplicit) {
            const bool isOpaque = (src == MetalBlendFactor::One && dst == MetalBlendFactor::Zero);
            info.depthWrite = isOpaque;
        }
    }

    void finalizeDefaults() {
        if (!rgbGenExplicit) {
            // GL2 default: CGEN_IDENTITY_LIGHTING for opaque/additive stages,
            // CGEN_IDENTITY for filter-blend stages (ParseStage, tr_shader.c:1366-1373).
            // In Metal, IdentityLighting (white, no per-stage overbright) is the correct
            // equivalent for both cases:
            //   - Opaque stages: white × 1.0 matches GL2's (1/2^OBB) × hw_overbright = 1.0.
            //   - Filter stages: tr_backend already zeroes stageOverBright for DST_COLOR blends,
            //     so Identity or IdentityLighting both reduce to white × 1.0 here anyway.
            // Using Vertex was wrong: it applied BSP per-vertex lighting on top of the lightmap
            // texture stage, causing double-lighting on shader-defined lightmapped surfaces.
            info.rgbGen.type = MetalRGBGen::IdentityLighting;
        }

        if (!alphaGenExplicit) {
            info.alphaGen.type = MetalAlphaGen::Identity;
        }

        const bool rgbAllowsSkip =
            info.rgbGen.type == MetalRGBGen::Identity ||
            info.rgbGen.type == MetalRGBGen::LightingDiffuse;
        if (info.alphaGen.type == MetalAlphaGen::Identity && rgbAllowsSkip) {
            info.alphaGen.type = MetalAlphaGen::Skip;
        }

        if (info.usesLightmap && info.imagePaths.empty() && !tcGenExplicit &&
            info.tcGen.type == MetalTCGen::Texture) {
            info.tcGen.type = MetalTCGen::Lightmap;
        }

        if (hasUnsupportedAlphaMap) {
            info.srcBlendFactor = MetalBlendFactor::One;
            info.dstBlendFactor = MetalBlendFactor::Zero;
            info.blendFuncExplicit = false;
            info.blendMode = MetalShaderBlendMode::Opaque;
            info.depthWrite = false;
            info.depthWriteExplicit = false;
            info.rgbGen.type = MetalRGBGen::Bad;
            info.alphaGen.type = MetalAlphaGen::Identity;
        }
    }

    bool isStandaloneLightmapPass() const {
        const bool hasLightmapToken = info.usesLightmap && info.imagePaths.empty();
        const bool usesDefaultTCGen = !tcGenExplicit || info.tcGen.type == MetalTCGen::Texture;
        if (!hasLightmapToken || !usesDefaultTCGen || !info.tcMods.empty()) {
            return false;
        }

        if (info.blendFuncExplicit && info.blendMode != MetalShaderBlendMode::Filter) {
            return false;
        }

        return true;
    }

    [[nodiscard]] bool shouldAbortFurtherStages() const {
        return hasUnsupportedAlphaMap;
    }
};

static void skipRestOfLine(TokenStream &stream) {
    char **cursor = stream.rawCursor();
    if (!cursor || !*cursor) {
        return;
    }

    while (**cursor && **cursor != '\n' && **cursor != '\r') {
        ++(*cursor);
    }
    if (**cursor == '\n' || **cursor == '\r') {
        const char newlineChar = **cursor;
        ++(*cursor);
        if (newlineChar == '\r' && *cursor && **cursor == '\n') {
            ++(*cursor);
        }
    }
}

class ShaderBuilder {
public:
    explicit ShaderBuilder(std::string name) : name_(std::move(name)) {}

    void addStage(StageBuilder &&stage) {
        if (discardStages_ || abortRemainingStages_) {
            return;
        }
        ensurePlaceholderImages(stage.info);
        noteStageForCollapse(stage);

        if (info_.stages.empty()) {
            const bool stageBlends = stage.info.blendFuncExplicit &&
                stage.info.blendMode != MetalShaderBlendMode::Opaque;
            firstStageUsesBlend_ = stageBlends;
        }

        info_.stages.push_back(stage.info);
        info_.stageBlendModes.push_back(stage.info.blendMode);
        info_.stageDepthWrite.push_back(stage.info.depthWrite);
        info_.stageDepthWriteExplicit.push_back(stage.info.depthWriteExplicit);

        const size_t stageIndex = info_.stages.size() - 1;
        for (const auto &path : stage.info.imagePaths) {
            recordImage(path, stageIndex);
        }
        if (stage.info.usesLightmap) {
            recordImage("$lightmap", stageIndex);
        }
        if (stage.info.usesWhiteImage) {
            recordImage("$whiteimage", stageIndex);
        }

        if (info_.alphaFunc == 0 && stage.info.alphaFunc != 0) {
            info_.alphaFunc = stage.info.alphaFunc;
        }
    }

    [[nodiscard]] const std::string &name() const { return name_; }

    [[nodiscard]] MetalShaderScriptInfo build() {
        applyFogAdjustmentRules();
        if (discardStages_) {
            info_.stages.clear();
            info_.stageBlendModes.clear();
            info_.stageDepthWrite.clear();
            info_.stageDepthWriteExplicit.clear();
        }
        collapseStandaloneLightmapStagesIfAllowed();
        convertStandaloneLightmapsIfNeeded();
        convertOpaqueLightmapStagesToTexture();
        rebuildImageRecords();
        finalizeForceOpaque();
        return info_;
    }

    MetalShaderScriptInfo &mutableInfo() { return info_; }

    void registerDeform(const MetalDeformInfo &deform) {
        if (!info_.hasDeform) {
            info_.deform = deform;
        }
        info_.hasDeform = true;
        allowLightmapCollapse_ = false;
    }

    void markLegacySkyDirective() {
        discardStages_ = true;
        info_.isSky = true;
    }

    void markAbortRemainingStages() {
        abortRemainingStages_ = true;
    }

    [[nodiscard]] bool discardStages() const { return discardStages_; }

private:
    void ensurePlaceholderImages(MetalShaderStageInfo &stage) {
        if (!stage.imagePaths.empty()) {
            return;
        }

        if (stage.usesWhiteImage || stage.usesLightmap) {
            stage.imagePaths.emplace_back("<white>");
        }
    }

    static void applyLeadingLightmapState(const MetalShaderStageInfo &lightmapStage,
                                          MetalShaderStageInfo &targetStage) {
        targetStage.srcBlendFactor = lightmapStage.srcBlendFactor;
        targetStage.dstBlendFactor = lightmapStage.dstBlendFactor;
        targetStage.blendMode = lightmapStage.blendMode;
        targetStage.blendFuncExplicit = lightmapStage.blendFuncExplicit;
        targetStage.depthWrite = lightmapStage.depthWrite;
        targetStage.depthWriteExplicit = lightmapStage.depthWriteExplicit;
        targetStage.alphaFunc = lightmapStage.alphaFunc;
        targetStage.adjustColorsForFog = lightmapStage.adjustColorsForFog;
    }

    static bool isSupportedTCGen(MetalTCGen type) {
        switch (type) {
            case MetalTCGen::Texture:
            case MetalTCGen::Lightmap:
            case MetalTCGen::Environment:
            case MetalTCGen::Vector:
                return true;
            default:
                return false;
        }
    }

    static bool isFilterBlend(const MetalShaderStageInfo &stage) {
        const bool dstColorZero = stage.srcBlendFactor == MetalBlendFactor::DstColor &&
                                  stage.dstBlendFactor == MetalBlendFactor::Zero;
        const bool zeroDstColor = stage.srcBlendFactor == MetalBlendFactor::Zero &&
                                  stage.dstBlendFactor == MetalBlendFactor::DstColor;
        return dstColorZero || zeroDstColor;
    }

    static bool isEligibleLightmapCollapseTarget(const MetalShaderStageInfo &stage) {
        if (stage.isStandaloneLightmapPass) {
            return false;
        }
        return stage.blendMode == MetalShaderBlendMode::Opaque ||
               stage.blendMode == MetalShaderBlendMode::Filter;
    }

    void noteStageForCollapse(const StageBuilder &stage) {
        if (!allowLightmapCollapse_) {
            return;
        }

        const MetalShaderStageInfo &info = stage.info;

        if (!info.isStandaloneLightmapPass && !seenNonStandaloneStage_) {
            if (pendingLeadingStandaloneStages_ > 0 && !isEligibleLightmapCollapseTarget(info)) {
                allowLightmapCollapse_ = false;
                pendingLeadingStandaloneStages_ = 0;
                return;
            }
            pendingLeadingStandaloneStages_ = 0;
            seenNonStandaloneStage_ = true;
        }

        if (!isSupportedTCGen(info.tcGen.type)) {
            allowLightmapCollapse_ = false;
            return;
        }

        if (info.alphaGen.type == MetalAlphaGen::LightingSpecular ||
            info.alphaGen.type == MetalAlphaGen::Portal) {
            allowLightmapCollapse_ = false;
            return;
        }

        if (info.isStandaloneLightmapPass) {
            if (!seenNonStandaloneStage_) {
                ++pendingLeadingStandaloneStages_;
            }
        }

    }

    void applyFogAdjustmentRules() {
        if (info_.stages.empty()) {
            return;
        }

        const bool enableFogAdjustments = firstStageUsesBlend_;
        bool hasFogAdjustedStage = false;

        for (auto &stage : info_.stages) {
            const bool stageBlends = stage.blendFuncExplicit &&
                stage.blendMode != MetalShaderBlendMode::Opaque;
            if (!enableFogAdjustments || !stageBlends) {
                stage.adjustColorsForFog = MetalAdjustColorsForFog::None;
                continue;
            }

            if (stage.adjustColorsForFog != MetalAdjustColorsForFog::None) {
                hasFogAdjustedStage = true;
            }
        }

        if (hasFogAdjustedStage) {
            allowLightmapCollapse_ = false;
        }
    }

    bool shouldCollapseTrailingStandaloneStages() const {
        if (info_.hasDeform) {
            return false;
        }

        const bool hasEmbeddedLightmap = std::any_of(
            info_.stages.begin(), info_.stages.end(), [](const MetalShaderStageInfo &stage) {
                return stage.usesLightmap && !stage.isStandaloneLightmapPass &&
                       stage.tcGen.type == MetalTCGen::Texture;
            });
        if (hasEmbeddedLightmap) {
            return false;
        }

        for (const auto &stage : info_.stages) {
            if (!isSupportedTCGen(stage.tcGen.type)) {
                return false;
            }
            if (stage.adjustColorsForFog != MetalAdjustColorsForFog::None) {
                return false;
            }
            if (stage.alphaGen.type == MetalAlphaGen::LightingSpecular ||
                stage.alphaGen.type == MetalAlphaGen::Portal) {
                return false;
            }
            if (stage.tcGen.type == MetalTCGen::Lightmap && !isFilterBlend(stage)) {
                return false;
            }
        }

        return true;
    }

    void collapseStandaloneLightmapStagesIfAllowed() {
        if (!allowLightmapCollapse_ || pendingLeadingStandaloneStages_ > 0) {
            return;
        }

        const size_t stageCount = info_.stages.size();
        if (stageCount == 0) {
            return;
        }

        std::vector<size_t> standaloneIndices;
        standaloneIndices.reserve(stageCount);
        for (size_t i = 0; i < stageCount; ++i) {
            if (info_.stages[i].isStandaloneLightmapPass) {
                standaloneIndices.push_back(i);
            }
        }

        if (standaloneIndices.empty()) {
            return;
        }

        size_t lastNonStandaloneIndex = stageCount;
        for (size_t idx = stageCount; idx-- > 0;) {
            if (!info_.stages[idx].isStandaloneLightmapPass) {
                lastNonStandaloneIndex = idx;
                break;
            }
        }
        if (lastNonStandaloneIndex == stageCount) {
            return;
        }

        const size_t firstStandaloneIndex = standaloneIndices.front();
        size_t firstNonStandaloneIndex = 0;
        while (firstNonStandaloneIndex < stageCount &&
               info_.stages[firstNonStandaloneIndex].isStandaloneLightmapPass) {
            ++firstNonStandaloneIndex;
        }
        bool leadingLightmapInjected = false;
        if (firstNonStandaloneIndex < stageCount && firstStandaloneIndex < firstNonStandaloneIndex) {
            MetalShaderStageInfo &target = info_.stages[firstNonStandaloneIndex];
            applyLeadingLightmapState(info_.stages[firstStandaloneIndex], target);
            info_.stageBlendModes[firstNonStandaloneIndex] = target.blendMode;
            info_.stageDepthWrite[firstNonStandaloneIndex] = target.depthWrite;
            info_.stageDepthWriteExplicit[firstNonStandaloneIndex] = target.depthWriteExplicit;
            if (info_.alphaFunc == 0 && target.alphaFunc != 0) {
                info_.alphaFunc = target.alphaFunc;
            }
            target.usesLightmap = true;
            ensurePlaceholderImages(target);
            leadingLightmapInjected = true;
        }

        const bool hasTrailingStandalone =
            lastNonStandaloneIndex + 1 < stageCount &&
            std::any_of(standaloneIndices.begin(), standaloneIndices.end(), [&](size_t idx) {
                return idx > lastNonStandaloneIndex;
            });
        const bool collapseTrailingStandalone = hasTrailingStandalone &&
            shouldCollapseTrailingStandaloneStages();

        const auto shouldRemoveStandalone = [&](size_t index) {
            if (index < lastNonStandaloneIndex) {
                return true;
            }
            if (collapseTrailingStandalone && index > lastNonStandaloneIndex) {
                return true;
            }
            return false;
        };

        const auto hasFutureRemovedStandalone = [&](size_t index) {
            for (size_t standaloneIdx : standaloneIndices) {
                if (standaloneIdx > index && shouldRemoveStandalone(standaloneIdx)) {
                    return true;
                }
            }
            return false;
        };

        bool usedLightmap = leadingLightmapInjected;
        for (size_t i = 0; i < stageCount; ++i) {
            MetalShaderStageInfo &stage = info_.stages[i];
            if (stage.isStandaloneLightmapPass) {
                continue;
            }
            if (!hasFutureRemovedStandalone(i)) {
                continue;
            }
            const bool filterBlend = isFilterBlend(stage);
            if (!usedLightmap || !filterBlend) {
                stage.usesLightmap = true;
                ensurePlaceholderImages(stage);
                usedLightmap = true;
            }
        }

        std::vector<MetalShaderStageInfo> collapsedStages;
        std::vector<MetalShaderBlendMode> collapsedBlendModes;
        std::vector<bool> collapsedDepthWrite;
        std::vector<bool> collapsedDepthWriteExplicit;
        collapsedStages.reserve(stageCount - standaloneIndices.size());
        collapsedBlendModes.reserve(stageCount - standaloneIndices.size());
        collapsedDepthWrite.reserve(stageCount - standaloneIndices.size());
        collapsedDepthWriteExplicit.reserve(stageCount - standaloneIndices.size());

        for (size_t i = 0; i < stageCount; ++i) {
            if (info_.stages[i].isStandaloneLightmapPass && shouldRemoveStandalone(i)) {
                continue;
            }

            collapsedStages.push_back(info_.stages[i]);
            collapsedBlendModes.push_back(collapsedStages.back().blendMode);
            collapsedDepthWrite.push_back(collapsedStages.back().depthWrite);
            collapsedDepthWriteExplicit.push_back(collapsedStages.back().depthWriteExplicit);
        }

        info_.stages.swap(collapsedStages);
        info_.stageBlendModes.swap(collapsedBlendModes);
        info_.stageDepthWrite.swap(collapsedDepthWrite);
        info_.stageDepthWriteExplicit.swap(collapsedDepthWriteExplicit);
    }

    void convertStandaloneLightmapsIfNeeded() {
        // Disabled to match OpenGL2 parity - GL2 doesn't convert tcGen for lightmap stages
        // This was causing lightmap stages to sample with diffuse texture coordinates
        // instead of lightmap coordinates, resulting in incorrect colors (orange tint)
        return;
        
        if (info_.hasDeform) {
            return;
        }

        for (auto &stage : info_.stages) {
            if (!stage.isStandaloneLightmapPass) {
                continue;
            }
            if (stage.adjustColorsForFog != MetalAdjustColorsForFog::None) {
                continue;
            }
            if (stage.tcGen.type != MetalTCGen::Lightmap) {
                continue;
            }

            const bool opaqueBlend = stage.blendMode == MetalShaderBlendMode::Opaque;
            const bool dstColorZero = stage.srcBlendFactor == MetalBlendFactor::DstColor &&
                                      stage.dstBlendFactor == MetalBlendFactor::Zero;
            const bool zeroSrcColor = stage.srcBlendFactor == MetalBlendFactor::Zero &&
                                      stage.dstBlendFactor == MetalBlendFactor::SrcColor;
            const bool filterBlend = stage.blendMode == MetalShaderBlendMode::Filter &&
                                     (dstColorZero || zeroSrcColor);
            if (!opaqueBlend && !filterBlend) {
                continue;
            }

            stage.tcGen.type = MetalTCGen::Texture;
        }
    }

    void convertOpaqueLightmapStagesToTexture() {
        for (auto &stage : info_.stages) {
            if (!stage.usesLightmap) {
                continue;
            }
            if (stage.tcGen.type != MetalTCGen::Lightmap) {
                continue;
            }
            if (stage.blendMode != MetalShaderBlendMode::Opaque) {
                continue;
            }
            if (stage.isStandaloneLightmapPass) {
                continue;
            }

            stage.tcGen.type = MetalTCGen::Texture;
        }
    }

    void rebuildImageRecords() {
        info_.imagePaths.clear();
        info_.imageStageIndices.clear();

        for (size_t stageIndex = 0; stageIndex < info_.stages.size(); ++stageIndex) {
            const MetalShaderStageInfo &stage = info_.stages[stageIndex];
            for (const auto &path : stage.imagePaths) {
                recordImage(path, stageIndex);
            }

            if (stage.usesLightmap) {
                recordImage("$lightmap", stageIndex);
            }
            if (stage.usesWhiteImage) {
                recordImage("$whiteimage", stageIndex);
            }
        }
    }

    void recordImage(const std::string &path, size_t stageIndex) {
        if (path.empty()) {
            return;
        }

        const std::string normalized = MetalNormalizeShaderName(path.c_str());
        if (normalized.empty()) {
            return;
        }

        auto it = std::find_if(info_.imagePaths.begin(), info_.imagePaths.end(),
                               [&](const std::string &candidate) {
                                   return !Q_stricmp(candidate.c_str(), normalized.c_str());
                               });
        if (it == info_.imagePaths.end()) {
            info_.imagePaths.push_back(normalized);
            info_.imageStageIndices.push_back(static_cast<int>(stageIndex));
        }
    }

    void finalizeForceOpaque() {
        if (info_.stages.empty()) {
            info_.forceOpaque = qtrue;
            return;
        }

        const bool hasTransparentStage = std::any_of(
            info_.stages.begin(), info_.stages.end(), [](const MetalShaderStageInfo &stage) {
                return stage.blendMode != MetalShaderBlendMode::Opaque || stage.alphaFunc != 0;
            });
        info_.forceOpaque = hasTransparentStage ? qfalse : qtrue;
    }

    std::string name_;
    MetalShaderScriptInfo info_;
    bool allowLightmapCollapse_ = false;  // Disabled to match OpenGL2 parity - GL2 doesn't collapse lightmap stages
    bool discardStages_ = false;
    bool abortRemainingStages_ = false;
    bool firstStageUsesBlend_ = false;
    bool seenNonStandaloneStage_ = false;
    int pendingLeadingStandaloneStages_ = 0;
};

struct ShaderCache {
    bool loaded = false;
    std::unordered_map<std::string, ShaderRecord> records;
} g_shaderCache;

// Shader remap table: normalized old-name → normalized new-name.
// Populated by MetalRemapShader; consulted by every public shader lookup.
static std::unordered_map<std::string, std::string> g_shaderRemapTable;

// Resolve a normalized shader key through the remap table (one level deep,
// matching GL2's single-redirect semantics).
static const std::string& resolveRemap(const std::string &key) {
    const auto it = g_shaderRemapTable.find(key);
    if (it != g_shaderRemapTable.end()) {
        return it->second;
    }
    return key;
}

// ------------------------------------------------------------
// Parsing helpers
// ------------------------------------------------------------

static bool parseFloat(TokenStream &stream, float &outValue, bool crossLine = false) {
    const std::string token = stream.next(crossLine);
    if (token.empty()) {
        return false;
    }
    outValue = static_cast<float>(atof(token.c_str()));
    return true;
}

static bool parseVector(TokenStream &stream, int count, float *values) {
    std::string token = stream.next(false);
    bool expectParen = false;
    if (token == "(") {
        expectParen = true;
    } else if (!token.empty()) {
        values[0] = static_cast<float>(atof(token.c_str()));
        for (int i = 1; i < count; ++i) {
            const std::string component = stream.next(false);
            if (component.empty()) {
                return false;
            }
            values[i] = static_cast<float>(atof(component.c_str()));
        }
        return true;
    } else {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        const std::string component = stream.next(false);
        if (component.empty()) {
            return false;
        }
        values[i] = static_cast<float>(atof(component.c_str()));
    }

    if (expectParen) {
        const std::string closing = stream.next(false);
        if (closing != ")") {
            return false;
        }
    }
    return true;
}

static MetalWaveFunc parseWaveFunc(const std::string &token) {
    if (token.empty()) {
        return MetalWaveFunc::None;
    }

    if (!Q_stricmp(token.c_str(), "sin")) {
        return MetalWaveFunc::Sin;
    }
    if (!Q_stricmp(token.c_str(), "triangle")) {
        return MetalWaveFunc::Triangle;
    }
    if (!Q_stricmp(token.c_str(), "square")) {
        return MetalWaveFunc::Square;
    }
    if (!Q_stricmp(token.c_str(), "sawtooth")) {
        return MetalWaveFunc::Sawtooth;
    }
    if (!Q_stricmp(token.c_str(), "inversesawtooth") || !Q_stricmp(token.c_str(), "inverseSawtooth")) {
        return MetalWaveFunc::InverseSawtooth;
    }
    if (!Q_stricmp(token.c_str(), "noise")) {
        return MetalWaveFunc::Noise;
    }
    return MetalWaveFunc::None;
}

static MetalWaveForm parseWaveForm(TokenStream &stream) {
    MetalWaveForm wave;
    const std::string funcToken = stream.next(false);
    wave.func = parseWaveFunc(funcToken);
    parseFloat(stream, wave.base, false);
    parseFloat(stream, wave.amplitude, false);
    parseFloat(stream, wave.phase, false);
    parseFloat(stream, wave.frequency, false);
    return wave;
}

static MetalBlendFactor parseBlendFactor(const std::string &token) {
    if (!Q_stricmp(token.c_str(), "GL_ZERO")) {
        return MetalBlendFactor::Zero;
    }
    if (!Q_stricmp(token.c_str(), "GL_ONE")) {
        return MetalBlendFactor::One;
    }
    if (!Q_stricmp(token.c_str(), "GL_SRC_COLOR")) {
        return MetalBlendFactor::SrcColor;
    }
    if (!Q_stricmp(token.c_str(), "GL_ONE_MINUS_SRC_COLOR")) {
        return MetalBlendFactor::OneMinusSrcColor;
    }
    if (!Q_stricmp(token.c_str(), "GL_DST_COLOR")) {
        return MetalBlendFactor::DstColor;
    }
    if (!Q_stricmp(token.c_str(), "GL_ONE_MINUS_DST_COLOR")) {
        return MetalBlendFactor::OneMinusDstColor;
    }
    if (!Q_stricmp(token.c_str(), "GL_SRC_ALPHA")) {
        return MetalBlendFactor::SrcAlpha;
    }
    if (!Q_stricmp(token.c_str(), "GL_ONE_MINUS_SRC_ALPHA")) {
        return MetalBlendFactor::OneMinusSrcAlpha;
    }
    if (!Q_stricmp(token.c_str(), "GL_DST_ALPHA")) {
        return MetalBlendFactor::DstAlpha;
    }
    if (!Q_stricmp(token.c_str(), "GL_ONE_MINUS_DST_ALPHA")) {
        return MetalBlendFactor::OneMinusDstAlpha;
    }
    return MetalBlendFactor::Unknown;
}

static MetalShaderBlendMode classifyBlendMode(MetalBlendFactor src, MetalBlendFactor dst) {
    if (src == MetalBlendFactor::One && dst == MetalBlendFactor::One) {
        return MetalShaderBlendMode::Additive;
    }
    if (src == MetalBlendFactor::DstColor && dst == MetalBlendFactor::Zero) {
        return MetalShaderBlendMode::Filter;
    }
    if (src == MetalBlendFactor::Zero && dst == MetalBlendFactor::SrcColor) {
        return MetalShaderBlendMode::Filter;
    }
    if (src == MetalBlendFactor::SrcAlpha && dst == MetalBlendFactor::OneMinusSrcAlpha) {
        return MetalShaderBlendMode::Alpha;
    }
    if (src == MetalBlendFactor::One && dst == MetalBlendFactor::Zero) {
        return MetalShaderBlendMode::Opaque;
    }
    return MetalShaderBlendMode::Alpha;
}

MetalShaderBlendMode MetalClassifyBlendMode(MetalBlendFactor src, MetalBlendFactor dst) {
    return classifyBlendMode(src, dst);
}

static int parseAlphaFunc(const std::string &token) {
    if (!Q_stricmp(token.c_str(), "GT0")) {
        return 1;
    }
    if (!Q_stricmp(token.c_str(), "LT128")) {
        return 2;
    }
    if (!Q_stricmp(token.c_str(), "GE128")) {
        return 3;
    }
    return 0;
}

static void parseImageToken(const std::string &token, StageBuilder &stage) {
    if (token.empty()) {
        return;
    }
    if (token == "$lightmap") {
        stage.info.usesLightmap = true;
        return;
    }
    if (token == "$whiteimage") {
        stage.info.usesWhiteImage = true;
        return;
    }
    stage.info.imagePaths.push_back(token);
}

static void parseAnimMap(TokenStream &stream, StageBuilder &stage) {
    float frequency = 0.0f;
    parseFloat(stream, frequency, false);
    stage.info.animFrequency = frequency;

    while (true) {
        std::string frame = stream.next(false);
        if (frame.empty()) {
            break;
        }
        parseImageToken(frame, stage);
    }
}

static void parseBlendFunc(TokenStream &stream, StageBuilder &stage) {
    std::string src = stream.next(false);
    if (src.empty()) {
        return;
    }

    std::string lower = src;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    MetalBlendFactor srcFactor = MetalBlendFactor::One;
    MetalBlendFactor dstFactor = MetalBlendFactor::Zero;

    if (lower == "add") {
        srcFactor = MetalBlendFactor::One;
        dstFactor = MetalBlendFactor::One;
    } else if (lower == "filter") {
        srcFactor = MetalBlendFactor::DstColor;
        dstFactor = MetalBlendFactor::Zero;
    } else if (lower == "blend") {
        srcFactor = MetalBlendFactor::SrcAlpha;
        dstFactor = MetalBlendFactor::OneMinusSrcAlpha;
    } else {
        std::string dst = stream.next(false);
        if (dst.empty()) {
            dst = "GL_ZERO";
        }
        srcFactor = parseBlendFactor(src);
        dstFactor = parseBlendFactor(dst);
    }

    stage.setBlendFactors(srcFactor, dstFactor);
}

static void parseRGBGen(TokenStream &stream, StageBuilder &stage) {
    const std::string mode = stream.next(false);
    if (mode.empty()) {
        return;
    }

    MetalRGBGenConfig config;
    if (!Q_stricmp(mode.c_str(), "identity")) {
        config.type = MetalRGBGen::Identity;
    } else if (!Q_stricmp(mode.c_str(), "identitylighting")) {
        config.type = MetalRGBGen::IdentityLighting;
    } else if (!Q_stricmp(mode.c_str(), "entity")) {
        config.type = MetalRGBGen::Entity;
    } else if (!Q_stricmp(mode.c_str(), "oneminusentity")) {
        config.type = MetalRGBGen::OneMinusEntity;
    } else if (!Q_stricmp(mode.c_str(), "vertex")) {
        config.type = MetalRGBGen::Vertex;
        stage.rgbGenExactVertex = false;
    } else if (!Q_stricmp(mode.c_str(), "exactvertex")) {
        config.type = MetalRGBGen::Vertex;
        stage.rgbGenExactVertex = true;
    } else if (!Q_stricmp(mode.c_str(), "oneminusvertex")) {
        config.type = MetalRGBGen::OneMinusVertex;
    } else if (!Q_stricmp(mode.c_str(), "lightingdiffuse")) {
        config.type = MetalRGBGen::LightingDiffuse;
    } else if (!Q_stricmp(mode.c_str(), "lightingspecular")) {
        config.type = MetalRGBGen::LightingSpecular;
    } else if (!Q_stricmp(mode.c_str(), "const")) {
        config.type = MetalRGBGen::Const;
        float vec[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        parseVector(stream, 4, vec);
        for (int i = 0; i < 4; ++i) {
            config.constColor[i] = vec[i];
        }
    } else if (!Q_stricmp(mode.c_str(), "wave")) {
        config.type = MetalRGBGen::Wave;
        config.wave = parseWaveForm(stream);
    }

    stage.info.rgbGen = config;
    stage.rgbGenExplicit = true;

    if (!stage.alphaGenExplicit && config.type == MetalRGBGen::Vertex && !stage.rgbGenExactVertex) {
        stage.info.alphaGen.type = MetalAlphaGen::Vertex;
        stage.alphaGenExplicit = true;
    }
}

static void parseAlphaGen(TokenStream &stream, StageBuilder &stage) {
    const std::string mode = stream.next(false);
    if (mode.empty()) {
        return;
    }

    MetalAlphaGenConfig config;
    if (!Q_stricmp(mode.c_str(), "identity")) {
        config.type = MetalAlphaGen::Identity;
    } else if (!Q_stricmp(mode.c_str(), "skip")) {
        config.type = MetalAlphaGen::Skip;
    } else if (!Q_stricmp(mode.c_str(), "entity")) {
        config.type = MetalAlphaGen::Entity;
    } else if (!Q_stricmp(mode.c_str(), "oneminusentity")) {
        config.type = MetalAlphaGen::OneMinusEntity;
    } else if (!Q_stricmp(mode.c_str(), "vertex")) {
        config.type = MetalAlphaGen::Vertex;
    } else if (!Q_stricmp(mode.c_str(), "oneminusvertex")) {
        config.type = MetalAlphaGen::OneMinusVertex;
    } else if (!Q_stricmp(mode.c_str(), "lightingspecular")) {
        config.type = MetalAlphaGen::LightingSpecular;
    } else if (!Q_stricmp(mode.c_str(), "portal")) {
        config.type = MetalAlphaGen::Portal;
        parseFloat(stream, config.portalRange, false);
    } else if (!Q_stricmp(mode.c_str(), "const")) {
        config.type = MetalAlphaGen::Const;
        parseFloat(stream, config.constAlpha, false);
    } else if (!Q_stricmp(mode.c_str(), "wave")) {
        config.type = MetalAlphaGen::Wave;
        config.wave = parseWaveForm(stream);
    }

        stage.info.alphaGen = config;
        stage.alphaGenExplicit = true;
}

static void parseTCGen(TokenStream &stream, StageBuilder &stage) {
    const std::string mode = stream.next(false);
    if (mode.empty()) {
        return;
    }

    stage.tcGenExplicit = true;

    if (!Q_stricmp(mode.c_str(), "environment")) {
        stage.info.tcGen.type = MetalTCGen::Environment;
    } else if (!Q_stricmp(mode.c_str(), "lightmap")) {
        stage.info.tcGen.type = MetalTCGen::Lightmap;
    } else if (!Q_stricmp(mode.c_str(), "fog")) {
        stage.info.tcGen.type = MetalTCGen::Fog;
    } else if (!Q_stricmp(mode.c_str(), "vector")) {
        stage.info.tcGen.type = MetalTCGen::Vector;
        parseVector(stream, 3, stage.info.tcGen.sVector.data());
        parseVector(stream, 3, stage.info.tcGen.tVector.data());
    } else {
        stage.info.tcGen.type = MetalTCGen::Texture;
    }
}

static void parseTCMod(TokenStream &stream, StageBuilder &stage) {
    const std::string mode = stream.next(false);
    if (mode.empty()) {
        return;
    }

    MetalTCMod mod;
    if (!Q_stricmp(mode.c_str(), "scale")) {
        mod.type = MetalTCModType::Scale;
        parseFloat(stream, mod.args[0], false);
        parseFloat(stream, mod.args[1], false);
    } else if (!Q_stricmp(mode.c_str(), "scroll")) {
        mod.type = MetalTCModType::Scroll;
        parseFloat(stream, mod.args[0], false);
        parseFloat(stream, mod.args[1], false);
    } else if (!Q_stricmp(mode.c_str(), "rotate")) {
        mod.type = MetalTCModType::Rotate;
        parseFloat(stream, mod.args[0], false);
    } else if (!Q_stricmp(mode.c_str(), "stretch")) {
        mod.type = MetalTCModType::Stretch;
        mod.wave = parseWaveForm(stream);
    } else if (!Q_stricmp(mode.c_str(), "turb") || !Q_stricmp(mode.c_str(), "turbulent")) {
        mod.type = MetalTCModType::Turbulence;
        parseFloat(stream, mod.args[0], false);
        parseFloat(stream, mod.args[1], false);
        parseFloat(stream, mod.args[2], false);
        parseFloat(stream, mod.args[3], false);
    } else if (!Q_stricmp(mode.c_str(), "transform")) {
        mod.type = MetalTCModType::Transform;
        for (float &value : mod.args) {
            parseFloat(stream, value, false);
        }
    } else if (!Q_stricmp(mode.c_str(), "entitytranslate")) {
        mod.type = MetalTCModType::EntityTranslate;
    } else {
        mod.type = MetalTCModType::None;
    }

    stage.info.tcMods.push_back(mod);
}

static void parseDeformVertexes(TokenStream &stream, ShaderBuilder &builder) {
    MetalDeformInfo deform;
    std::string subtype = stream.next(false);
    if (subtype.empty()) {
        builder.registerDeform(deform);
        return;
    }

    if (!Q_stricmp(subtype.c_str(), "bulge")) {
        deform.type = MetalDeformType::Bulge;
        parseFloat(stream, deform.bulgeWidth, false);
        parseFloat(stream, deform.bulgeHeight, false);
        parseFloat(stream, deform.bulgeSpeed, false);
        deform.requiresCPU = true;
    } else if (!Q_stricmp(subtype.c_str(), "wave")) {
        deform.type = MetalDeformType::Wave;
        float divisor = 0.0f;
        parseFloat(stream, divisor, false);
        deform.spread = divisor;
        deform.wave = parseWaveForm(stream);
        deform.requiresCPU = true;
    } else if (!Q_stricmp(subtype.c_str(), "autosprite")) {
        deform.type = MetalDeformType::Autosprite;
        deform.requiresCPU = true;
    } else if (!Q_stricmp(subtype.c_str(), "autosprite2")) {
        deform.type = MetalDeformType::Autosprite2;
        deform.requiresCPU = true;
    } else if (!Q_stricmp(subtype.c_str(), "projectionshadow")) {
        deform.type = MetalDeformType::Unsupported;
        deform.requiresCPU = true;
    } else if (!Q_stricmp(subtype.c_str(), "normal")) {
        deform.type = MetalDeformType::Unsupported;
        parseFloat(stream, deform.wave.amplitude, false);
        parseFloat(stream, deform.wave.frequency, false);
        deform.requiresCPU = true;
    } else if (!Q_stricmp(subtype.c_str(), "move")) {
        deform.type = MetalDeformType::Unsupported;
        float temp = 0.0f;
        for (int i = 0; i < 3; ++i) {
            parseFloat(stream, temp, false);
        }
        deform.wave = parseWaveForm(stream);
        deform.requiresCPU = true;
    } else if (!Q_stricmpn(subtype.c_str(), "text", 4)) {
        deform.type = MetalDeformType::Unsupported;
        deform.requiresCPU = true;
    } else {
        deform.type = MetalDeformType::Unsupported;
        deform.requiresCPU = true;
    }

    builder.registerDeform(deform);
}

static void parseSkyParms(TokenStream &stream, ShaderBuilder &builder) {
    auto &info = builder.mutableInfo();
    info.isSky = true;

    // skyparms <outerbox> <cloudheight> <innerbox>
    const std::string farBox = stream.next(false);      // outerbox (e.g., "env/xnight2")
    const std::string heightToken = stream.next(false); // cloudheight (number or "-")
    const std::string nearBox = stream.next(false);     // innerbox (e.g., "-" or another prefix)

    if (!heightToken.empty() && heightToken != "-") {
        info.cloudHeight = static_cast<float>(atof(heightToken.c_str()));
    }

    static const char *sides[6] = {"rt", "lf", "ft", "bk", "up", "dn"};

    for (size_t i = 0; i < 6; ++i) {
        if (!farBox.empty() && farBox != "-" && farBox != "none") {
            info.outerboxTextures[i] = MetalNormalizeShaderName((farBox + std::string("_") + sides[i]).c_str());
        }
        if (!nearBox.empty() && nearBox != "-" && nearBox != "none") {
            info.innerboxTextures[i] = MetalNormalizeShaderName((nearBox + std::string("_") + sides[i]).c_str());
        }
    }
}

static void parseFogParms(TokenStream &stream, ShaderBuilder &builder) {
    float color[3] = {0.0f, 0.0f, 0.0f};
    if (!parseVector(stream, 3, color)) {
        return;
    }

    float depth = 0.0f;
    parseFloat(stream, depth, false);

    auto &info = builder.mutableInfo();
    info.hasFogParms = true;
    info.fogColor = {color[0], color[1], color[2]};
    info.fogDepthForOpaque = depth;
}

    class ShaderFileParser {
    public:
        ShaderFileParser(const std::string &source, char *buffer)
            : sourceName_(source), stream_(buffer) {}

        void parse(std::unordered_map<std::string, ShaderRecord> &records) {
            while (!stream_.eof()) {
                const std::string shaderName = stream_.next(true);
                if (shaderName.empty()) {
                    break;
                }

                if (stream_.next(true) != "{") {
                    ri.Printf(PRINT_WARNING, "Metal: missing '{' for shader '%s' in %s\n",
                              shaderName.c_str(), sourceName_.c_str());
                    continue;
                }

                ShaderBuilder builder(shaderName);
                if (!parseShaderBody(builder)) {
                    continue;
                }

                const std::string normalizedName =
                    MetalNormalizeShaderName(shaderName.c_str());
                if (normalizedName.empty()) {
                    continue;
                }

                ShaderRecord record;
                record.info = builder.build();

                const auto insertResult = records.emplace(normalizedName, std::move(record));
                if (!insertResult.second && g_metalShaderDebugDrawLogging) {
                    ri.Printf(PRINT_DEVELOPER, "Metal: duplicate shader '%s' ignored (first definition wins)\n",
                              shaderName.c_str());
                }
            }
        }

    private:
        bool parseShaderBody(ShaderBuilder &builder) {
            while (true) {
                const std::string token = stream_.next(true);
                if (token.empty()) {
                    ri.Printf(PRINT_WARNING, "Metal: unexpected EOF in shader '%s'\n",
                              builder.name().c_str());
                    return false;
                }

                if (token == "}") {
                    return true;
                }

                if (token == "{") {
                    StageBuilder stage;
                    if (!parseStage(stage)) {
                        return false;
                    }
                    stage.finalizeDefaults();
                    stage.info.isStandaloneLightmapPass = stage.isStandaloneLightmapPass();
                    if (!builder.discardStages()) {
                        const bool abortAfterStage = stage.shouldAbortFurtherStages();
                        builder.addStage(std::move(stage));
                        if (abortAfterStage) {
                            builder.markAbortRemainingStages();
                        }
                    }
                    continue;
                }

                handleGeneralDirective(token, builder);
            }
        }

        bool parseStage(StageBuilder &stage) {
            while (true) {
                const std::string token = stream_.next(true);
                if (token.empty()) {
                    return false;
                }

                if (token == "}") {
                    return true;
                }

                handleStageDirective(token, stage);
            }
        }

        void handleGeneralDirective(const std::string &token, ShaderBuilder &builder) {
            if (!Q_stricmp(token.c_str(), "fogparms")) {
                parseFogParms(stream_, builder);
            } else if (!Q_stricmp(token.c_str(), "skyparms")) {
                parseSkyParms(stream_, builder);
            } else if (!Q_stricmp(token.c_str(), "sky")) {
                builder.markLegacySkyDirective();
                skipRestOfLine(stream_);
            } else if (!Q_stricmp(token.c_str(), "cloudparms")) {
                builder.markLegacySkyDirective();
                skipRestOfLine(stream_);
            } else if (!Q_stricmp(token.c_str(), "portal")) {
                // Portal shader - renders scene from portal's perspective
                builder.mutableInfo().isPortal = true;
            } else if (!Q_stricmp(token.c_str(), "surfaceparm")) {
                const std::string parm = stream_.next(false);
                if (!Q_stricmp(parm.c_str(), "fog")) {
                    builder.mutableInfo().hasFogParms = true;
                }
            } else if (!Q_stricmp(token.c_str(), "deformvertexes")) {
                parseDeformVertexes(stream_, builder);
            } else if (!Q_stricmp(token.c_str(), "cull")) {
                const std::string value = stream_.next(false);
                if (!Q_stricmp(value.c_str(), "twosided") || !Q_stricmp(value.c_str(), "none")) {
                    builder.mutableInfo().forceOpaque = qfalse;
                }
            }
        }

        void handleStageDirective(const std::string &token, StageBuilder &stage) {
            if (!Q_stricmp(token.c_str(), "map")) {
                parseImageToken(stream_.next(false), stage);
            } else if (!Q_stricmp(token.c_str(), "clampmap")) {
                parseImageToken(stream_.next(false), stage);
                stage.info.clampMap = true;
            } else if (!Q_stricmp(token.c_str(), "animmap")) {
                parseAnimMap(stream_, stage);
            } else if (!Q_stricmp(token.c_str(), "blendfunc")) {
                parseBlendFunc(stream_, stage);
            } else if (!Q_stricmp(token.c_str(), "rgbgen")) {
                parseRGBGen(stream_, stage);
            } else if (!Q_stricmp(token.c_str(), "alphagen")) {
                parseAlphaGen(stream_, stage);
            } else if (!Q_stricmp(token.c_str(), "tcgen")) {
                parseTCGen(stream_, stage);
            } else if (!Q_stricmp(token.c_str(), "tcmod")) {
                parseTCMod(stream_, stage);
            } else if (!Q_stricmp(token.c_str(), "depthwrite")) {
                stage.info.depthWrite = true;
                stage.info.depthWriteExplicit = true;
            } else if (!Q_stricmp(token.c_str(), "alphafunc")) {
                stage.info.alphaFunc = parseAlphaFunc(stream_.next(false));
            } else if (!Q_stricmp(token.c_str(), "glow")) {
                stage.info.hasGlow = true;
            } else if (!Q_stricmp(token.c_str(), "alphamap")) {
                stage.hasUnsupportedAlphaMap = true;
                (void)stream_.next(false);
            }
        }

        std::string sourceName_;
        TokenStream stream_;
    };

    static void ensureShadersLoaded() {
        if (g_shaderCache.loaded) {
            return;
        }
        g_shaderCache.loaded = true;

        int fileCount = 0;
        char **fileList = ri.FS_ListFiles("scripts", ".shader", &fileCount);
        if (!fileList) {
            return;
        }

        const int bounded = std::min(fileCount, kMaxShaderFiles);
        for (int i = 0; i < bounded; ++i) {
            const char *fileName = fileList[i];
            if (!fileName) {
                continue;
            }

            char virtualPath[MAX_QPATH];
            Com_sprintf(virtualPath, sizeof(virtualPath), "scripts/%s", fileName);

            char *buffer = nullptr;
            const int length = ri.FS_ReadFile(virtualPath, (void **)&buffer);
            if (length <= 0 || !buffer) {
                continue;
            }

            COM_Compress(buffer);

            ShaderFileParser parser(virtualPath, buffer);
            parser.parse(g_shaderCache.records);

            ri.FS_FreeFile(buffer);
        }

        ri.FS_FreeFileList(fileList);
    }

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool MetalShaderScriptLookup(const std::string &shaderName, std::string &outTexturePath) {
    ensureShadersLoaded();
    const std::string key = resolveRemap(MetalNormalizeShaderName(shaderName.c_str()));
    auto it = g_shaderCache.records.find(key);
    if (it == g_shaderCache.records.end()) {
        return false;
    }

    if (!it->second.info.imagePaths.empty()) {
        outTexturePath = it->second.info.imagePaths.front();
        return true;
    }

    return false;
}

bool MetalShaderScriptCollectImages(const std::string &shaderName,
                                    std::vector<std::string> &outImages,
                                    bool &outForceOpaque,
                                    int &outAlphaFunc) {
    ensureShadersLoaded();
    const std::string key = resolveRemap(MetalNormalizeShaderName(shaderName.c_str()));
    auto it = g_shaderCache.records.find(key);
    if (it == g_shaderCache.records.end()) {
        return false;
    }

    outImages = it->second.info.imagePaths;
    outForceOpaque = it->second.info.forceOpaque;
    outAlphaFunc = it->second.info.alphaFunc;
    return true;
}

bool MetalShaderScriptGetInfo(const std::string &shaderName, MetalShaderScriptInfo &outInfo) {
    ensureShadersLoaded();
    const std::string key = resolveRemap(MetalNormalizeShaderName(shaderName.c_str()));
    auto it = g_shaderCache.records.find(key);
    if (it == g_shaderCache.records.end()) {
        return false;
    }

    outInfo = it->second.info;
    return true;
}

void MetalShaderScriptClearCache() {
    g_shaderCache.records.clear();
    g_shaderCache.loaded = false;
}

// Mirrors GL2's R_RemapShader.  Maps every shader whose stripped name matches
// shaderName to resolve instead to newShaderName at lookup time.  If both
// names normalise to the same key the remap is removed (GL2 sets
// remappedShader = NULL in that case).  The timeOffset parameter shifts
// animation timing on the destination shader; it is stored for completeness
// even though the Metal renderer does not yet consume it at draw time.
void MetalRemapShader(const char *shaderName, const char *newShaderName,
                      const char *timeOffset) {
    if (!shaderName || !shaderName[0] || !newShaderName || !newShaderName[0]) {
        ri.Printf(PRINT_WARNING,
                  "WARNING: MetalRemapShader: invalid shader name(s)\n");
        return;
    }

    const std::string oldKey = MetalNormalizeShaderName(shaderName);
    const std::string newKey = MetalNormalizeShaderName(newShaderName);

    if (oldKey == newKey) {
        // Remove any existing remap (mirrors GL2 setting remappedShader = NULL).
        g_shaderRemapTable.erase(oldKey);
    } else {
        g_shaderRemapTable[oldKey] = newKey;
    }

    (void)timeOffset; // stored for API parity; not yet consumed at draw time
}
