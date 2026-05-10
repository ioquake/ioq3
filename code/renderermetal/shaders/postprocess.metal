/*
===========================================================================
Post-processing pipeline for the Metal renderer.
Implements HDR tonemapping, SSAO, bloom, depth-of-field, and sun rays,
matching the mathematics of GL2's tr_postprocess.c and associated shaders.
===========================================================================
*/

#include <metal_stdlib>
using namespace metal;

// ============================================================
// Shared vertex output / fullscreen triangle
// ============================================================

struct FullscreenVOut {
    float4 position [[position]];
    float2 texCoord;
};

// Generates a single large triangle that covers the entire viewport.
// Avoids the overdraw of two-triangle quads and requires no vertex buffer.
vertex FullscreenVOut vertex_fullscreen(uint vid [[vertex_id]])
{
    FullscreenVOut out;
    // Three corners in NDC that cover [-1,1]×[-1,1]:
    //   vid=0: top-left     NDC(-1, 1)  UV(0,0)
    //   vid=1: bottom-left  NDC(-1,-3)  UV(0,2)
    //   vid=2: top-right    NDC( 3, 1)  UV(2,0)
    const float4 positions[3] = {
        float4(-1.0f,  1.0f, 0.0f, 1.0f),
        float4(-1.0f, -3.0f, 0.0f, 1.0f),
        float4( 3.0f,  1.0f, 0.0f, 1.0f)
    };
    const float2 uvs[3] = {
        float2(0.0f, 0.0f),
        float2(0.0f, 2.0f),
        float2(2.0f, 0.0f)
    };
    out.position = positions[vid];
    out.texCoord = uvs[vid];
    return out;
}

// ============================================================
// Pass-through blit
// ============================================================

fragment float4 fragment_passthrough(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]])
{
    return tex.sample(s, in.texCoord);
}

// ============================================================
// 4×4 box downsample  (matches GL2 down4x_fp.glsl)
// buffer(0): float4(invSrcWidth, invSrcHeight, 0, 0)
// ============================================================

fragment float4 fragment_downsample4x(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]],
    constant float4& inv     [[buffer(0)]])
{
    float2 tc = in.texCoord;
    float2 d  = inv.xy;
    float4 color = float4(0.0f);
    color += tex.sample(s, tc + d * float2(-1.5f, -1.5f));
    color += tex.sample(s, tc + d * float2(-0.5f, -1.5f));
    color += tex.sample(s, tc + d * float2( 0.5f, -1.5f));
    color += tex.sample(s, tc + d * float2( 1.5f, -1.5f));
    color += tex.sample(s, tc + d * float2(-1.5f, -0.5f));
    color += tex.sample(s, tc + d * float2(-0.5f, -0.5f));
    color += tex.sample(s, tc + d * float2( 0.5f, -0.5f));
    color += tex.sample(s, tc + d * float2( 1.5f, -0.5f));
    color += tex.sample(s, tc + d * float2(-1.5f,  0.5f));
    color += tex.sample(s, tc + d * float2(-0.5f,  0.5f));
    color += tex.sample(s, tc + d * float2( 0.5f,  0.5f));
    color += tex.sample(s, tc + d * float2( 1.5f,  0.5f));
    color += tex.sample(s, tc + d * float2(-1.5f,  1.5f));
    color += tex.sample(s, tc + d * float2(-0.5f,  1.5f));
    color += tex.sample(s, tc + d * float2( 0.5f,  1.5f));
    color += tex.sample(s, tc + d * float2( 1.5f,  1.5f));
    return color * 0.0625f;
}

// ============================================================
// Luminance — first pass
// Reads one HDR texel and encodes log-luminance as min/avg/max.
// Matches GL2 calclevels4x_fp.glsl with FIRST_PASS defined.
// buffer(0): float4(invSrcWidth, invSrcHeight, 0, 0)
// ============================================================

constant float3 kLuminanceVector = float3(0.2125f, 0.7154f, 0.0721f);

