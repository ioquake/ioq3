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
    float greyscale;           // r_greyscale: 0.0 = colour, 1.0 = full greyscale
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
    // --- 8 scalars at offsets 0-28 ---
    float alphaRef;
    float alphaFunc;
    float alphaTestEnabled;
    float texCoordSelector;
    // rgbGenType: 0=Vertex, 1=Identity, 2=IdentityLighting, 3=LightingDiffuse, 4=Wave,
    //             5=Const, 6=Entity, 7=OneMinusEntity, 8=OneMinusVertex
    float rgbGenType;
    // tcGenType:  0=Texture, 1=Lightmap, 2=Environment, 3=Vector, 4=Fog
    float tcGenType;
    float overBrightBits;
    float waveColorScale; // For rgbGen wave - precomputed CPU wave value
    // --- 4 scalars at offsets 32-44 (explicit pad to align float4s) ---
    // alphaGenType: 0=Identity/default, 1=Entity, 2=OneMinusEntity, 3=Wave, 4=Specular, 5=Portal
    float alphaGenType;
    float alphaWaveValue;  // Precomputed clamped wave value for AGEN_WAVEFORM
    float portalRange;     // View-distance threshold for AGEN_PORTAL
    float _pad0;           // Explicit pad so entityColor lands at offset 48
    // --- float4 members at 16-byte-aligned offsets 48-111 ---
    float4 entityColor;    // Entity shaderRGBA / 255  (CGEN/AGEN_ENTITY, ONE_MINUS_ENTITY)
    float4 constColor;     // CGEN_CONST constant color
    float4 tcGenSVector;   // TCGEN_VECTOR s-axis (w unused)
    float4 tcGenTVector;   // TCGEN_VECTOR t-axis (w unused)
};

// Entity lighting parameters - for CGEN_LIGHTING_DIFFUSE
// Layout must match C++ EntityLightingParams struct exactly
struct EntityLightingParams {
    float4 ambientLight;    // xyz = color (0-1), w = padding
    float4 directedLight;   // xyz = color (0-1), w = padding
    float4 lightDir;        // xyz = world space dir, w = padding
    float4 modelLightDir;   // xyz = model space dir, w = overBrightBits
};

