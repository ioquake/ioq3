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