fragment float4 fragment_calc_levels_first(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]],
    constant float4& inv     [[buffer(0)]])
{
    float3 color   = tex.sample(s, in.texCoord).rgb;
    float  lum     = max(dot(kLuminanceVector, color), 0.000001f);
    float  logLum  = clamp(log2(lum), -10.0f, 10.0f);
    float  encoded = logLum * 0.05f + 0.5f;   // pack [-10,10] into [0,1]
    // All three components (min, avg, max) are the same for a single sample.
    return float4(encoded, encoded, encoded, 1.0f);
}

// ============================================================
// Luminance — subsequent passes
// Propagates min/avg/max through a 4×4 downsample.
// Matches GL2 calclevels4x_fp.glsl without FIRST_PASS.
// buffer(0): float4(invSrcWidth, invSrcHeight, 0, 0)
// ============================================================

fragment float4 fragment_calc_levels(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]],
    constant float4& inv     [[buffer(0)]])
{
    float2 tc = in.texCoord;
    float2 d  = inv.xy;

    float3 current = float3(1.0f, 0.0f, 0.0f);  // min=1, avgSum=0, max=0

    const float2 offsets[16] = {
        float2(-1.5f,-1.5f), float2(-0.5f,-1.5f), float2(0.5f,-1.5f), float2(1.5f,-1.5f),
        float2(-1.5f,-0.5f), float2(-0.5f,-0.5f), float2(0.5f,-0.5f), float2(1.5f,-0.5f),
        float2(-1.5f, 0.5f), float2(-0.5f, 0.5f), float2(0.5f, 0.5f), float2(1.5f, 0.5f),
        float2(-1.5f, 1.5f), float2(-0.5f, 1.5f), float2(0.5f, 1.5f), float2(1.5f, 1.5f)
    };
    for (int i = 0; i < 16; ++i) {
        float3 v = tex.sample(s, tc + d * offsets[i]).rgb;
        current.x  = min(current.x, v.x);
        current.y += v.y;
        current.z  = max(current.z, v.z);
    }
    current.y *= 0.0625f;   // average over 16 samples

    return float4(current, 1.0f);
}

// ============================================================
// Luminance temporal blend
// Blends the freshly-computed luminance (3 %) with the
// accumulated value (97 %) for smooth auto-exposure adaptation.
// Matches GL2: alpha=0.03, blend = SRC_ALPHA + ONE_MINUS_SRC_ALPHA.
// buffer(0): float4(blendAlpha, 0, 0, 0)   (0.03 for float, 0.1 for non-float)
// ============================================================

fragment float4 fragment_lum_blend(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> lumTex  [[texture(0)]],   // current (new) raw luminance  1×1
    texture2d<float> prevTex [[texture(1)]],   // previous accumulated         1×1
    sampler          s       [[sampler(0)]],
    constant float4& params  [[buffer(0)]])
{
    float4 newLum  = lumTex.sample(s,  float2(0.5f, 0.5f));
    float4 prevLum = prevTex.sample(s, float2(0.5f, 0.5f));
    float  alpha   = params.x;                 // 0.03
    return mix(prevLum, newLum, alpha);
}

// ============================================================
// Filmic tonemapping helper (Uncharted 2 / Hable curve)
// Matches GL2 tonemap_fp.glsl FilmicTonemap().
// ============================================================

static float FilmicTonemap(float x)
{
    const float SS  = 0.22f;   // Shoulder Strength
    const float LS  = 0.30f;   // Linear Strength
    const float LA  = 0.10f;   // Linear Angle
    const float TS  = 0.20f;   // Toe Strength
    const float TAN = 0.01f;   // Toe Angle Numerator
    const float TAD = 0.30f;   // Toe Angle Denominator
    return ((x * (SS * x + LA * LS) + TS * TAN) /
            (x * (SS * x + LS)      + TS * TAD)) - TAN / TAD;
}