// Normal-map and specular-map parameters
// Layout must match C++ NormalSpecularParams struct exactly
struct NormalSpecularParams {
    float useNormalMap;    // 1.0 if a normal map is active, 0.0 otherwise
    float useSpecularMap;  // 1.0 if a specular map is active, 0.0 otherwise
    float normalScale;     // XY normal perturbation scale (typically 1.0)
    float specularPower;   // Blinn-Phong shininess exponent (typically 32.0)
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
                                      texture2d<float> normalMap [[texture(1)]],
                                      texture2d<float> specMap [[texture(2)]],
                                      sampler samp [[sampler(0)]],
                                      constant StageFragmentParams& stage [[buffer(0)]],
                                      constant SceneUniforms& uniforms [[buffer(1)]],
                                      constant EntityLightingParams& lighting [[buffer(2)]],
                                      constant NormalSpecularParams& nsParams [[buffer(3)]]) {
    // Select texture coordinates based on tcGenType
    // 0=Texture, 1=Lightmap, 2=Environment, 3=Vector, 4=Fog
    float2 uv;
    if (stage.tcGenType > 3.5f) {
        // tcGen fog - compute fog texture coordinates from depth/distance vectors
        // Matches RB_CalcFogTexCoords: s = distance factor, t = depth clamp factor
        float fogS = dot(float4(in.worldPosition, 1.0f), uniforms.fogDistanceVector);
        float fogT = dot(float4(in.worldPosition, 1.0f), uniforms.fogDepthVector);
        float eyeOutside = uniforms.fogEyeT < 0.0f ? 1.0f : 0.0f;
        float fogged     = fogT >= eyeOutside ? 1.0f : 0.0f;
        fogT += 1e-6f;
        float denom = fogT - uniforms.fogEyeT * eyeOutside;
        fogT = fogged * fogT / (fabs(denom) > 1e-6f ? denom : 1e-6f);
        uv = float2(fogS, fogT);
    } else if (stage.tcGenType > 2.5f) {
        // tcGen vector - dot-product UV generation (GL2: GenTexCoords TCGEN_VECTOR)
        uv = float2(dot(in.worldPosition, stage.tcGenSVector.xyz),
                    dot(in.worldPosition, stage.tcGenTVector.xyz));
    } else if (stage.tcGenType > 1.5f) {
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

    // Apply rgbGen: determine vertex color based on rgbGenType
    // 0=Vertex, 1=Identity, 2=IdentityLighting, 3=LightingDiffuse, 4=Wave,
    // 5=Const, 6=Entity, 7=OneMinusEntity, 8=OneMinusVertex
    float4 vertexColor = in.color;
    bool applyOverbright = false;

    if (stage.rgbGenType > 7.5f) {
        // rgbGen oneMinusVertex - invert vertex RGB, alpha fixed at 1.0 (matches GL2)
        vertexColor = float4(1.0f - in.color.rgb, 1.0f);
        applyOverbright = false;
    } else if (stage.rgbGenType > 6.5f) {
        // rgbGen oneMinusEntity - invert entity RGBA
        vertexColor = float4(1.0f - stage.entityColor.rgb, 1.0f - stage.entityColor.a);
        applyOverbright = false;
    } else if (stage.rgbGenType > 5.5f) {
        // rgbGen entity - use entity shaderRGBA
        vertexColor = stage.entityColor;
        applyOverbright = false;
    } else if (stage.rgbGenType > 4.5f) {
        // rgbGen const - use constant color from shader script
        vertexColor = stage.constColor;
        applyOverbright = false;
    } else if (stage.rgbGenType > 3.5f) {
        // rgbGen wave - use pre-computed wave color scale
        vertexColor = float4(stage.waveColorScale, stage.waveColorScale, stage.waveColorScale, 1.0f);
        applyOverbright = true;
    } else if (stage.rgbGenType > 2.5f) {
        // rgbGen lightingDiffuse - entity lighting (ambient + directional)
        float3 ambient  = lighting.ambientLight.xyz;
        float3 directed = lighting.directedLight.xyz;
        float NdotL     = max(0.0f, dot(normalize(in.normal), lighting.lightDir.xyz));
        float3 litColor = ambient + NdotL * directed;
        vertexColor = float4(litColor, in.color.a);
        applyOverbright = true;
    } else if (stage.rgbGenType > 1.5f) {
        // rgbGen identityLighting - white, NO overbright (pre-lit surfaces)
        vertexColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        applyOverbright = false;
    } else if (stage.rgbGenType > 0.5f) {
        // rgbGen identity - white WITH overbright
        vertexColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        applyOverbright = true;
    } else {
        // rgbGen vertex - vertex colors (overbright already baked in via ColorShiftLightingBytes)
        applyOverbright = false;
    }

    float4 color = texColor * vertexColor;

    // alphaGen override - 0=Identity(keep), 1=Entity, 2=OneMinusEntity, 3=Wave, 4=Specular, 5=Portal
    if (stage.alphaGenType > 4.5f) {
        // AGEN_PORTAL: fade alpha based on distance from viewer
        float dist = length(uniforms.viewOrigin.xyz - in.worldPosition);
        color.a = texColor.a * clamp(dist / stage.portalRange, 0.0f, 1.0f);
    } else if (stage.alphaGenType > 3.5f) {
        // AGEN_LIGHTING_SPECULAR: specular highlight with fixed light position (matches GL2 generic_vp.glsl)
        float3 lightDir  = normalize(float3(-960.0f, 1980.0f, 96.0f) - in.worldPosition);
        float3 reflected = -reflect(lightDir, normalize(in.normal));
        float3 viewer    = normalize(uniforms.viewOrigin.xyz - in.worldPosition);
        float spec = clamp(dot(reflected, viewer), 0.0f, 1.0f);
        spec = spec * spec * spec * spec;  // ^4 as in GL2
        color.a = texColor.a * spec;
    } else if (stage.alphaGenType > 2.5f) {
        // AGEN_WAVEFORM: precomputed alpha wave value from CPU
        color.a = texColor.a * stage.alphaWaveValue;
    } else if (stage.alphaGenType > 1.5f) {
        // AGEN_ONE_MINUS_ENTITY
        color.a = texColor.a * (1.0f - stage.entityColor.a);
    } else if (stage.alphaGenType > 0.5f) {
        // AGEN_ENTITY
        color.a = texColor.a * stage.entityColor.a;
    }
    // else: AGEN_IDENTITY / default — keep color.a = texColor.a * vertexColor.a

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

    // Normal-map and specular-map contribution (additive Blinn-Phong specular).
    // The diffuse path is left unchanged so lightmaps continue to look correct.
    // World-surface EntityLighting is in 0-255 range, so divide by 255 for specular.
    if (nsParams.useNormalMap > 0.5f || nsParams.useSpecularMap > 0.5f) {
        // Cotangent-frame TBN — no per-vertex tangent data required.
        float3 dp1  = dfdx(in.worldPosition);
        float3 dp2  = dfdy(in.worldPosition);
        float2 duv1 = dfdx(uv);
        float2 duv2 = dfdy(uv);
        float det = duv1.x * duv2.y - duv2.x * duv1.y;
        float f   = (abs(det) > 1e-8f) ? (1.0f / det) : 0.0f;
        float3 T_raw = f * (duv2.y * dp1 - duv1.y * dp2);
        float3 N     = normalize(in.normal);
        float3 T     = normalize(T_raw - dot(T_raw, N) * N);
        float3 B     = cross(N, T);
        float3x3 TBN = float3x3(T, B, N);

        if (nsParams.useNormalMap > 0.5f) {
            float3 tsN = normalMap.sample(samp, uv).rgb * 2.0f - 1.0f;
            tsN.xy    *= nsParams.normalScale;
            tsN.z      = sqrt(max(0.0f, 1.0f - tsN.x * tsN.x - tsN.y * tsN.y));
            N          = normalize(TBN * tsN);
        }

        float3 L    = normalize(lighting.lightDir.xyz);
        float3 V    = normalize(uniforms.viewOrigin.xyz - in.worldPosition);
        float3 H    = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0f);
        float NdotL = max(dot(N, L), 0.0f);
        float specF = pow(NdotH, nsParams.specularPower) * NdotL;

        float3 specLight = lighting.directedLight.xyz * (1.0f / 255.0f);
        if (nsParams.useSpecularMap > 0.5f) {
            specLight *= specMap.sample(samp, uv).rgb;
        }
        color.rgb += specLight * specF;
    }

    // Apply greyscale (matches GL2 greyscale_fp.glsl, Rec. 709-1 LUMA coefficients)
    if (uniforms.greyscale > 0.0f) {
        const float3 LUMA = float3(0.2125f, 0.7154f, 0.0721f);
        float y = dot(color.rgb, LUMA);
        color.rgb = mix(color.rgb, float3(y), clamp(uniforms.greyscale, 0.0f, 1.0f));
    }

    return color;
}

