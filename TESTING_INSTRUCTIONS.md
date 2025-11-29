# Testing the Metal Renderer Lighting Fixes

## Changes Made (3 commits)

### Commit 1: `34941189` - LightGrid Loading Fix
Fixed lightGrid not loading due to wrong world bounds calculation.
- **Before:** 68KB loaded (wrong bounds)
- **After:** 8MB+ loaded (correct bounds from BSP)

### Commit 2: `ebfbe819` - Use Vertex Colors for Lightmapped Surfaces
Fixed world surfaces incorrectly using entity lighting instead of lightmaps.
- **Before:** `rgbGen lightingDiffuse` applied to all surfaces
- **After:** Surfaces with lightmaps use `rgbGen vertex` (lightmap data)

### Commit 3: `a08882bc` - Fix Double-Overbright
Fixed overbright being applied twice, causing washed-out appearance.
- **Before:** Overbright in vertex colors + shader = too bright
- **After:** Overbright only in vertex colors (matches OpenGL2)

## How to Test

### 1. Build and Run
```bash
./just-build.sh
./build-and-run.sh
```

### 2. Load q3dm1
In console:
```
/devmap q3dm1
```

### 3. Visual Comparison

**Floor Surfaces:**
- Should match lighting/brightness of surrounding non-shader textures
- No visible discontinuity between shader and non-shader surfaces

**Wall Surfaces:**
- Same as floors - consistent lighting across all surface types
- Vertical surfaces should be as bright as horizontal ones (if lightmap says so)

**Overall Brightness:**
- Should match OpenGL2 renderer brightness
- Not too dark (old bug: no lightmap)
- Not too bright (old bug: double overbright)

### 4. Side-by-Side Test

**Metal Renderer:**
```
/r_renderer metal
/vid_restart
/devmap q3dm1
```
Take screenshot.

**OpenGL2 Renderer:**
```
/r_renderer opengl2
/vid_restart
/devmap q3dm1
```
Take screenshot.

**Compare:** Lighting should be nearly identical.

## Expected Console Output

When loading q3dm1, you should see:
```
^2Metal: lightGrid loaded successfully (8653 points, bounds 68x68x19)
```

NOT:
```
^3Metal: lightGrid size mismatch (got 68112, expected 8653320)
```

## Known Remaining Issues

If surfaces STILL don't match OpenGL2 after these fixes, check:

1. **Multi-stage shaders** - Some shaders blend lightmaps with base textures in separate stages
2. **Lightmap texture loading** - The lightmap images themselves might have issues
3. **Blend modes** - Stage blend modes might not match OpenGL2
4. **Fog interaction** - Fog might affect lighting differently

## Debug Commands

```
/r_showshaders 1        # Show shader names on surfaces
/r_lightmap 1           # Show only lightmaps (no textures)
/r_overBrightBits 1     # Runtime overbright setting
/r_mapOverBrightBits 2  # Map authoring overbright (read-only)
```

## What Was Actually Wrong

The core problem was a **triple failure**:

1. **LightGrid didn't load** → Entity lighting used default (wrong) values
2. **World surfaces used entity lighting** → Ignored lightmap vertex colors
3. **Overbright applied twice** → Even when using vertex colors, brightness was 2x-4x too high

All three issues are now fixed!