// ============================================================
// Tonemap + optional SSAO composite
// Matches GL2 tonemap_fp.glsl.
//
// texture(0) = HDR scene color
// texture(1) = luminance map (min/avg/max encoded, 1×1)
// texture(2) = SSAO map (r = occlusion, 1=lit, 0=occluded)
//              ignored when params.useSSAO == 0
// buffer(0)  = TonemapParams
// ============================================================

struct TonemapParams {
    float4 colorScale;          // xyz = exp2(exposure) multiplier, w = 1
    float2 autoExposureMinMax;  // clamp range for log-luminance (GL2 u_AutoExposureMinMax)
    float  toneMin;             // GL2 u_ToneMinAvgMaxLinear.x
    float  toneAvgFactor;       // GL2 u_ToneMinAvgMaxLinear.y
    float  invWhite;            // 1/FilmicTonemap(toneMax-toneMin) for white normalisation
    float  useSSAO;             // 1.0 if SSAO composite enabled
    float2 _pad;
};

fragment float4 fragment_tonemap(
    FullscreenVOut in         [[stage_in]],
    texture2d<float> hdrTex   [[texture(0)]],
    texture2d<float> lumTex   [[texture(1)]],
    texture2d<float> ssaoTex  [[texture(2)]],
    sampler          s        [[sampler(0)]],
    constant TonemapParams& p [[buffer(0)]])
{
    float4 color = hdrTex.sample(s, in.texCoord) * p.colorScale;

    // Decode accumulated luminance map.
    // GL2: logMinAvgMaxLum = clamp(minAvgMax * 20 - 10, -maxExp, -minExp)
    float3 minAvgMax = lumTex.sample(s, in.texCoord).rgb;
    float3 logLum    = clamp(minAvgMax * 20.0f - 10.0f,
                             -p.autoExposureMinMax.y,
                             -p.autoExposureMinMax.x);

    float invAvgLum = p.toneAvgFactor * exp2(-logLum.y);

    color.rgb = color.rgb * invAvgLum - float3(p.toneMin);
    color.rgb = max(float3(0.0f), color.rgb);

    color.r = FilmicTonemap(color.r);
    color.g = FilmicTonemap(color.g);
    color.b = FilmicTonemap(color.b);

    color.rgb = clamp(color.rgb * p.invWhite, 0.0f, 1.0f);

    // Optional SSAO multiplicative composite (matches GL2 FBO_Blit with
    // GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO applied to SSAO result).
    if (p.useSSAO > 0.5f) {
        float ao = ssaoTex.sample(s, in.texCoord).r;
        color.rgb *= ao;
    }

    return color;
}

// ============================================================
// Bloom — bright-pixel extraction
// Extracts HDR pixels that exceed the luminance threshold.
// ============================================================

struct BloomExtractParams {
    float threshold;   // luminance cutoff (typically 1.0 for HDR)
    float strength;    // output multiplier
    float2 _pad;
};

fragment float4 fragment_bloom_extract(
    FullscreenVOut in              [[stage_in]],
    texture2d<float> hdrTex        [[texture(0)]],
    sampler          s             [[sampler(0)]],
    constant BloomExtractParams& p [[buffer(0)]])
{
    float4 color = hdrTex.sample(s, in.texCoord);
    float  lum   = dot(color.rgb, kLuminanceVector);
    // Soft-knee: smoothly extract the over-threshold portion.
    float excess = max(lum - p.threshold, 0.0f);
    float scale  = (lum > 0.0001f) ? (excess / lum) : 0.0f;
    return float4(color.rgb * scale * p.strength, 1.0f);
}

// ============================================================
// Gaussian blur — horizontal  (matches GL2 RB_BlurAxis)
// buffer(0): float4(invWidth, invHeight, strength, 0)
// ============================================================

