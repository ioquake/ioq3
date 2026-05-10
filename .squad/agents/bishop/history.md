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
