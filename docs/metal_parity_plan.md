# Metal Renderer Parity Work

## Stage-Count Collapse Fix Plan

### Symptoms
- Remaining `run_parity.py` failures are dominated by stage-count mismatches (e.g., `textures/base_floor/cybergrate3`, `textures/base_trim/wires01`, `textures/base_wall/glass01`).
- Metal collapses standalone `$lightmap` passes aggressive enough that entire stages disappear, while GL2 commonly keeps those passes alive (often rewritten to `<white>` plus `usesLightmap`).

### Root-Cause Buckets
| Bucket | Example shaders | GL2 behavior | Current Metal Behavior | Why collapse should stay disabled |
| --- | --- | --- | --- | --- |
| Additive / fog-adjusted overlays | `textures/base_floor/cybergrate3`, `textures/base_wall/glass01`, `textures/base_wall/patch10_beat4_drip` | GL2 sets `adjustColorsForFog` for additive/filter blends and short-circuits `CollapseStagesToGLSL`, keeping the later `$lightmap` pass. | Metal still collapses the `$lightmap` stage into the next opaque/alpha stage because we never look at `adjustColorsForFog`. | GL2 intentionally skips collapsing once any stage requires fog color adjustment; we must mirror that gate before folding lightmaps. |
| Shaders with `deformVertexes` / sprite deforms | `textures/base_trim/wires01`, `textures/base_wall/protobanner[_ow]`, `textures/base_trim/wires01_ass` | `shader.numDeforms > 0` forces `CollapseStagesToGLSL` to bail immediately, so GL2 preserves every stage, including `$lightmap`. | Metal ignores `deformVertexes` entirely and collapses, so we drop the explicit lightmap pass. | The vertex program that drives the deform relies on matching stage sequencing; Metal needs to skip collapse whenever the shader script consumed any `deformVertexes` directive. |
| Lightmap-first stacks (no explicit `blendFunc filter`) | `textures/base_floor/diamond_dirty`, `textures/base_wall/shinymetal2`, `textures/base_wall/comp3` | GL2 sets `skip=true` because a standalone lightmap stage is using the default `GL_ONE GL_ZERO` blend (or anything other than `filter`). GL treats this as an actual draw pass. | Metal only checks for *explicit* blends when deciding whether collapse is allowed, so defaulted blends slip through and the `$lightmap` pass gets merged. | We need to treat "missing blend" as `GL_ONE/GL_ZERO` and block collapse unless the stage explicitly matches the `filter` combos that GL expects. |
| Detail-first sequences | `textures/base_floor/diamond_dirty` (detail overlay sandwiched between lightmap + base), `textures/base_floor/diamond_dirty2` | GL preserves the initial `$lightmap` pass so the later detail overlay modulates lit data. | Metal merges the lightmap into the first detail stage and then marks it as `usesLightmap`, which makes the JSON disagree with GL. | Same fix as the "lightmap-first" bucket; once the leading pass is protected, stage ordering will match GL output again. |

### Implementation Plan
1. **Plumb shader-level deform knowledge into `ShaderBuilder`:**
   - Track a `hasShaderDeforms` boolean while parsing (`deformVertexes`, `deformVertexes wave`, `autoSprite`, `tessSize`, etc.).
   - Expose that flag to `ShaderBuilder` and disable `collapseStandaloneLightmapStagesIfAllowed()` up-front when it is set.

2. **Honor fog-adjustment gates:**
   - We already compute `MetalShaderStageInfo::adjustColorsForFog`; extend `noteStageForCollapse` so *any* stage with a non-`None` adjustment flips `allowLightmapCollapse_` to `false`.
   - This mirrors the GL2 guard that bails as soon as additive or alpha blends that require fog compensation are encountered.

3. **Require explicit filter blends before collapsing standalone lightmaps:**
   - Treat the absence of `blendFunc` as `GL_ONE GL_ZERO` (opaque). If a standalone `$lightmap` pass lacks an explicit filter blend, mark it ineligible for collapse.
   - Extend the existing `isFilterBlend` helper to acknowledge both `GL_DST_COLOR GL_ZERO` and `GL_ZERO GL_DST_COLOR` *and* only return true when `blendFuncExplicit` is set.
   - With that stricter check, leading `$lightmap` stages that rely on default blending will remain intact like they do under GL2.

4. **Add regression coverage:**
   - Before landing the heuristics, capture Metal + GL2 JSON snapshots for one shader from each bucket and store them under `code/tools/shader_parser/tests/fixtures/` (or add script-driven asserts) so we catch future regressions.
   - Update `run_parity.py` to print an aggregate summary per bucket once the failure count drops, making it easier to see when the stage-count category is finally resolved.

