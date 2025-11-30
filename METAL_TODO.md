# Metal Renderer Completion Plan

**Last Updated:** 2025-11-29  
**Branch:** metal  

---

## Phase 1: Low-Hanging Fruit (Quick Fixes)

### 1.1 LOD Selection
- [x] **Status:** ALREADY IMPLEMENTED
- **File:** `tr_backend.cpp:7546` - `computeModelLod()`
- **Note:** LOD selection was already implemented! The hardcoded LOD 0 in `lerpTag()` is intentional (matches GL2 behavior - tags should be consistent across LODs).

### 1.2 EntityTranslate tcMod
- [x] **Status:** Complete
- **File:** `tr_backend.cpp:546`
- **Fix Applied:** Added optional `refEntity_t*` parameter to `ComputeTCModMatrix()` and `computeTCModParams()`. EntityTranslate now uses `entity->shaderTexCoord[2]` for translation.
- **Commit:** (pending)

### 1.3 MetalScene_EndFrame Placeholder
- [x] **Status:** Complete
- **File:** `tr_scene.cpp:78`
- **Fix Applied:** Updated comment to be more specific about future GPU timing capture.
- **Commit:** (pending)

---

## Phase 2: Culling & Clipping

### 2.1 Clip Handling for Models
- [x] **Status:** Complete (documented as intentional)
- **File:** `tr_backend.cpp:6015`
- **Note:** The `fullyVisible` placeholder was replaced with documentation explaining that per-surface clip checks aren't needed since we do frustum culling at model level.
- **Commit:** (pending)

### 2.2 Dlight Surface Culling
- [x] **Status:** Complete
- **File:** `tr_backend.cpp:5509`
- **Fix Applied:** Added distance-based culling. Surfaces are skipped if first vertex is more than 2x light radius away.
- **Commit:** (pending)

---

## Phase 3: Portal/Mirror Support

### 3.1 Portal View Exclusion
- [ ] **Status:** Not started
- **File:** `tr_backend.cpp:7187`
- **Current:** `personalModel = isThirdPerson` without checking portal views
- **Fix:** Check if current view is a portal/mirror view
- **Details:**
  - Add a flag like `isPortalView` to scene/render state
  - personalModel = isThirdPerson && !isPortalView
  - This ensures first-person model appears in mirrors
- **Effort:** ~10 lines
- **BLOCKED:** Requires portal rendering system to be implemented first

### 3.2 Mirror Handling
- [ ] **Status:** Not started  
- **File:** `tr_backend.cpp:6394-6399`
- **Current:** Commented out
- **Fix:** Uncomment and implement mirror axis flip
- **Details:**
  - Check backEnd.viewParms.isMirror or equivalent flag
  - If mirrored: VectorSubtract(vec3_origin, left, left) to flip sprite
  - May need to also reverse face winding for proper culling
- **Effort:** ~5 lines
- **BLOCKED:** Requires portal/mirror rendering system to be implemented first

### 3.3 Portal Rendering System (NEW - Large Feature)
- [ ] **Status:** Not started
- **Scope:** Major feature - multi-pass rendering through portals/mirrors
- **Required for:** 3.1, 3.2
- **Details:**
  - Detect portal surfaces during scene traversal
  - Set up new view frustum from portal perspective
  - Render recursively through portal (with depth limit)
  - Add `isPortal` and `isMirror` flags to SceneCamera
  - Composite portal render into main scene
- **Effort:** 200-400 lines, significant complexity

---

## Phase 4: LightGrid Fix (Critical for Lighting)

### 4.1 Fix LightGrid Loading
- [x] **Status:** VERIFIED WORKING
- **File:** `tr_backend.cpp:2070-2120`
- **Note:** LightGrid loading was already working correctly! Debug output shows:
  - q3dm1: 8514 grid points (22x43x9), 68112 bytes ✓
  - World bounds read correctly from LUMP_MODELS
  - `R_LoadLightGrid()` called successfully
- The "size mismatch" error in the status doc was from an older version or different map.

### 4.2 Sample LightGrid for Surfaces
- [x] **Status:** ALREADY IMPLEMENTED
- **File:** `tr_backend.cpp:5682-5714` - `setupEntityLighting()`
- **Note:** Entity lighting already calls `R_LightForPoint()` to sample the lightgrid!
  - Samples at entity's lighting origin
  - Applies bonus minimum light for visibility
  - Results passed to shader via EntityLightingParams

---

## Phase 5: Deforms in Dynamic Lights

### 5.1 Deforms for Dlight Surfaces
- [ ] **Status:** Not started
- **File:** `tr_backend.cpp:5498`
- **Current:** `uniforms.deformGen = 0` (no deforms)
- **Fix:** Apply same deform logic as regular surfaces
- **Details:**
  - Copy deform parameters from shader to DlightUniforms
  - dlight.metal shader already has DeformPosition() function
  - Just need to populate the uniform values
