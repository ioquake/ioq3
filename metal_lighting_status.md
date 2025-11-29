# Metal Renderer Lighting Implementation Status

## Problem Statement
Certain surfaces in q3dm1 (and likely other maps) have incorrect lighting - they appear darker or differently lit compared to surrounding textures. This is NOT related to dynamic lights from weapons; it's a static lighting issue with how certain shader surfaces are rendered.

## What We've Implemented So Far

### 1. Complete Lighting System (tr_light.cpp)
- ✅ `R_LoadLightGrid()` - Loads BSP lightGrid data
- ✅ `R_SetupEntityLightingGrid()` - Trilinear interpolation of 8 grid samples
- ✅ `R_SetupEntityLighting()` - Applies lightGrid + dynamic lights to entities
- ✅ `R_LightForPoint()` - Public API for sampling lighting at a point
- ✅ Integrated into backend initialization/shutdown
- ⚠️  WARNING: "lightGrid size mismatch" error suggests lightGrid loading may have issues

### 2. Dynamic Light Rendering (dlight.metal + drawDynamicLights)
- ✅ DlightUniforms structure
- ✅ Dlight shader with radial falloff
- ✅ Dlight pipeline with additive blending
- ✅ drawDynamicLights() function
- ✅ Proper render order (geometry → dlights → fog)
- ℹ️  This is working but irrelevant to the surface lighting problem

### 3. Entity Lighting Shader Support (scene.metal)
- ✅ EntityLightingParams structure in shader
- ✅ Fragment shader code for rgbGen lightingDiffuse (lines 200-207)
- ✅ EntityLightingParams uploaded to buffer slot 2
- ✅ Added case for MetalRGBGen::LightingDiffuse → rgbGenType = 3.0f

## Current Issue: Surfaces Still Look Wrong

Despite implementing all the above, surfaces with lighting issues in q3dm1 still don't match surrounding textures.

## Known Problems to Investigate

### 1. LightGrid Loading Error
```
^3Metal: lightGrid size mismatch (got 68112, expected 8653320)
```
**Location:** `tr_backend.cpp` lines 1638-1714 in `loadWorldMap()`

**The Issue:** The lightGrid data size calculation is wrong. We're getting 68KB when expecting 8MB.

**What to check:**
- Line 1698: `gridBounds` calculation may be incorrect
- The expected size formula: `gridBounds[0] * gridBounds[1] * gridBounds[2] * 8` (8 bytes per grid point)
- Actual LUMP_LIGHTGRID size from BSP doesn't match expected
- This means we're probably NOT loading the lightGrid correctly at all!

### 2. EntityLightingParams Default Values
**Location:** `tr_backend.cpp` lines 3560-3573

Currently using:
```cpp
ambientLight = (255, 255, 255)  // Full white
directedLight = (0, 0, 0)       // No directional light
```

**Problem:** This is "identity" lighting which makes everything full brightness. For surfaces with `rgbGen lightingDiffuse`, we should be:
- Sampling the lightGrid at the surface position
- Using that lighting data instead of identity values

### 3. World Surface Lighting
World surfaces (BSP geometry) should be lit primarily by **lightmaps**, not entity lighting. Entity lighting is for dynamic entities (models, sprites).

**Question:** Are the problematic surfaces actually world geometry or shader effects?
- If world geometry → should use lightmaps (already in vertex colors)
- If shader effects → might need different lighting approach

### 4. Shader Parser - rgbGen lightingDiffuse
**Location:** `tr_shader.cpp` line 887-890

Check if the shader parser is correctly detecting and setting `MetalRGBGen::LightingDiffuse` when it encounters `rgbGen lightingDiffuse` in shader scripts.

**To verify:** Add logging to see if any shaders are actually being parsed with LightingDiffuse type.

## Recommended Next Steps

### Step 1: Fix LightGrid Loading
The size mismatch error suggests lightGrid isn't loading correctly. This is likely the root cause.

**Action:**
1. Check the `gridBounds` calculation in `loadWorldMap()`
2. Verify gridSize parsing from entities lump
3. Compare with OpenGL2 renderer's `R_LoadLightGridArray()` in `tr_bsp.c`
4. The grid might be stored as 8-bit or 16-bit data, not always the expected format

