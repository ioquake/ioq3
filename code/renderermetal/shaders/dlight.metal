#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

//==============================================================================
// DYNAMIC LIGHTING SHADERS
// Port of OpenGL2's dlight_vp.glsl / dlight_fp.glsl
// Implements ProjectDlightTexture() functionality
//==============================================================================

// Deform generation types - must match tr_local.h
#define DGEN_NONE                    0
#define DGEN_WAVE_SIN                1
#define DGEN_WAVE_SQUARE             2
#define DGEN_WAVE_TRIANGLE           3
#define DGEN_WAVE_SAWTOOTH           4
#define DGEN_WAVE_INVERSE_SAWTOOTH   5
#define DGEN_BULGE                   6

struct DlightUniforms {
    float4x4 modelViewProjection;
    float4 dlightInfo;         // xyz = light position, w = 1/radius (scale)
    float4 color;              // rgb = light color, a = alpha
    int deformGen;             // Deform generation type
    float deformParams[5];     // base, amplitude, phase, frequency, spread
    float time;                // Shader time for deforms
    float vertexLerp;          // Vertex animation lerp factor
};

struct DlightVertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
    // For vertex animation
    float3 position2 [[attribute(5)]];
    float3 normal2 [[attribute(6)]];
};

struct DlightVSOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

// Deform vertex position based on deformGen
// Matches OpenGL2's DeformPosition in dlight_vp.glsl
float3 DeformPosition(float3 pos, float3 normal, float2 st, constant DlightUniforms& uniforms) {
    if (uniforms.deformGen == DGEN_NONE) {
        return pos;
    }

    float base = uniforms.deformParams[0];
    float amplitude = uniforms.deformParams[1];
    float phase = uniforms.deformParams[2];
    float frequency = uniforms.deformParams[3];
    float spread = uniforms.deformParams[4];

    if (uniforms.deformGen == DGEN_BULGE) {
        phase *= st.x;
    } else {
        phase += dot(pos.xyz, float3(spread));
    }

    float value = phase + (uniforms.time * frequency);
    float func;

    if (uniforms.deformGen == DGEN_WAVE_SIN) {
        func = sin(value * 2.0 * M_PI_F);
    } else if (uniforms.deformGen == DGEN_WAVE_SQUARE) {
        func = sign(0.5 - fract(value));
    } else if (uniforms.deformGen == DGEN_WAVE_TRIANGLE) {
        func = abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
    } else if (uniforms.deformGen == DGEN_WAVE_SAWTOOTH) {
        func = fract(value);
    } else if (uniforms.deformGen == DGEN_WAVE_INVERSE_SAWTOOTH) {
        func = 1.0 - fract(value);
    } else { // DGEN_BULGE
        func = sin(value);
    }

    return pos + normal * (base + func * amplitude);
}

// Dynamic light vertex shader
// Matches OpenGL2's dlight_vp.glsl
vertex DlightVSOut vertex_dlight(DlightVertexIn in [[stage_in]],
                                  constant DlightUniforms& uniforms [[buffer(1)]]) {
    DlightVSOut out;

    // Vertex animation lerp if needed
    float3 position = mix(in.position, in.position2, uniforms.vertexLerp);
    float3 normal = mix(in.normal, in.normal2, uniforms.vertexLerp);

    // Apply deform if needed
    if (uniforms.deformGen != DGEN_NONE) {
        position = DeformPosition(position, normal, in.texCoord, uniforms);
    }

    // Transform to clip space
    out.position = uniforms.modelViewProjection * float4(position, 1.0);

    // Calculate distance from vertex to light
    float3 dist = uniforms.dlightInfo.xyz - position;

    // Project onto XY plane and scale by light radius to get texture coordinates
    // Add 0.5 offset to center the projection
    out.texCoord = dist.xy * uniforms.dlightInfo.w + float2(0.5);

    // Calculate dynamic light modulation:
    // 1. Back-face culling: only light front faces (step(0, N·L))
    float dlightmod = step(0.0, dot(dist, normal));

    // 2. Z-axis attenuation: fade light based on distance along Z axis
    // clamp(2.0 * (1.0 - |Z|/radius), 0, 1)
    dlightmod *= clamp(2.0 * (1.0 - abs(dist.z) * uniforms.dlightInfo.w), 0.0, 1.0);

    // Apply modulation to light color
    out.color = uniforms.color * dlightmod;

    return out;
}

// Dynamic light fragment shader
// Matches OpenGL2's dlight_fp.glsl
fragment float4 fragment_dlight(DlightVSOut in [[stage_in]],
                                 texture2d<float> dlightTexture [[texture(0)]],
                                 sampler samp [[sampler(0)]],
                                 constant DlightUniforms& uniforms [[buffer(1)]]) {
    // Sample the dynamic light texture (radial falloff pattern)
    float4 texColor = dlightTexture.sample(samp, in.texCoord);

    // Multiply by vertex color (which contains the light color and modulation)
    float4 finalColor;
    finalColor.rgb = texColor.rgb * in.color.rgb;
    finalColor.a = texColor.a * in.color.a;

    return finalColor;
}
