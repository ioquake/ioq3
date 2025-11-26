//
// 2D UI Rendering Shader
// GPU-accelerated quad rendering with instancing
//

#include <metal_stdlib>
using namespace metal;

//
// Instance data for each quad
//
struct QuadInstance {
    float4 rect;        // x, y, w, h in NDC coordinates
    float4 texCoords;   // s1, t1, s2, t2
    float4 color;       // RGBA modulation
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

//
// Vertex Shader: Procedurally generate quad vertices
// No vertex buffer needed - GPU generates geometry
//
vertex VertexOut vertex_ui_2d(
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]],
    constant QuadInstance* instances [[buffer(0)]])
{
    // Triangle strip positions: (0,0), (1,0), (0,1), (1,1)
    const float2 positions[4] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.0, 1.0),
        float2(1.0, 1.0)
    };
    
    QuadInstance inst = instances[instanceID];
    float2 pos = positions[vertexID];
    
    VertexOut out;
    
    // Transform to NDC using instance rect
    out.position = float4(
        inst.rect.x + pos.x * inst.rect.z,  // x + u * width
        inst.rect.y + pos.y * inst.rect.w,  // y + v * height
        0.0,
        1.0
    );
    
    // Inter pollate texture coordinates
    out.texCoord = mix(inst.texCoords.xy, inst.texCoords.zw, pos);
    
    // Pass through color
    out.color = inst.color;
    
    return out;
}

//
// Fragment Shader: Sample texture and apply color modulation
//
fragment float4 fragment_ui_2d(
    VertexOut in [[stage_in]],
    texture2d<float> tex [[texture(0)]],
    sampler samp [[sampler(0)]])
{
    float4 texColor = tex.sample(samp, in.texCoord);
    return texColor * in.color;
}
