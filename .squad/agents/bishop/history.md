# Bishop — History

## Project
ioq3 with custom Metal renderer at code/renderermetal/

## User
Scott (scottmclauchlin)

## Learnings

### Session 1 — Metal Scene Pipeline Format Mismatch (Red/Green Color Swap)

**Bug**: All 3D world geometry rendered with swapped red/green channels; HUD/2D was fine.

**Root cause**: Every 3D scene pipeline state was created with `MTL::PixelFormatBGRA8Unorm`
as its colour attachment format.  When `r_hdr=1` (the default), the scene render encoder
actually writes to `hdrColorTarget_`, which is `MTL::PixelFormatRGBA16Float`.  Metal
requires the pipeline colour attachment format to match the render target format exactly;
a mismatch causes incorrect channel mapping in the GPU output.  2D/HUD pipelines were
correct because they always target the CAMetalLayer drawable (BGRA8Unorm) and their
pipeline declared BGRA8Unorm.

**Fix**: Added `MTL::PixelFormat sceneColorFormat_` member (default BGRA8Unorm).
`beginFrame()` detects any format change and invalidates all scene pipeline states
(stage cache, fog, model, shadow, dlight, animated dlight, pshadow receiver, flare, and
portal texture).  All 10 affected pipeline creation functions now use `sceneColorFormat_`
instead of the hardcoded literal.  The portal render target is also created with
`sceneColorFormat_` so stage pipelines remain valid during portal sub-passes.

**Files changed**: `code/renderermetal/tr_backend.cpp` only.

**Key architecture facts**:
- HDR path: scene → `hdrColorTarget_` (RGBA16Float) → tonemap → drawable (BGRA8Unorm)
- Non-HDR path: scene → drawable (BGRA8Unorm) directly
- 2D/HUD: always → drawable (BGRA8Unorm) — unaffected by `sceneColorFormat_`
- Portal sub-pass: renders geometry into `portalTexture_` using the same stage pipelines,
  so portal texture format must equal `sceneColorFormat_`
- `r_hdr_` defaults to `"1"` (HDR always on unless the user sets it to 0)
- `resetStagePipelineCache()` clears both `stagePipelineCache_` and `modelStagePipelineCache_`

## Decision Captured

**Scene Pipeline Colour-Attachment Format Must Track the Active Render Target** (2026-05-10)

Documented decision that any Metal pipeline state writing to the main 3D scene encoder must declare the same pixel format as the active scene colour target. This is a formal record of the architecture decision made in Session 1.

---

### Session 2 — HDR Darkness + Lightmap Seam Artifacts

**Bugs**:
1. Entire scene severely underexposed / dark; sky dark red instead of bright; surfaces muddy.
2. Visible lighter rectangular patches on floors with hard edges — lightmap tiles at different brightness than neighbours.

**Root cause — Issue 1 (darkness)**:
The `fragment_tonemap` shader (postprocess.metal) outputs linear-space values after the Uncharted 2 filmic curve. The CAMetalLayer drawable is configured as `MTL::PixelFormatBGRA8Unorm` (NOT sRGB), so Metal does NOT automatically apply linear→sRGB conversion. GL2's `tonemap_fp.glsl` applies `pow(x, 1/2.2)` at the end of its tonemap; our Metal shader was missing this step. Tonemapped linear 0.5 reads as display-0.5 (mid-grey correct), but the filmic curve already compresses highlights so a typical "bright surface" came out at ~0.3 linear, which displays very dark without gamma lift.

