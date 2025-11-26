//
// Cinematic Playback Shader
// Simple textured quad rendering for video playback
//

#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

//
// Vertex Shader: Pass-through with NDC coordinates
//
vertex VertexOut vertex_cinematic(uint vid [[vertex_id]],
                                   const device VertexIn* verts [[buffer(0)]]) {
    VertexOut out;
    VertexIn v = verts[vid];
    out.position = float4(v.position, 0.0, 1.0);
    out.texCoord = v.texCoord;
    return out;
}

//
// Fragment Shader: Simple texture sampling
//
fragment float4 fragment_cinematic(VertexOut in [[stage_in]],
                                    texture2d<float> tex [[texture(0)]],
                                    sampler samp [[sampler(0)]]) {
    return tex.sample(samp, in.texCoord);
}