fragment float4 fragment_gaussian_blur_h(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]],
    constant float4& params  [[buffer(0)]])
{
    float2 tc       = in.texCoord;
    float  invW     = params.x;
    float  strength = params.z;

    // 3-tap approximation used by GL2's RB_BlurAxis.
    const float weights[3] = { 0.227027027f, 0.316216216f, 0.070270270f };
    const float offsets[3] = { 0.0f,         1.3846153846f, 3.2307692308f };

    float4 color = tex.sample(s, tc) * weights[0];
    for (int i = 1; i < 3; ++i) {
        float dx = offsets[i] * invW * strength;
        color += tex.sample(s, tc + float2( dx, 0.0f)) * weights[i];
        color += tex.sample(s, tc + float2(-dx, 0.0f)) * weights[i];
    }
    return color;
}

// ============================================================
// Gaussian blur — vertical  (matches GL2 RB_BlurAxis)
// buffer(0): float4(invWidth, invHeight, strength, 0)
// ============================================================

fragment float4 fragment_gaussian_blur_v(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]],
    constant float4& params  [[buffer(0)]])
{
    float2 tc       = in.texCoord;
    float  invH     = params.y;
    float  strength = params.z;

    const float weights[3] = { 0.227027027f, 0.316216216f, 0.070270270f };
    const float offsets[3] = { 0.0f,         1.3846153846f, 3.2307692308f };

    float4 color = tex.sample(s, tc) * weights[0];
    for (int i = 1; i < 3; ++i) {
        float dy = offsets[i] * invH * strength;
        color += tex.sample(s, tc + float2(0.0f,  dy)) * weights[i];
        color += tex.sample(s, tc + float2(0.0f, -dy)) * weights[i];
    }
    return color;
}

// ============================================================
// SSAO — Screen-Space Ambient Occlusion
// Matches GL2 ssao_fp.glsl exactly.
//
// texture(0) = depth texture (Depth32Float, sampled as .r)
// buffer(0)  = float4(zFar/zNear, zFar, 1/width, 1/height)
//              Note: GL2 passes u_ViewInfo.wz as (1/h,1/w) so w=1/h, z=1/w.
// ============================================================

static float ssao_random(float2 p)
{
    const float2 r = float2(23.1406926327792690f, 2.6651441426902251f);
    return fmod(123456789.0f, 1e-7f + 256.0f * dot(p, r));
}

static float2x2 ssao_randomRotation(float2 p)
{
    float r    = ssao_random(p);
    float sinr = sin(r);
    float cosr = cos(r);
    return float2x2(cosr, sinr, -sinr, cosr);
}

static float ssao_linearDepth(texture2d<float> depthMap, sampler s,
                               float2 tc, float zFarDivZNear)
{
    float sampleZDivW = depthMap.sample(s, tc).r;
    return 1.0f / mix(zFarDivZNear, 1.0f, sampleZDivW);
}