//
// MODEL RENDERING (MD3)
// Matches OpenGL2 generic_vp.glsl with USE_VERTEX_ANIMATION
//

struct ModelUniforms {
    float4x4 modelViewProjection;  // Combined MVP matrix for the entity
    float4x4 modelMatrix;          // Model matrix to recover world-space position
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
    float2 envTexCoord;   // Pre-computed environment map texture coordinates
    float3 normal;        // Model-space normal for lighting (matches OpenGL)
    float3 worldNormal;   // World-space normal for TBN construction
    float3 worldPosition; // World-space position for fog and env mapping
};

// Parameters for model stage rendering (per-stage uniforms)
// Includes tcMod matrices for animated texture effects
struct ModelStageParams {
    float tcGenType;      // 0 = Texture, 1 = Lightmap, 2 = Environment
    float padding[3];
    // TCMod matrices (same format as TCModParams)
    float4 texMatrix0;    // First tcMod transform row 0
    float4 texMatrix1;    // First tcMod transform row 1
    float4 texMatrix2;    // Second tcMod transform row 0
    float4 texMatrix3;    // Second tcMod transform row 1
    float4 texMatrix4;    // Third tcMod transform row 0
    float4 texMatrix5;    // Third tcMod transform row 1
    float4 texMatrix6;    // Fourth tcMod transform row 0
    float4 texMatrix7;    // Fourth tcMod transform row 1
};