- **Effort:** ~40 lines

---

## Implementation Order (Recommended)

| Order | Task | Risk | Effort | Impact | Status |
|-------|------|------|--------|--------|--------|
| 1 | 1.3 EndFrame placeholder | None | 5 min | Cleanup | ✅ |
| 2 | 1.2 EntityTranslate tcMod | Low | 15 min | Minor visual | ✅ |
| 3 | 1.1 LOD selection | Low | 20 min | Performance | ✅ (already done) |
| 4 | 2.1 Clip handling | Low | 15 min | Performance | ✅ (documented) |
| 5 | 2.2 Dlight culling | Low | 30 min | Performance | ✅ |
| 6 | 3.2 Mirror flip | Medium | 15 min | Visual | ❌ Blocked |
| 7 | 3.1 Portal view exclusion | Medium | 20 min | Visual | ❌ Blocked |
| 8 | **4.1 LightGrid loading** | **High** | **2-3 hrs** | **Critical** | ✅ (verified working) |
| 9 | 4.2 LightGrid sampling | Medium | 1 hr | Visual | ✅ (already done) |
| 10 | 5.1 Dlight deforms | Medium | 45 min | Visual | ✅ |

**Legend:** ⬜ Not started | 🔄 In progress | ✅ Complete | ❌ Blocked

---

## Files to Modify

| File | Changes |
|------|---------|
| `tr_backend.cpp` | ~~LOD~~, ~~tcMod~~, ~~clip~~, ~~dlight cull~~, ~~lightgrid load~~, ~~lightgrid sample~~, ~~dlight deforms~~, portal, mirror |
| `tr_scene.cpp` | ~~EndFrame cleanup~~ |
| `tr_light.cpp` | ~~Verified working~~ |

---

## Testing Checkpoints

- [x] **After Phase 1:** Run game, ensure no crashes, verify LOD switching with `r_lodBias`
- [x] **After Phase 2:** Run q3dm17 (lots of lights), check performance with `r_speeds 1`
- [ ] **After Phase 3:** Find a mirror in a map (q3dm0?), verify player model appears (BLOCKED - requires portal system)
- [x] **After Phase 4:** Check q3dm1 lighting - lightGrid loads successfully
- [ ] **After Phase 5:** Fire rocket near deforming surface, verify light interacts correctly

---

## Session Log

### 2025-01-29
- Created this tracking document
- Identified all TODOs/stubs/shortcuts in Metal renderer
- Pushed branch to personal fork as `nu-metal`
- **Implemented Phase 1 & 2:**
  - 1.1 LOD selection - discovered already implemented in `computeModelLod()`
  - 1.2 EntityTranslate tcMod - added entity parameter to `ComputeTCModMatrix()` 
  - 1.3 EndFrame placeholder - updated comment
  - 2.1 Clip handling - documented as intentional design
  - 2.2 Dlight culling - added distance-based surface culling
- **Phase 3 (Portals/Mirrors):** Marked as BLOCKED - requires full portal rendering system (200-400 lines, major feature)
- **Phase 4 VERIFIED WORKING:**
  - 4.1 LightGrid loading - q3dm1 loads 8514 grid points correctly (22x43x9 = 68112 bytes)
  - 4.2 LightGrid sampling - `setupEntityLighting()` already calls `R_LightForPoint()` to sample grid
  - The "size mismatch" error referenced in `metal_lighting_status.md` is from old version or different map
- **Remaining:** 5.1 Dlight deforms, then portal system (major feature)

### 2025-11-30
- **Phase 5.1 Dlight deforms - IMPLEMENTED:**
  - Modified `drawDynamicLights()` in `tr_backend.cpp` to extract deform info per surface
  - Computes `deformGen` (DGEN_WAVE_SIN, DGEN_WAVE_SQUARE, etc. or DGEN_BULGE) from shader
  - Populates `deformParams[5]` (base, amplitude, phase, frequency, spread)
  - Uploads to GPU via `DlightUniforms` - shader's `DeformPosition()` already handles GPU-side deform
  - Optimizes by tracking `lastDeformGen` to avoid redundant uniform uploads for non-deforming surfaces

**ALL QUICK-FIX ITEMS COMPLETE!**
- Only remaining work: Portal/Mirror system (major feature, ~200-400 lines)

---

## Reference Documents

- `metal_lighting_status.md` - Detailed analysis of lighting issues
- `docs/metal_parity_plan.md` - OpenGL2 parity planning
- `docs/metal_renderer_next_subsystem.md` - Next subsystem planning
