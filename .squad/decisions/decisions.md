# Decisions

## Decision: Scene Pipeline Colour-Attachment Format Must Track the Active Render Target

**Date**: 2026-05-10  
**Proposed by**: Bishop (Metal GPU rendering engineer)  
**Session**: 2025 — colour-channel swap bug fix  
**Status**: Implemented

### Context

The Metal renderer supports two scene render paths:

| Path | r_hdr | Scene render target | Format |
|------|-------|---------------------|--------|
| HDR (default) | 1 | `hdrColorTarget_` | `RGBA16Float` |
| Non-HDR | 0 | CAMetalLayer drawable | `BGRA8Unorm` |

The 2D/HUD path always targets the drawable (`BGRA8Unorm`).

All ~10 scene pipeline state objects (stage cache, fog, model, additive model, shadow,
dlight, animated dlight, pshadow receivers, flare, and portal sub-pass) were previously
created with a hardcoded `MTL::PixelFormatBGRA8Unorm` colour attachment format.  Metal
requires the pipeline format to match the render target format exactly.  With HDR on (the
default), every scene pipeline targeted an `RGBA16Float` encoder while declaring
`BGRA8Unorm`, producing incorrect colour output (reported as a red/green channel swap on
all 3D world geometry).

### Decision

**Any Metal pipeline state that writes to the main 3D scene encoder must declare the same
pixel format as the active scene colour target.**

Concretely:

1. A single `MTL::PixelFormat sceneColorFormat_` member tracks the current format.
2. `beginFrame()` reads `colorTarget->pixelFormat()` after choosing the render target and
   invalidates all affected pipeline states when the format changes.
3. Every scene pipeline creation function uses `sceneColorFormat_` rather than a
   hardcoded literal.
4. The portal render texture (`portalTexture_`) is created with `sceneColorFormat_` so
   that portal sub-passes share the same pipeline states without introducing a new
   mismatch.
5. Pipelines that do NOT write to the scene encoder — `pipeline2D_`, all `pp*Pso_`
   post-process pipelines, and `pshadowCasterPipeline_` — are explicitly exempt and
   continue to use the format appropriate to their own render target.

### Rationale

- Metal validation (and correct GPU behaviour without validation) requires pipeline
  colour-attachment format == render target format.
- Tracking the format at the one place where the render target is chosen (`beginFrame`)
  and propagating it lazily through the existing ensure*/getStagePipeline functions is
  the minimal, lowest-risk approach.
- Making the portal texture format follow `sceneColorFormat_` is necessary for
  correctness and also future-proofs portal HDR rendering.

### Affected Files

- `code/renderermetal/tr_backend.cpp` — only file changed

---

## Decision: HDR Tonemap Gamma Correction + Lightmap Sampler Mode

**Author**: Bishop (Renderer Engineer)  
**Date**: 2026-05-10  
**Commit**: 49549f18

### Decision 1 — The HDR Tonemap Shader Must Apply sRGB Gamma Correction

**Context**: The Metal CAMetalLayer drawable is configured as `MTL::PixelFormatBGRA8Unorm`. This format stores values verbatim — Metal does **not** perform automatic linear→sRGB conversion on write. The tonemapped output of `fragment_tonemap` is in linear space (Uncharted 2 filmic curve outputs linear). GL2's equivalent (`tonemap_fp.glsl`) always applies `pow(x, 1/2.2)` at the end.

**Decision**: The `fragment_tonemap` function in `postprocess.metal` **must** apply `pow(color.rgb, float3(1.0f / 2.2f))` as its final step before returning, after all SSAO/bloom compositing. This converts the tonemapped linear output to display-referred sRGB.

**Rationale**: 
- Without gamma correction, linear 0.3 (a typical compressed highlight after the filmic curve) displays as `0.3 * 255 ≈ 77/255` on screen instead of the gamma-corrected `76^2.2 ≈ 0.3 → 148/255`. The whole scene appears severely underexposed.
- This is the responsibility of the tonemap shader, not the drawable format. Changing the drawable to `BGRA8Unorm_sRGB` would apply automatic gamma to **all** passes including 2D UI, which is incorrect.
- `r_noPostProcess 1` bypasses the tonemap entirely and blits raw HDR to drawable — scene will appear linear/dark in that mode, which is expected and acceptable for debug use.

**Do Not**:
- Do NOT use `MTL::PixelFormatBGRA8Unorm_sRGB` for the drawable to "fix" this — it would incorrectly gamma-correct the 2D UI passes.
- Do NOT apply gamma correction in the scene (3D) fragment shader — that stage writes to the HDR render target, not the drawable.

### Decision 2 — Lightmap Texture Stages Must Use ClampToEdge Sampler

**Context**: Q3 BSP lightmap textures are individual 128×128 textures (not atlased). Each BSP face has its own lightmap handle. UV coordinates for lightmap sampling are nominally in [0,1] but can overshoot the boundary due to floating-point precision at surface edges.

**Decision**: Any shader stage that samples a lightmap texture (`stageInfo->usesLightmap == true`) **must** use `sceneClampSampler_` (ClampToEdge address mode), not `sceneSampler_` (Repeat address mode).

All `useClamp` sampler-selection expressions in `tr_backend.cpp` must check:
```cpp
bool useClamp = stageInfo && (stageInfo->clampMap || stageInfo->usesLightmap);
```

**Rationale**:
- With Repeat mode, UV values slightly above 1.0 wrap to ~0.0, sampling the opposite edge of the 128×128 tile. That edge typically contains different luminance data, producing a hard-edged brightness seam that outlines the entire surface polygon.
- `stageInfo->clampMap` corresponds to the `clampmap` keyword in Q3 shader scripts and is never set for `$lightmap` stages; a separate check is required.
- `stageInfo->usesLightmap` is the correct flag for lightmap texture stages throughout the codebase.

**Scope**: This applies to all sampler binding sites in `tr_backend.cpp`:  
world surface rendering (2 sites), model multi-pass rendering (3 sites), and other specialized surface types (4 sites). All 9 sites were updated in commit 49549f18.
