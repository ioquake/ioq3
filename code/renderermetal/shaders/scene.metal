#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

struct SceneUniforms {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4x4 inverseView;
    float4 viewOrigin;
    float4 clipInfo;
    float4 timeInfo;
};

struct StageFragmentParams {
    float alphaRef;
    float alphaFunc;
    float alphaTestEnabled;
    float texCoordSelector;
};

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float2 lightmapCoord [[attribute(2)]];
    float3 normal [[attribute(3)]];
    float4 color [[attribute(4)]];
};

struct SceneVSOut {
    float4 position [[position]];
    float2 texCoord;
    float2 lightmapCoord;
    float4 color;
    uint primitiveID [[flat]];
};

vertex SceneVSOut vertex_scene_basic(VertexIn in [[stage_in]],
                                      constant SceneUniforms& uniforms [[buffer(1)]],
                                      uint vid [[vertex_id]]) {
    SceneVSOut out;
    float4 worldPos = float4(in.position, 1.0);

    // Apply transformations in two steps: view then projection
    // This bypasses any potential matrix multiplication issues
    float4 viewPos = uniforms.view * worldPos;
    out.position = uniforms.projection * viewPos;

    out.texCoord = in.texCoord;
    out.lightmapCoord = in.lightmapCoord;
    out.color = in.color;  // Already normalized by MTL::VertexFormatUChar4Normalized
    out.primitiveID = vid / 3;  // Triangle ID
    return out;
}

// Generate a distinct color for each surface ID
float3 hashColor(uint id) {
    // Simple hash to generate pseudo-random but consistent colors
    uint h = id * 2654435761u;
    float r = float((h >> 0) & 0xFFu) / 255.0;
    float g = float((h >> 8) & 0xFFu) / 255.0;
    float b = float((h >> 16) & 0xFFu) / 255.0;
    return float3(r, g, b);
}

fragment float4 fragment_scene_basic(SceneVSOut in [[stage_in]],
                                      texture2d<float> tex [[texture(0)]],
                                      sampler samp [[sampler(0)]],
                                      constant StageFragmentParams& stage [[buffer(0)]]) {
    const float2 uv = (stage.texCoordSelector > 0.5f) ? in.lightmapCoord : in.texCoord;
    float4 color = tex.sample(samp, uv) * in.color;
    if (stage.alphaTestEnabled > 0.5f) {
        const float alpha = color.a;
        const int func = int(stage.alphaFunc + 0.5f);
        bool keep = true;
        switch (func) {
            case 1:
                keep = alpha > stage.alphaRef;
                break;
            case 2:
                keep = alpha < stage.alphaRef;
                break;
            case 3:
                keep = alpha >= stage.alphaRef;
                break;
            default:
                break;
        }
        if (!keep) {
            discard_fragment();
        }
    }
    return color;
}
