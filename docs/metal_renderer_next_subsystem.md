# Metal Renderer: Next Subsystem Plan

## Selected Subsystem: Front-End Scene Command Buffer
Port the GL2 scene/command ingestion path (`code/renderergl2/tr_scene.c` and `tr_cmds.c`) so the Metal renderer can accept the full `refexport_t` API rather than stubs. This keeps us aligned with the original renderer layering and unlocks real 3D rendering work.

## Goals
- Replace the current no-op implementations of `ClearScene`, `AddRefEntityToScene`, `AddPolyToScene`, `AddLightToScene`, `RenderScene`, etc. in `code/renderermetal/tr_init.cpp` with functional code.
- Mirror the GL2 data structures (`trRefdef_t`, `trRefEntity_t`, dlight batching, poly buffers) in Metal-friendly C++ without breaking file parity.
- Produce a deterministic command buffer that the Metal backend can consume each frame once draw code arrives.

## References
- GL2 sources: `code/renderergl2/tr_scene.c`, `code/renderergl2/tr_cmds.c`, `code/renderergl2/tr_local.h` (definitions of `backEndState_t`, `trRefdef_t`, counters).
- Existing Metal infrastructure: `code/renderermetal/tr_backend.cpp` (frame lifecycle), `code/renderermetal/tr_shader.cpp` (parser outputs), `code/renderermetal/tr_dsa.cpp` (state cache).

## Implementation Steps
1. **Scaffold scene module**
   - Create `code/renderermetal/tr_scene.h/.cpp` to host shared structs, counters, and command-buffer functions.
   - Port minimal subsets of `trRefEntity_t`, `trRefdef_t`, and helper enums that the Metal path needs now.
2. **Wire refexport hooks**
   - Update `tr_init.cpp` so each API call forwards into the new scene module instead of lambdas.
   - Keep signatures identical to GL2 for parity and easier future merges.
3. **Implement collection logic**
   - Mirror GL2 limits (e.g., `MAX_REF_ENTITIES`, `MAX_DLIGHTS`).
   - Store refEntities, polys, and dlights in contiguous arrays; validate overflow behaviour.
   - Add lightweight logging via `ri.Printf` for debugging when limits are exceeded.
4. **Integrate with Metal backend**
   - Expose a method on `MetalRenderer` to accept a completed `trRefdef_t` + command lists.
   - For now, simply stash the data so later passes (lighting/drawing) can iterate.
5. **Testing hooks**
   - Add developer commands (mirroring GL2) like `modellist` only once we have real data.
   - Instrument with assertions to ensure begin/end frame order is respected.

## Risks & Mitigations
- **Large structure parity**: copying every GL2 struct risks drift. Start with the fields the Metal renderer actually consumes; leave TODOs referencing GL2 locations for missing pieces.
- **Memory pressure**: command buffers can be large. Use static arrays matching GL2 constants to keep behaviour predictable.
- **Incremental usefulness**: until we have draw paths, the scene data is unused. Mitigate by adding debug views (e.g., `sceneinfo` command) so we can validate ingestion early.

## Definition of Done
- `MetalBackend_*` refs in `tr_init.cpp` call into real functions rather than empty lambdas.
- New scene module compiles and passes through `BeginFrame`/`EndFrame` without regressions.
- Console command (e.g., `gfxinfo`) can report non-zero counts when entities or polys are added.
- Unit/manual test: load a map, ensure `imagelist`/`gfxinfo` still work and no fatal errors occur.
