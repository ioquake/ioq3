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

    // Fog parameters
    float4 fogDistanceVector;  // Distance from eye to fog volume
    float4 fogDepthVector;     // Fog surface plane equation
    float4 fogColor;           // Fog RGB color + alpha
    float4 fogSurface;         // Fog surface normal and distance
    float4 fogBoundsMin;       // Fog volume min bounds
    float4 fogBoundsMax;       // Fog volume max bounds
    float fogEyeT;             // Eye position relative to fog surface
    float fogTcScale;          // Texture coordinate scale
    float fogHasSurface;       // Whether fog has a visible surface plane
    float fogEnabled;          // Enable/disable fog rendering
    float overBrightBits;      // r_overBrightBits value
};

// Texture coordinate modification parameters
// Uses the same format as GL2: 4 pairs of vec4s for up to 4 tcMod stages
// Each pair: [0] = (scaleX, shearX, translateX, turbAmplitude)
//            [1] = (shearY, scaleY, translateY, turbPhase)
struct TCModParams {
    float4 texMatrix0;  // First tcMod transform row 0
    float4 texMatrix1;  // First tcMod transform row 1
    float4 texMatrix2;  // Second tcMod transform row 0
    float4 texMatrix3;  // Second tcMod transform row 1
    float4 texMatrix4;  // Third tcMod transform row 0
    float4 texMatrix5;  // Third tcMod transform row 1
    float4 texMatrix6;  // Fourth tcMod transform row 0
    float4 texMatrix7;  // Fourth tcMod transform row 1
};

struct StageFragmentParams {
    float alphaRef;
    float alphaFunc;
    float alphaTestEnabled;
    float texCoordSelector;
    float rgbGenType;     // 0 = Vertex, 1 = Identity, 2 = IdentityLighting, 3 = LightingDiffuse
    float tcGenType;      // 0 = Texture, 1 = Lightmap, 2 = Environment
    float overBrightBits; // Per-stage overbright bits
    float padding3;
};

// Entity lighting parameters - for CGEN_LIGHTING_DIFFUSE
struct EntityLightingParams {
    float3 ambientLight;    // Ambient light color (0-255 scale)
    float padding0;
    float3 directedLight;   // Directional light color (0-255 scale)
    float padding1;
    float3 lightDir;        // Normalized light direction in world space
    float padding2;
    float3 modelLightDir;   // Light direction in model space (for normals)
    float padding3;
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
    float2 envTexCoord;   // Pre-computed environment map texture coordinates
    float4 color;
    float3 worldPosition; // World-space position for lighting
    float3 normal;        // World-space normal for lighting
    uint primitiveID [[flat]];
};

// Apply texture coordinate modifications (tcMod)
// This matches GL2's ModTexCoords function
// Each pair of texMatrix vectors represents one tcMod stage:
//   [even].xyz = row 0 of 2x3 matrix (scaleX, shearX, translateX)
//   [odd].xyz = row 1 of 2x3 matrix (shearY, scaleY, translateY)
//   [even].w = turbulence amplitude (0 if no turb)
//   [odd].w = turbulence phase
float2 ApplyTCMod(float2 st, float3 position, constant TCModParams& tcmod) {
    float2 st2 = st;
    float2 offsetPos = float2(position.x + position.z, position.y);
    
    // First tcMod
    st2 = float2(st2.x * tcmod.texMatrix0.x + st2.y * tcmod.texMatrix0.y + tcmod.texMatrix0.z,
                 st2.x * tcmod.texMatrix1.x + st2.y * tcmod.texMatrix1.y + tcmod.texMatrix1.z);
    st2 += tcmod.texMatrix0.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(tcmod.texMatrix1.w * 2.0 * M_PI_F));
    
    // Second tcMod
    st2 = float2(st2.x * tcmod.texMatrix2.x + st2.y * tcmod.texMatrix2.y + tcmod.texMatrix2.z,
                 st2.x * tcmod.texMatrix3.x + st2.y * tcmod.texMatrix3.y + tcmod.texMatrix3.z);
    st2 += tcmod.texMatrix2.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(tcmod.texMatrix3.w * 2.0 * M_PI_F));
    
    // Third tcMod
    st2 = float2(st2.x * tcmod.texMatrix4.x + st2.y * tcmod.texMatrix4.y + tcmod.texMatrix4.z,
                 st2.x * tcmod.texMatrix5.x + st2.y * tcmod.texMatrix5.y + tcmod.texMatrix5.z);
    st2 += tcmod.texMatrix4.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(tcmod.texMatrix5.w * 2.0 * M_PI_F));
    
    // Fourth tcMod
    st2 = float2(st2.x * tcmod.texMatrix6.x + st2.y * tcmod.texMatrix6.y + tcmod.texMatrix6.z,
                 st2.x * tcmod.texMatrix7.x + st2.y * tcmod.texMatrix7.y + tcmod.texMatrix7.z);
    st2 += tcmod.texMatrix6.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(tcmod.texMatrix7.w * 2.0 * M_PI_F));
    
    return st2;
}