struct ModelFogParams {
    float4 fogColor;
    float4 fogDistanceVector;
    float4 fogDepthVector;
    float fogEyeT;
    float fogTcScale;
    float fogEnabled;
    float fogHasSurface;
};

// Model vertex shader with frame interpolation
vertex ModelVertexOut vertex_model(ModelVertexIn in [[stage_in]],
                                   constant ModelUniforms& modelUniforms [[buffer(1)]],
                                   constant SceneUniforms& sceneUniforms [[buffer(2)]]) {
    ModelVertexOut out;

    // Interpolate between old and new frame
    float3 position = mix(in.position, in.position2, modelUniforms.vertexLerp);
    float3 normal = mix(in.normal, in.normal2, modelUniforms.vertexLerp);

    float4 localPos = float4(position, 1.0);
    float4 worldPos = modelUniforms.modelMatrix * localPos;

    // Transform to clip space
    out.position = modelUniforms.modelViewProjection * localPos;

    // Pass through texture coordinates
    out.texCoord = in.texCoord;

    // Keep normal in model space for lighting (matches OpenGL's u_ModelLightDir approach)
    out.normal = normalize(normal);
    // World-space normal for fragment-shader TBN (rigid-body model, upper-left 3x3 is rotation)
    out.worldNormal = normalize((modelUniforms.modelMatrix * float4(normal, 0.0f)).xyz);
    out.worldPosition = worldPos.xyz;

    // Pre-compute environment map texture coordinates (for tcGen environment)
    out.envTexCoord = CalcEnvironmentTexCoords(worldPos.xyz, out.normal, sceneUniforms.viewOrigin.xyz);

    return out;
}

inline float CalcModelFog(float3 worldPos, constant ModelFogParams& fog) {
    if (fog.fogEnabled < 0.5f) {
        return 0.0f;
    }

    float s = dot(float4(worldPos, 1.0), fog.fogDistanceVector) * 8.0f;
    float t = dot(float4(worldPos, 1.0), fog.fogDepthVector);

    float eyeOutside = fog.fogEyeT < 0.0f ? 1.0f : 0.0f;
    float fogged = t >= eyeOutside ? 1.0f : 0.0f;

    t += 1e-6f;
    float denom = t - fog.fogEyeT * eyeOutside;
    if (fabs(denom) < 1e-6f) {
        return 0.0f;
    }

    t *= fogged / denom;
    return s * t;
}

