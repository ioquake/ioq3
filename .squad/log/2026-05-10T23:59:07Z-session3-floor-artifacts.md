# Session Log: Floor Rendering Artifacts

**Session**: 3  
**Date**: 2026-05-10T23:59:07Z  
**Agent**: Bishop  

## Summary

Fixed two floor rendering artifacts by correcting sampler binding mode at dynamic light and lightmap texture stages.

1. **Lightmap seam** — ClampToEdge now applied to lightmap stages in `renderBrushModel()`.
2. **Orange dlight splash** — ClampToEdge now applied at all 3 dlight pass binding sites.

Both fixes prevent out-of-range UV coordinates from wrapping or repeating into bright texture centers, which was causing visible artifacts in real-time gameplay.

**Commit**: 36d6ab32