// Compute environment map texture coordinates
// This matches OpenGL's RB_CalcEnvironmentTexCoords
float2 CalcEnvironmentTexCoords(float3 position, float3 normal, float3 viewOrigin) {
    // Calculate viewer direction from vertex to eye
    float3 viewer = normalize(viewOrigin - position);
    
    // Calculate reflection vector: R = 2 * (N . V) * N - V
    float d = dot(normal, viewer);
    float3 reflected = normal * 2.0 * d - viewer;
    
    // Generate texture coordinates from reflection vector
    // Use Y and Z components to create a spherical mapping
    float s = 0.5 + reflected.y * 0.5;
    float t = 0.5 - reflected.z * 0.5;
    
    return float2(s, t);
}

vertex SceneVSOut vertex_scene_basic(VertexIn in [[stage_in]],
                                      constant SceneUniforms& uniforms [[buffer(1)]],
                                      constant TCModParams& tcmod [[buffer(2)]],
                                      uint vid [[vertex_id]]) {
    SceneVSOut out;
    float4 worldPos = float4(in.position, 1.0);

    // Apply transformations in two steps: view then projection
    // This bypasses any potential matrix multiplication issues
    float4 viewPos = uniforms.view * worldPos;
    out.position = uniforms.projection * viewPos;

    // Apply texture coordinate modifications
    out.texCoord = ApplyTCMod(in.texCoord, in.position, tcmod);
    out.lightmapCoord = in.lightmapCoord;

    // Pre-compute environment map texture coordinates
    out.envTexCoord = CalcEnvironmentTexCoords(in.position, in.normal, uniforms.viewOrigin.xyz);

    out.color = in.color;  // Already normalized by MTL::VertexFormatUChar4Normalized
    out.worldPosition = in.position;
    out.normal = in.normal;
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
                                      constant StageFragmentParams& stage [[buffer(0)]],
                                      constant SceneUniforms& uniforms [[buffer(1)]],
                                      constant EntityLightingParams& lighting [[buffer(2)]]) {
    // Select texture coordinates based on tcGenType
    // 0 = Texture (base texcoords), 1 = Lightmap, 2 = Environment
    float2 uv;
    if (stage.tcGenType > 1.5f) {
        // tcGen environment - use pre-computed environment map coords
        uv = in.envTexCoord;
    } else if (stage.tcGenType > 0.5f) {
        // tcGen lightmap
        uv = in.lightmapCoord;
    } else {
        // tcGen texture (default)
        uv = in.texCoord;
    }

    float4 texColor = tex.sample(samp, uv);

    // Apply rgbGen: determine vertex color to use based on rgbGen type
    // 0 = Vertex (use in.color), 1 = Identity (white), 2 = IdentityLighting (white), 3 = LightingDiffuse
    float4 vertexColor = in.color;
    bool applyOverbright = false;

    if (stage.rgbGenType > 2.5f) {
        // rgbGen lightingDiffuse - entity lighting (CGEN_LIGHTING_DIFFUSE)
        // Matches OpenGL2: color = ambientLight + N·L * directedLight
        float3 ambient = lighting.ambientLight / 255.0;
        float3 directed = lighting.directedLight / 255.0;

        // Calculate N·L (normal dot light direction)
        float NdotL = max(0.0, dot(normalize(in.normal), lighting.lightDir));

        // Combine ambient and directional lighting
        float3 litColor = ambient + NdotL * directed;

        vertexColor = float4(litColor, in.color.a);
        applyOverbright = true;
    } else if (stage.rgbGenType > 1.5f) {
        // rgbGen identityLighting - use white, NO overbright (for pre-lit surfaces)
        vertexColor = float4(1.0, 1.0, 1.0, 1.0);
        applyOverbright = false;
    } else if (stage.rgbGenType > 0.5f) {
        // rgbGen identity - use white WITH overbright
        vertexColor = float4(1.0, 1.0, 1.0, 1.0);
        applyOverbright = true;
    } else {
        // rgbGen vertex - use vertex colors WITHOUT additional overbright
        // Vertex colors already have overbright baked in via ColorShiftLightingBytes
        // This matches OpenGL2's generic_fp.glsl which just does "color.rgb * var_Color.rgb"
        applyOverbright = false;
    }

    float4 color = texColor * vertexColor;

    // Apply overbright bits scaling (per-stage) for Identity and LightingDiffuse modes only
    // Do NOT apply for Vertex mode - vertex colors already have overbright baked in!
    if (applyOverbright && stage.overBrightBits > 0.0) {
        color.rgb *= exp2(stage.overBrightBits);
    }

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

//
// MODEL RENDERING (MD3)
// Matches OpenGL2 generic_vp.glsl with USE_VERTEX_ANIMATION
//

struct ModelUniforms {
    float4x4 modelViewProjection;  // Combined MVP matrix for the entity
    float vertexLerp;              // Frame interpolation factor (0.0-1.0)
    float3 padding;
};

struct ModelVertexIn {
    float3 position [[attribute(0)]];      // Old frame position
    float3 normal [[attribute(1)]];        // Old frame normal
    float2 texCoord [[attribute(2)]];      // Texture coordinates
    float3 position2 [[attribute(3)]];     // New frame position
    float3 normal2 [[attribute(4)]];       // New frame normal
};

struct ModelVertexOut {
    float4 position [[position]];
    float2 texCoord;
    float3 normal;        // Interpolated normal for lighting
    float3 worldPosition; // World position for lighting
};

// Model vertex shader with frame interpolation
vertex ModelVertexOut vertex_model(ModelVertexIn in [[stage_in]],
                                   constant ModelUniforms& modelUniforms [[buffer(1)]],
                                   constant SceneUniforms& sceneUniforms [[buffer(2)]]) {
    ModelVertexOut out;

    // Interpolate between old and new frame
    float3 position = mix(in.position, in.position2, modelUniforms.vertexLerp);
    float3 normal = mix(in.normal, in.normal2, modelUniforms.vertexLerp);

    // Transform to clip space
    out.position = modelUniforms.modelViewProjection * float4(position, 1.0);

    // Pass through texture coordinates
    out.texCoord = in.texCoord;

    // Pass through normal and position for lighting
    out.normal = normalize(normal);
    out.worldPosition = position;

    return out;
}

// Model fragment shader
fragment float4 fragment_model(ModelVertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant EntityLightingParams& lighting [[buffer(0)]]) {
    // Sample texture
    float4 texColor = tex.sample(samp, in.texCoord);

    // Calculate lighting (CGEN_LIGHTING_DIFFUSE)
    // Matches OpenGL2: color = ambientLight + N·L * directedLight
    float3 ambient = lighting.ambientLight / 255.0;
    float3 directed = lighting.directedLight / 255.0;

    // Calculate N·L (normal dot light direction)
    float NdotL = max(0.0, dot(normalize(in.normal), lighting.lightDir));

    // Combine ambient and directional lighting
    float3 litColor = ambient + NdotL * directed;

    // Combine texture and lighting
    float4 color = texColor * float4(litColor, 1.0);

    return color;
}

//
// FOG RENDERING
// Matches OpenGL2 fogpass_vp.glsl / fogpass_fp.glsl implementation
//

struct FogVertexOut {
    float4 position [[position]];
    float fogScale;      // Computed fog intensity/alpha scale
};

// Calculate fog intensity for a vertex position
// Simplified approach: fog increases with depth below the fog surface
float CalcFog(float3 position, constant SceneUniforms& uniforms) {
    // t = how far the vertex is below the fog surface (positive = below surface)
    float t = dot(float4(position, 1.0), uniforms.fogDepthVector);
    
    // If t <= 0, vertex is above fog surface - no fog
    if (t <= 0.0) {
        return 0.0;
    }
    
    // eyeT tells us how far the camera is below the surface
    float eyeT = uniforms.fogEyeT;
    
    // s = fog distance factor based on tcScale
    // Using the depth below surface as our fog distance
    float s = t * uniforms.fogTcScale * 8.0;
    
    // If eye is above the fog (eyeT < 0), we need to clip the fog
    // to only show the portion of the vertex-to-eye line that's inside fog
    if (eyeT < 0.0) {
        // Eye is outside fog, vertex is inside
        // Only fog the portion of the line from fog surface to vertex
        // t is distance from surface to vertex, eyeT is (negative) distance from surface to eye
        // The fraction of the line inside fog is t / (t - eyeT) 
        float fraction = t / (t - eyeT);
        s *= fraction;
    }
    // If eye is inside fog (eyeT >= 0), full fog applies
    
    return s;
}

// Fog vertex shader
vertex FogVertexOut vertex_fog(VertexIn in [[stage_in]],
                                constant SceneUniforms& uniforms [[buffer(1)]]) {
    FogVertexOut out;

    // Transform to clip space
    float4 viewPos = uniforms.view * float4(in.position, 1.0);
    out.position = uniforms.projection * viewPos;

    // The surface's fogIndex already filters which surfaces are in fog volumes
    // Just calculate fog for this vertex
    float fogValue = CalcFog(in.position, uniforms);
    // Apply color alpha squared (matches u_Color.a * u_Color.a in GLSL)
    out.fogScale = fogValue * uniforms.fogColor.a * uniforms.fogColor.a;

    return out;
}

// Fog fragment shader
fragment float4 fragment_fog(FogVertexOut in [[stage_in]],
                              constant SceneUniforms& uniforms [[buffer(1)]]) {
    // Compute fog alpha matching OpenGL2: gl_FragColor.a = sqrt(clamp(var_Scale, 0.0, 1.0))
    float alpha = sqrt(clamp(in.fogScale, 0.0, 1.0));

    // Return fog color with computed alpha
    return float4(uniforms.fogColor.rgb, alpha);
}