fragment float4 fragment_ssao(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> depthTex [[texture(0)]],
    sampler          s        [[sampler(0)]],
    constant float4& viewInfo [[buffer(0)]])  // x=zFar/zNear, y=zFar, z=1/w, w=1/h
{
    float2 tc           = in.texCoord;
    float  zFarDivZNear = viewInfo.x;
    float  zFar         = viewInfo.y;
    float2 scale        = viewInfo.wz;   // matches GL2: u_ViewInfo.wz = (1/h, 1/w)

    // Poisson-disc sample offsets (GL2 ssao_fp.glsl)
    const float2 poissonDisc[9] = {
        float2(-0.7055767f,  0.196515f),
        float2( 0.3524343f, -0.7791386f),
        float2( 0.2391056f,  0.9189604f),
        float2(-0.07580382f,-0.09224417f),
        float2( 0.5784913f, -0.002528916f),
        float2( 0.192888f,   0.4064181f),
        float2(-0.6335801f, -0.5247476f),
        float2(-0.5579782f,  0.7491854f),
        float2( 0.7320465f,  0.6317794f)
    };

    float  sampleZ  = ssao_linearDepth(depthTex, s, tc, zFarDivZNear);
    float  scaleZ   = zFarDivZNear * sampleZ;
    float2 slope    = float2(dfdx(sampleZ), dfdy(sampleZ))
                    / float2(dfdx(tc.x),   dfdy(tc.y));

    if (length(slope) * zFar > 5000.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float2     offsetScale = scale * 1024.0f / scaleZ;
    float2x2   rmat        = ssao_randomRotation(tc);
    float      invZFar     = 1.0f / zFar;
    float      zLimit      = 20.0f * invZFar;
    float      result      = 0.0f;

    // GL2 uses NUM_SAMPLES = 3
    for (int i = 0; i < 3; ++i) {
        float2 offset    = rmat * poissonDisc[i] * offsetScale;
        float  sampleDiff = ssao_linearDepth(depthTex, s, tc + offset, zFarDivZNear) - sampleZ;
        bool   s1 = abs(sampleDiff) > zLimit;
        bool   s2 = sampleDiff + invZFar > dot(slope, offset);
        result += (s1 || s2) ? 1.0f : 0.0f;
    }
    result /= 3.0f;

    return float4(result, result, result, 1.0f);
}

// ============================================================
// SSAO depth-aware blur  (matches GL2 depthblur_fp.glsl)
//
// texture(0) = SSAO image to blur
// texture(1) = depth texture for edge-stopping
// buffer(0)  = DepthBlurParams
// ============================================================

struct DepthBlurParams {
    float4 viewInfo;        // x=zFar/zNear, y=zFar, z=1/w, w=1/h
    float2 scale;           // texel-space blur half-axis
    float  isHorizontal;    // 1.0 = H pass, 0.0 = V pass
    float  _pad;
};

fragment float4 fragment_depthblur(
    FullscreenVOut in          [[stage_in]],
    texture2d<float> imageTex  [[texture(0)]],
    texture2d<float> depthTex  [[texture(1)]],
    sampler          s         [[sampler(0)]],
    constant DepthBlurParams& p [[buffer(0)]])
{
    float2 tc           = in.texCoord;
    float  zFarDivZNear = p.viewInfo.x;
    float  zFar         = p.viewInfo.y;

    // GL2 gauss coefficients (BLUR_SIZE=4)
    const float gauss[4] = { 0.40f, 0.24f, 0.054f, 0.0044f };

    float2 direction, nudge;
    if (p.isHorizontal > 0.5f) {
        direction = float2(p.scale.x * 2.0f, 0.0f);
        nudge     = float2(0.0f, p.scale.y * 0.5f);
    } else {
        direction = float2(0.0f, p.scale.y * 2.0f);
        nudge     = float2(-p.scale.x * 0.5f, 0.0f);
    }

    float  depthCenter = ssao_linearDepth(depthTex, s, tc, zFarDivZNear);
    float2 slope = float2(dfdx(depthCenter), dfdy(depthCenter))
                 / float2(dfdx(tc.x),        dfdy(tc.y));
    float  depthScale  = clamp(zFarDivZNear * depthCenter / 32.0f, 1.0f, 2.0f);
    direction /= depthScale;
    nudge     /= depthScale;

    float4 result = imageTex.sample(s, tc);
    float  total  = 1.0f;
    float  zLimit = 5.0f / zFar;

    for (int i = 0; i < 2; ++i) {
        for (int j = 1; j < 4; ++j) {
            float2 offset     = direction * (float(j) - 0.25f) + nudge;
            float  depthSmp   = ssao_linearDepth(depthTex, s, tc + offset, zFarDivZNear);
            float  depthExp   = depthCenter + dot(slope, offset);
            float  useSample  = (abs(depthSmp - depthExp) < zLimit) ? 1.0f : 0.0f;
            result += imageTex.sample(s, tc + offset) * useSample;
            total  += useSample;
            nudge   = -nudge;
        }
        direction = -direction;
        nudge     = -nudge;
    }

    return result / total;
}

// ============================================================
// Sun rays — screen-space volumetric light shafts
//
// Implements GL2's radial-blur god-ray effect (RB_SunRays /
// RB_RadialBlur) as a single classic screen-space pass:
// samples along the ray from each pixel toward the sun,
// accumulating exponentially decaying contributions.
//
// texture(0) = quarter-res HDR scene (with sun mask applied)
// buffer(0)  = SunRaysParams
// ============================================================

struct SunRaysParams {
    float2 sunScreenPos;   // sun UV position in [0,1]
    float  strength;       // intensity multiplier
    float  decay;          // per-sample radial decay  (e.g. 0.97)
};

fragment float4 fragment_sun_rays(
    FullscreenVOut in          [[stage_in]],
    texture2d<float> sceneTex  [[texture(0)]],
    sampler          s         [[sampler(0)]],
    constant SunRaysParams& p  [[buffer(0)]])
{
    // Classic god-ray / light-shaft radial blur.
    // Matches the visual intent of GL2's RB_RadialBlur+ping-pong approach.
    const int   NUM_SAMPLES = 64;
    const float exposure    = 0.0034f;
    const float density     = 0.84f;
    const float weight      = 5.65f;

    float2 tc  = in.texCoord;
    float2 dir = (tc - p.sunScreenPos) * (density / float(NUM_SAMPLES));

    float4 color             = float4(0.0f);
    float  illuminationDecay = 1.0f;
    float2 sampleTc          = tc;

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sampleTc -= dir;
        float4 smpl = sceneTex.sample(s, clamp(sampleTc, float2(0.0f), float2(1.0f)));
        smpl        *= illuminationDecay * weight;
        color       += smpl;
        illuminationDecay *= p.decay;
    }

    return color * (exposure * p.strength);
}

