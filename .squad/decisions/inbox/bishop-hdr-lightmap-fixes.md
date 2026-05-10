# Decision: HDR Tonemap Gamma Correction + Lightmap Sampler Mode

**Author**: Bishop (Renderer Engineer)  
**Date**: 2026-05-10  
**Commit**: 49549f18

---

## Decision 1 — The HDR Tonemap Shader Must Apply sRGB Gamma Correction

### Context
The Metal CAMetalLayer drawable is configured as `MTL::PixelFormatBGRA8Unorm`. This format stores values verbatim — Metal does **not** perform automatic linear→sRGB conversion on write. The tonemapped output of `fragment_tonemap` is in linear space (Uncharted 2 filmic curve outputs linear). GL2's equivalent (`tonemap_fp.glsl`) always applies `pow(x, 1/2.2)` at the end.

### Decision
The `fragment_tonemap` function in `postprocess.metal` **must** apply `pow(color.rgb, float3(1.0f / 2.2f))` as its final step before returning, after all SSAO/bloom compositing. This converts the tonemapped linear output to display-referred sRGB.

### Rationale
- Without gamma correction, linear 0.3 (a typical compressed highlight after the filmic curve) displays as `0.3 * 255 ≈ 77/255` on screen instead of the gamma-corrected `76^2.2 ≈ 0.3 → 148/255`. The whole scene appears severely underexposed.
- This is the responsibility of the tonemap shader, not the drawable format. Changing the drawable to `BGRA8Unorm_sRGB` would apply automatic gamma to **all** passes including 2D UI, which is incorrect.
- `r_noPostProcess 1` bypasses the tonemap entirely and blits raw HDR to drawable — scene will appear linear/dark in that mode, which is expected and acceptable for debug use.

### Do Not
- Do NOT use `MTL::PixelFormatBGRA8Unorm_sRGB` for the drawable to "fix" this — it would incorrectly gamma-correct the 2D UI passes.
- Do NOT apply gamma correction in the scene (3D) fragment shader — that stage writes to the HDR render target, not the drawable.

---

## Decision 2 — Lightmap Texture Stages Must Use ClampToEdge Sampler

### Context
Q3 BSP lightmap textures are individual 128×128 textures (not atlased). Each BSP face has its own lightmap handle. UV coordinates for lightmap sampling are nominally in [0,1] but can overshoot the boundary due to floating-point precision at surface edges.

### Decision
Any shader stage that samples a lightmap texture (`stageInfo->usesLightmap == true`) **must** use `sceneClampSampler_` (ClampToEdge address mode), not `sceneSampler_` (Repeat address mode).

All `useClamp` sampler-selection expressions in `tr_backend.cpp` must check:
```cpp
bool useClamp = stageInfo && (stageInfo->clampMap || stageInfo->usesLightmap);
```

### Rationale
- With Repeat mode, UV values slightly above 1.0 wrap to ~0.0, sampling the opposite edge of the 128×128 tile. That edge typically contains different luminance data, producing a hard-edged brightness seam that outlines the entire surface polygon.
- `stageInfo->clampMap` corresponds to the `clampmap` keyword in Q3 shader scripts and is never set for `$lightmap` stages; a separate check is required.
- `stageInfo->usesLightmap` is the correct flag for lightmap texture stages throughout the codebase.

### Scope
This applies to all sampler binding sites in `tr_backend.cpp`:  
world surface rendering (2 sites), model multi-pass rendering (3 sites), and other specialized surface types (4 sites). All 9 sites were updated in commit 49549f18.