### Step 2: Sample LightGrid for Surfaces
Instead of using identity lighting for all surfaces, sample the lightGrid at each surface's position.

**Pseudocode:**
```cpp
// For each surface/packet
vec3_t surfaceCenter = calculateSurfaceCenter(packet);
vec3_t ambientLight, directedLight, lightDir;
R_LightForPoint(surfaceCenter, ambientLight, directedLight, lightDir);

EntityLightingParams lighting;
lighting.ambientLight = ambientLight;
lighting.directedLight = directedLight;
lighting.lightDir = lightDir;
// Upload to shader
```

### Step 3: Debug Shader Detection
Add logging to see which shaders use lightingDiffuse:

```cpp
// In buildStageParams around line 3625
if (stageInfo->rgbGen.type == MetalRGBGen::LightingDiffuse) {
    ri_.Printf(PRINT_ALL, "Shader uses lightingDiffuse: %s\n", shaderName);
}
```

### Step 4: Compare with OpenGL2
**Files to compare:**
- `renderergl2/tr_bsp.c` - How lightGrid is loaded
- `renderergl2/tr_shade.c` - How lighting is applied to surfaces
- `renderergl2/glsl/lightall_vp.glsl` - Vertex shader lighting

## Key Code Locations

### LightGrid Loading
- `tr_backend.cpp:1638-1714` - `loadWorldMap()` BSP parsing
- `tr_light.cpp:79-159` - `R_LoadLightGrid()`

### Entity Lighting Upload
- `tr_backend.cpp:3560-3573` - Where EntityLightingParams is uploaded
- `tr_backend.cpp:3625-3638` - Where rgbGenType is set

### Shader Fragment
- `scene.metal:200-207` - lightingDiffuse calculation
- `scene.metal:178` - EntityLightingParams buffer binding

### Shader Parsing
- `tr_shader.cpp:887-890` - rgbGen lightingDiffuse parsing

## Git Commits
Current state: commit `a8261f44` "Metal: Implement dynamic light rendering"

## Test Case
Map: q3dm1
Issue: Certain surfaces appear darker/wrongly lit compared to surrounding textures
Not related to: Weapon flashes, dynamic lights, or animated effects

## Debug Commands
```bash
# Run with verbose logging
./build/Release/ioquake3.app/Contents/MacOS/ioquake3 +set r_renderer metal +set developer 1 +devmap q3dm1

# Look for these in output:
# - "lightGrid size mismatch" - confirms loading issue  
# - "Shader uses lightingDiffuse" - if you add logging
# - "MetalScene: processed X entities, Y polys, Z lights"
```

## Questions to Answer
1. Is the lightGrid actually loading? (Fix the size mismatch first)
2. Which specific surfaces in q3dm1 look wrong? (Get shader names)
3. Do those shaders use rgbGen lightingDiffuse?
4. If yes, is the lighting data being sampled from the correct location?
5. Are the lighting values reasonable? (0-255 range, not all white/black)

## Most Likely Root Cause
The lightGrid size mismatch error (68KB vs 8MB) suggests **the lightGrid is not loading at all**. This means all the lighting calculations are using fallback/default values, which would explain why surfaces don't look right.

**Priority: Fix lightGrid loading first, everything else depends on it.**
## Summary for Context Resumption

Current Commit: 271f2533
Branch: metal

## The Core Problem
Surfaces in q3dm1 don't match lighting of surrounding textures. They appear darker/wrong.

## Root Cause (High Confidence)
LightGrid is NOT loading correctly - error shows 68KB loaded vs 8MB expected.
This means ALL lighting calculations are using fallback/default values.

## What to Fix Next
1. **PRIORITY**: Fix lightGrid loading in tr_backend.cpp:1638-1714
   - Check gridBounds calculation (line ~1698)
   - Compare with renderergl2/tr_bsp.c R_LoadLightGridArray()
   - BSP lightGrid format might be different than expected

2. Sample lightGrid for surfaces instead of identity lighting (tr_backend.cpp:3560-3573)

3. Add debug logging to verify shaders are detected as using lightingDiffuse

## Quick Reference
- LightGrid code: tr_light.cpp and tr_backend.cpp:1638-1714
- Entity lighting upload: tr_backend.cpp:3560-3573
- Shader detection: tr_backend.cpp:3625-3638
- Fragment shader: scene.metal:200-207

The document above has full details.