// ============================================================
// Depth-of-field bokeh blur  (matches GL2 bokeh_fp.glsl)
//
// 16-sample circular pattern at radius driven by blurRadius.
// texture(0) = source image (quarter-res for performance)
// buffer(0)  = BokehParams
// ============================================================

struct BokehParams {
    float2 invTexRes;   // (1/width, 1/height) of source texture
    float  blurRadius;  // radius in texels
    float  _pad;
};

fragment float4 fragment_dof_blur(
    FullscreenVOut in        [[stage_in]],
    texture2d<float> tex     [[texture(0)]],
    sampler          s       [[sampler(0)]],
    constant BokehParams& p  [[buffer(0)]])
{
    float2 tc  = in.texCoord;
    float2 inv = p.invTexRes * p.blurRadius;

    // Circular 16-sample pattern — matches GL2 bokeh_fp.glsl (active block).
    const float c0 = 1.0f;
    const float c1 = 0.9238795325f;
    const float c2 = 0.7071067812f;
    const float c3 = 0.3826834324f;
    const float c4 = 0.0f;

    float4 color = float4(0.0f);
    color += tex.sample(s, tc + inv * float2( c0,  c4));
    color += tex.sample(s, tc + inv * float2( c1,  c3));
    color += tex.sample(s, tc + inv * float2( c2,  c2));
    color += tex.sample(s, tc + inv * float2( c3,  c1));
    color += tex.sample(s, tc + inv * float2( c4,  c0));
    color += tex.sample(s, tc + inv * float2( c1, -c3));
    color += tex.sample(s, tc + inv * float2( c2, -c2));
    color += tex.sample(s, tc + inv * float2( c3, -c1));
    color += tex.sample(s, tc + inv * float2( c4, -c0));
    color += tex.sample(s, tc + inv * float2(-c0,  c4));
    color += tex.sample(s, tc + inv * float2(-c1,  c3));
    color += tex.sample(s, tc + inv * float2(-c2,  c2));
    color += tex.sample(s, tc + inv * float2(-c3,  c1));
    color += tex.sample(s, tc + inv * float2(-c1, -c3));
    color += tex.sample(s, tc + inv * float2(-c2, -c2));
    color += tex.sample(s, tc + inv * float2(-c3, -c1));
    return color * 0.0625f;   // 1/16
}