// Model fragment shader
fragment float4 fragment_model(ModelVertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              texture2d<float> normalMap [[texture(1)]],
                              texture2d<float> specMap [[texture(2)]],
                              sampler samp [[sampler(0)]],
                              constant EntityLightingParams& lighting [[buffer(0)]],
                              constant ModelFogParams& fogParams [[buffer(1)]],
                              constant ModelStageParams& stageParams [[buffer(2)]],
                              constant SceneUniforms& sceneUniforms [[buffer(3)]],
                              constant NormalSpecularParams& nsParams [[buffer(4)]]) {
    // Select texture coordinates based on tcGenType
    // 0 = Texture (base texcoords), 1 = Lightmap, 2 = Environment
    float2 uv;
    if (stageParams.tcGenType > 1.5f) {
        // tcGen environment - use pre-computed environment map coords
        uv = in.envTexCoord;
    } else {
        // tcGen texture (default) - models don't have lightmap coords
        uv = in.texCoord;
    }

    // Apply tcMod transformations (rotate, scroll, scale, etc.)
    // Uses same format as world surface ApplyTCMod but simplified for models
    float2 offsetPos = float2(in.worldPosition.x + in.worldPosition.z, in.worldPosition.y);
    
    // First tcMod
    uv = float2(uv.x * stageParams.texMatrix0.x + uv.y * stageParams.texMatrix0.y + stageParams.texMatrix0.z,
                uv.x * stageParams.texMatrix1.x + uv.y * stageParams.texMatrix1.y + stageParams.texMatrix1.z);
    uv += stageParams.texMatrix0.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(stageParams.texMatrix1.w * 2.0 * M_PI_F));
    
    // Second tcMod
    uv = float2(uv.x * stageParams.texMatrix2.x + uv.y * stageParams.texMatrix2.y + stageParams.texMatrix2.z,
                uv.x * stageParams.texMatrix3.x + uv.y * stageParams.texMatrix3.y + stageParams.texMatrix3.z);
    uv += stageParams.texMatrix2.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(stageParams.texMatrix3.w * 2.0 * M_PI_F));
    
    // Third tcMod
    uv = float2(uv.x * stageParams.texMatrix4.x + uv.y * stageParams.texMatrix4.y + stageParams.texMatrix4.z,
                uv.x * stageParams.texMatrix5.x + uv.y * stageParams.texMatrix5.y + stageParams.texMatrix5.z);
    uv += stageParams.texMatrix4.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(stageParams.texMatrix5.w * 2.0 * M_PI_F));
    
    // Fourth tcMod
    uv = float2(uv.x * stageParams.texMatrix6.x + uv.y * stageParams.texMatrix6.y + stageParams.texMatrix6.z,
                uv.x * stageParams.texMatrix7.x + uv.y * stageParams.texMatrix7.y + stageParams.texMatrix7.z);
    uv += stageParams.texMatrix6.w * sin(offsetPos * (2.0 * M_PI_F / 1024.0) + float2(stageParams.texMatrix7.w * 2.0 * M_PI_F));

    // Sample texture
    float4 texColor = tex.sample(samp, uv);
    
    // Calculate lighting (CGEN_LIGHTING_DIFFUSE)
    // Matches OpenGL2's lightall_vp.glsl with USE_LIGHT_VECTOR + USE_FAST_LIGHT (default):
    //   var_Color = u_VertColor * attr_Color + u_BaseColor;  // baseColor = overbright (e.g. 2.0)
    //   var_Color.rgb *= u_DirectedLight * (attenuation * NL) + u_AmbientLight;
    // Fragment shader (lightall_fp.glsl USE_FAST_LIGHT path):
    //   gl_FragColor.rgb = diffuse.rgb * lightColor;
    
    // Lighting values are already normalized to 0-1 range on CPU
    float3 ambient = lighting.ambientLight.xyz;
    float3 directed = lighting.directedLight.xyz;
    float overBrightBits = lighting.modelLightDir.w;  // Stored in modelLightDir.w

    // Calculate N·L using model-space light direction and model-space normals
    float3 modelLightDir = normalize(lighting.modelLightDir.xyz);
    float NdotL = clamp(dot(normalize(in.normal), modelLightDir), 0.0f, 1.0f);

    // Combine ambient and directional lighting
    float3 litColor = directed * NdotL + ambient;
    
    // Apply overbright scaling (matches lightall_vp.glsl: var_Color *= lighting)
    // baseColor = 1 << overBrightBits (e.g., 2.0 for overBrightBits=1)
    float overBrightScale = exp2(overBrightBits);
    litColor *= overBrightScale;
    
    // Combine texture and lighting
    // HDR values > 1.0 are clamped at framebuffer output, matching OpenGL2 behavior
    float4 color = texColor * float4(litColor, 1.0);

    // Apply fog
    if (fogParams.fogEnabled > 0.5f) {
        float fogValue = CalcModelFog(in.worldPosition, fogParams);
        float fogFactor = sqrt(clamp(fogValue, 0.0f, 1.0f));
        color.rgb = mix(color.rgb, fogParams.fogColor.rgb, fogFactor);
        color.a = mix(color.a, fogParams.fogColor.a, fogFactor);
    }

    // Normal-map and specular-map contribution (additive Blinn-Phong specular).
    // Lighting is already normalized to 0-1 for model surfaces (done on CPU).
    if (nsParams.useNormalMap > 0.5f || nsParams.useSpecularMap > 0.5f) {
        // Cotangent-frame TBN using world-space position and UV derivatives.
        float3 dp1  = dfdx(in.worldPosition);
        float3 dp2  = dfdy(in.worldPosition);
        float2 duv1 = dfdx(uv);
        float2 duv2 = dfdy(uv);
        float det = duv1.x * duv2.y - duv2.x * duv1.y;
        float f   = (abs(det) > 1e-8f) ? (1.0f / det) : 0.0f;
        float3 T_raw = f * (duv2.y * dp1 - duv1.y * dp2);
        float3 N     = normalize(in.worldNormal);
        float3 T     = normalize(T_raw - dot(T_raw, N) * N);
        float3 B     = cross(N, T);
        float3x3 TBN = float3x3(T, B, N);

        if (nsParams.useNormalMap > 0.5f) {
            float3 tsN = normalMap.sample(samp, uv).rgb * 2.0f - 1.0f;
            tsN.xy    *= nsParams.normalScale;
            tsN.z      = sqrt(max(0.0f, 1.0f - tsN.x * tsN.x - tsN.y * tsN.y));
            N          = normalize(TBN * tsN);
        }

        float3 L    = normalize(lighting.lightDir.xyz);
        float3 V    = normalize(sceneUniforms.viewOrigin.xyz - in.worldPosition);
        float3 H    = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0f);
        float NdotL = max(dot(N, L), 0.0f);
        float specF = pow(NdotH, nsParams.specularPower) * NdotL;

        // Specular uses the directed-light color (already 0-1 for models).
        float3 specLight = lighting.directedLight.xyz;
        if (nsParams.useSpecularMap > 0.5f) {
            specLight *= specMap.sample(samp, uv).rgb;
        }
        color.rgb += specLight * specF * overBrightScale;
    }

    // Apply greyscale (matches GL2 greyscale_fp.glsl, Rec. 709-1 LUMA coefficients)
    if (sceneUniforms.greyscale > 0.0f) {
        const float3 LUMA = float3(0.2125f, 0.7154f, 0.0721f);
        float y = dot(color.rgb, LUMA);
        color.rgb = mix(color.rgb, float3(y), clamp(sceneUniforms.greyscale, 0.0f, 1.0f));
    }

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

//
// PROJECTION (BLOB) SHADOW RENDERING
// Matches RB_ProjectionShadowDeform from renderergl2/tr_shadows.c.
// Vertex deformation is done on the CPU; this shader just transforms
// the already-projected shadow geometry and outputs a dark translucent colour.
//

struct ShadowVertexIn {
    float3 position [[attribute(0)]];
};

// Uniforms shared by vertex and fragment shadow shaders.
// Layout must match C++ ShadowShaderUniforms in tr_backend.cpp exactly.
struct ShadowShaderUniforms {
    float4x4 mvpMatrix;  // Combined model-view-projection (entity transform baked in)
    float4   color;      // Shadow colour – typically (0, 0, 0, alpha)
};

vertex float4 vertex_shadow(ShadowVertexIn in [[stage_in]],
                             constant ShadowShaderUniforms& uniforms [[buffer(1)]]) {
    return uniforms.mvpMatrix * float4(in.position, 1.0);
}

fragment float4 fragment_shadow(float4 in [[position]],
                                 constant ShadowShaderUniforms& uniforms [[buffer(1)]]) {
    return uniforms.color;
}