5. **Validation steps:**
   - Re-run `python3 code/tools/shader_parser/run_parity.py --filters 'textures/base_floor/diamond_dirty|textures/base_trim/wires01|textures/base_wall/glass01'` to verify the deltas.
   - Once the spot-checks are clean, run the full `textures/*` sweep to confirm the untriaged categories are the only remaining failures.

   ## Remaining Failure Category Triage

   | Category | Representative shaders | Symptom | Suspected gap | Next action |
   | --- | --- | --- | --- | --- |
   | **Sky shaders** | `textures/castle/sky_castle1`, `textures/common/nightsky*`, `textures/skies/x*`, `textures/hell/hellsky2goo` | GL2 emits 1–3 sky stages while Metal reports `stageCount: 0`. | Metal parser currently ignores `skyParms`, `cloudHeight`, and the implicit stage generation GL performs for skyboxes. | Implement a dedicated `MetalSkyInfo` block: parse `skyParms`, generate the two cloud stages + farbox stage, and emit JSON so parity recognizes the passes. |
   | **Portal / teleporter stacks** | `textures/ctf/blue_telep`, `textures/ctf/ctf_*flag`, `textures/sfx/beam_waterlight1`, `textures/sfx/glass` | Metal collapses to 3–4 stages while GL2 keeps an extra additive portal/lightmap pass. | Mix of additive blends (`adjustColorsForFog`) plus `alphaGen portal`/`tcMod rotate` sequences require preserving stage order and portal-specific alpha data. | After the collapse heuristics are fixed, teach the parser to map `alphaGen portal` and `rgbGen entity/exactVertex` to their GL2 equivalents so fields match, and add JSON flags for portal surfaces (`isPortal = true`). |
   | **Liquid / deforming water** | `textures/liquids/clear_ripple*`, `textures/liquids/calm_pool2`, `textures/liquids/proto_*`, `textures/liquids/mercury` | Stage counts differ, and several `tcGen` fields (`tcGen environment`) mismatch. | We ignore `deformVertexes wave/bulge` at the shader level and silently drop `tcGen environment` when tcMods are present. Metal therefore rewrites the pass as `tcGen texture`. | a) Reuse the deform guard from the collapse plan so we stop folding away their lightmap stages. b) Extend `parseImageToken` / `parseTcGen` to allow `tcGen environment` alongside tcMods and serialize that choice instead of reverting to `texture`. |
   | **Auto-sprite wires, banners, player skins** | `textures/base_trim/wires01[_ass]`, `textures/base_wall/protobanner*`, `textures/skin/*` | GL2 keeps two passes; Metal collapses to one. | All of these scripts include `deformVertexes` (`autoSprite2`, `bulge`, or `wave`), so GL’s `shader.numDeforms` guard keeps their `$lightmap` passes intact. | Same implementation as Stage-Count Plan step #1: mark shaders with any deform usage and skip the collapse path entirely. |
   | **`surfaceparm nolightmap` diagnostics** | `textures/dont_use/{metal2_2kc,openwindow,web}`, `textures/sfx/fanfx` | Field-level diffs: Metal reports `blendMode=opaque`, `rgbGen identity`, etc., while GL2 honors the scripted `blendFunc`/`alphaMap`/`rgbGen exactVertex`. | The parser does not yet handle `alphaMap`, nor does it honor `rgbGen exactVertex` or `alphaFunc GT0`, so the defaults disagree. | Add `alphaMap` parsing (it should behave like `map` that only feeds the alpha channel), plumb `rgbGen exactVertex` / `identityLighting` values through, and respect non-default `alphaFunc` tokens such as `GT0`. |
   | **SFX fans & animated overlays** | `textures/sfx/fan2`, `textures/sfx/fan3`, `textures/sfx/lightmap`, `textures/sfx/teslacoil` | Some are stage-count mismatches; others have `rgbGen`/`alphaGen` discrepancies. | Many of these rely on additive blends (already covered), but a subset uses `tcGen lightmap` or `rgbGen wave` functions we currently quantize differently. | After additive collapse fixes land, diff the remaining SFX shaders; expect to add explicit handling for `rgbGen wave` serialization (frequency/amplitude) so the JSON matches GL output. |

   **Priorities:** Skies and deform-driven stage collapse blocks are the largest remaining buckets by raw failure count, followed by liquids (tcGen) and the `nolightmap` diagnostics. Portal/teleporter stacks should stabilize once the collapse heuristics stop stripping their lightmap pass, but we still need the `alphaGen portal` plumbing so field diffs disappear.