**Root cause — Issue 2 (lightmap seams)**:
Lightmap textures are individual 128×128 textures per BSP surface face; they are NOT atlased. UV coordinates for each face are nominally in [0,1] but can overshoot by a tiny floating-point epsilon at surface boundaries. All nine `useClamp` sampler-selection sites in the renderer only checked `stageInfo->clampMap`; lightmap stages never set `clampMap` (it's for `clampmap` shader keyword, not `$lightmap`), so every lightmap stage used `sceneSampler_` (Repeat address mode). A UV overshoot past 1.0 with Repeat wraps to the opposite edge of the 128×128 tile. If the opposite edge happens to be significantly brighter (common in Q3 lightmap data), those texels appear as a bright rectangle matching the surface polygon footprint.

**Fixes**:
1. Added `color.rgb = pow(color.rgb, float3(1.0f / 2.2f));` at the end of `fragment_tonemap` in `postprocess.metal`, after the SSAO composite, before `return`. This matches GL2's gamma correction step.
2. Extended every `useClamp` expression in `tr_backend.cpp` (9 sites) to `stageInfo->clampMap || stageInfo->usesLightmap`. Lightmap stages now always use `sceneClampSampler_` (ClampToEdge). Also added the missing `sceneClampSampler_.reset()` to the shutdown cleanup.

**Files changed**: `code/renderermetal/shaders/postprocess.metal`, `code/renderermetal/tr_backend.cpp`.

**Key architecture notes**:
- The BGRA8Unorm drawable has NO automatic gamma conversion — the tonemap shader is solely responsible for gamma correction.
- `r_noPostProcess 1` bypasses all PP (including tonemap + gamma) and blits raw HDR to drawable — useful for debugging; scene will appear linear/dark as expected.
- `r_autoExposure 0` disables luminance adaptation and uses `r_cameraExposure` bias instead.
- `stageInfo->usesLightmap` is the canonical flag for lightmap texture stages; `stageInfo->clampMap` is only for the `clampmap` shader keyword.
- Lightmap textures use `false` for mipmap on upload (correct); their sampler must be ClampToEdge to avoid inter-tile wrapping.

---

### Session 3 — Brush-Model Lightmap Seam + Orange Dlight Blotch

**Bugs**:
1. Lighter/brighter rectangular patch on the floor — lightmap seam artifact.
2. Bright orange splash/blotch on the floor under rocket launcher — incorrect dlight coloring.

**Root cause — Issue 1 (brush-model lightmap seam)**:
`renderBrushModel()` binds `sceneSampler_` (Repeat) once before the per-stage rendering loop, but never updates the sampler inside the loop. Lightmap stages in brush-model surfaces (doors, platforms, func_wall) therefore always use Repeat mode, causing UV overshoots past 1.0 to wrap to the opposite edge of the 128×128 lightmap tile, picking up a brighter texel. The previous fix (Session 2) only covered the world BSP surface path (`drawPacket` lambda and `drawPolyPackets*`); brush-model surfaces had a separate rendering loop that was missed.

**Root cause — Issue 2 (orange dlight splash)**:
All three dlight pass sampler binds (`encodeDlights` for world surfaces, animated model dlight pass, and brush-model dlight pass) used `sampler2D_` which is configured with `SamplerAddressModeRepeat`. The dlight vertex shader computes UV as `dist.xy * (1/radius) + 0.5`. A surface vertex between 1× and 1.5× the light radius away in XY gets UV in range (1.0, 1.5); with Repeat this wraps to (0.0, 0.5), landing near the centre of the 16×16 radial-falloff disk and producing full-brightness dlight contribution far outside the light radius. With an orange/red rocket dlight this created a vivid orange blotch on the floor.

**Fixes**:
1. Added per-stage `useClamp` check in `renderBrushModel()`'s stage loop, immediately after the texture bind: `bool useClamp = stageInfo && (stageInfo->clampMap || stageInfo->usesLightmap)` → routes to `sceneClampSampler_` as needed.
2. Changed all three dlight pass sampler binds from `sampler2D_.get()` to `sceneClampSampler_.get()`. The ClampToEdge mode ensures UVs outside [0,1] sample the black border of the falloff disk (zero intensity) instead of wrapping to a bright region.

**Files changed**: `code/renderermetal/tr_backend.cpp` only.

**Key architecture notes**:
- The `useClamp` sampler-selection pattern (`stageInfo && (stageInfo->clampMap || stageInfo->usesLightmap)`) must be applied at EVERY per-stage texture bind site, not just the world-surface paths.
- `sampler2D_` is the 2D/UI sampler (Repeat mode) — do NOT use it for any 3D scene pass that needs clamped texture sampling (dlights, lightmaps).
- `sceneClampSampler_` (ClampToEdge) is the correct sampler for: lightmap stages, clampmap stages, AND the dlight radial-falloff texture.
- Dlight UV = `dist.xy * (1/radius) + 0.5` is only in [0,1] when the vertex is within the light radius; surfaces outside the radius WILL have UV outside [0,1] and MUST use ClampToEdge.
