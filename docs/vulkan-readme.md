# Vulkan Renderer

The Vulkan renderer is an alternate renderer for ioquake3 based on
[vkQuake3](https://github.com/suijingfeng/vkQuake3) (Sui Jingfeng's fork),
which itself is derived from
[Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).
It replaces the OpenGL fixed-function pipeline with Vulkan, providing a modern
graphics API path while maintaining compatibility with existing Quake 3 content.


-------------------------------------------------------------------------------
  FEATURES
-------------------------------------------------------------------------------

  - Compatible with vanilla Quake 3 and most mods.
  - Vulkan 1.0 rendering backend.
  - Pre-compiled SPIR-V shaders (single-texture and multi-texture paths).
  - Specialization constants for alpha testing, clipping planes, and texture
    blend operations -- avoiding redundant shader permutations.
  - Hardware clip distance support where available.
  - Push constants for per-primitive MVP and clipping plane data.
  - Single command buffer, single render pass architecture.
  - Self-contained renderer with its own image loaders (BMP, TGA, JPG, PNG,
    PCX) and font rendering.


-------------------------------------------------------------------------------
  BUILDING
-------------------------------------------------------------------------------

The Vulkan renderer is enabled by default.  To disable it, pass
`-DBUILD_RENDERER_VULKAN=OFF` to CMake.

    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make renderer_vulkan

This produces `renderer_vulkan_<arch>.so` (Linux), `renderer_vulkan_<arch>.dll`
(Windows), or a framework bundle (macOS).

### Shader compilation

The SPIR-V shaders are shipped pre-compiled as C byte arrays in
`code/renderer_vulkan/shaders/Compiled/`.  You do not need the Vulkan SDK to
build the renderer.

To regenerate the shaders from GLSL source (requires `glslangValidator`):

    cd code/renderer_vulkan/shaders
    ./compile.sh


-------------------------------------------------------------------------------
  INSTALLATION
-------------------------------------------------------------------------------

For *nix:

1. Build ioquake3 as usual.  The Vulkan renderer shared library will be
   produced alongside the OpenGL renderers.

2. Copy the renderer library into your Quake 3 install directory alongside
   the client binary.

For Win32:

1. Have a Quake 3 install, fully patched.

2. Copy the following files into Quake 3's install directory:

     ioquake3.x86_64.exe
     renderer_vulkan_x86_64.dll

   These can be found in the build output directory after compiling.


-------------------------------------------------------------------------------
  RUNNING
-------------------------------------------------------------------------------

1. Start ioquake3.

2. Open the console (the default key is tilde ~) and type:
   `/cl_renderer vulkan` and press enter,
   `/vid_restart` then press enter again.

3. To switch back to an OpenGL renderer:
   `/cl_renderer opengl1` (or `opengl2`) and press enter,
   `/vid_restart` then press enter again.


-------------------------------------------------------------------------------
  CONSOLE COMMANDS
-------------------------------------------------------------------------------

  * `pipelineList`      - List all created Vulkan pipelines.
  * `gpuMem`            - Show GPU image memory allocation.
  * `printOR`           - Print the value of backend.or.
  * `displayResoList`   - List display resolutions your monitor supports.


-------------------------------------------------------------------------------
  CVARS
-------------------------------------------------------------------------------

Cvars for display:

  * `r_mode`                - Video mode selection.
                                -2  - Use desktop resolution. (default)
                                -1  - Use r_customwidth / r_customheight.
                                0-28 - Predefined modes (e.g. 4 = 800x600,
                                       12 = 1280x720, 23 = 1920x1080).

  * `r_fullscreen`          - Run in fullscreen mode.
                                0 - Windowed.
                                1 - Fullscreen. (default)

  * `r_allowResize`         - Allow the window to be resized.
                                0 - No. (default)
                                1 - Yes.

  * `r_displayRefresh`      - Display refresh rate in Hz.
                                60 - Default.

  * `r_displayIndex`        - Select which display/monitor to use.
                                0 - Primary display. (default)

  * `r_customwidth`         - Custom resolution width (when r_mode -1).
                                960 - Default.

  * `r_customheight`        - Custom resolution height (when r_mode -1).
                                540 - Default.

Cvars for image quality:

  * `r_picmip`              - Texture detail reduction level (0 = full quality).
                                0   - Highest quality.
                                1   - Default.
                                2-8 - Progressively lower quality.

  * `r_intensity`           - Linear intensity multiplier applied to textures.
                              Requires vid_restart.
                                1.0 - No change.
                                1.5 - Default. Good starting point.
                                2.0 - Brighter.

  * `r_gamma`               - Non-linear gamma correction applied to textures
                              before upload to GPU.  Requires vid_restart.
                                1.0 - No correction. (default)
                                1.5 - Brighter.

  * `r_simpleMipMaps`       - Use simple box filter for mipmap generation.
                                0 - No.
                                1 - Yes. (default)

  * `r_subdivisions`        - Curve tessellation quality.
                                4 - Default. Lower = smoother curves.

  * `r_vertexLight`         - Use vertex lighting instead of lightmaps.
                              Better performance, lower visual quality.
                              Requires vid_restart.
                                0 - No. (default)
                                1 - Yes.

  * `r_mapOverBrightBits`   - Overbrightening of lightmap data.
                              Requires vid_restart.
                                1 - Default.

Cvars for rendering features:

  * `r_dynamiclight`        - Enable dynamic lights.
                                0 - No.
                                1 - Yes. (default)

  * `r_flares`              - Enable light flares.
                                0 - No. (default)
                                1 - Yes.

  * `r_facePlaneCull`       - Enable backface culling on planar surfaces.
                                0 - No.
                                1 - Yes. (default)

  * `r_inGameVideo`         - Enable in-game video playback.
                                0 - No.
                                1 - Yes. (default)

  * `cg_shadows`            - Shadow rendering mode.
                                0 - None.
                                1 - Blur shadows. (default)
                                2 - Stencil volume shadows.
                                3 - Black planar projection shadows.

Cvars for railgun effects:

  * `r_railWidth`           - Rail trail width.  Default 16.
  * `r_railCoreWidth`       - Rail trail core width.  Default 6.
  * `r_railSegmentLength`   - Rail trail segment length.  Default 32.

Cvars for debugging (cheat-protected):

  * `r_speeds`              - Display rendering performance stats.
                                0 - Off. (default)
                                1 - On.

  * `r_showtris`            - Wireframe rendering.
  * `r_shownormals`         - Draw surface normals.
  * `r_showsky`             - Force sky in front of all surfaces.
  * `r_nocull`              - Disable frustum culling.
  * `r_novis`               - Disable PVS culling.
  * `r_drawworld`           - Toggle world rendering.
  * `r_drawentities`        - Toggle entity rendering.
  * `r_lightmap`            - Render lightmaps only.
  * `r_fullbright`          - Skip lightmap pass entirely.
  * `r_singleShader`        - Force default shader on all world surfaces.
  * `r_lockpvs`             - Lock the PVS to the current viewpoint.
  * `r_noportals`           - Disable portal rendering.
  * `r_verbose`             - Enable verbose renderer debug output.
  * `r_znear`               - Near Z clip plane distance.  Default 4.


-------------------------------------------------------------------------------
  ARCHITECTURE
-------------------------------------------------------------------------------

The Vulkan renderer is loaded as a shared library via `cl_renderer vulkan`.
It exports the standard `GetRefAPI` entry point (REF_API_VERSION 8) and is
ABI-compatible with the OpenGL renderers.

### Geometry

Quake 3's renderer prepares geometry in `tess.xyz` and `tess.indexes` arrays.
The Vulkan backend appends this data to host-visible vertex and index buffers
each frame.  Typically up to 500 KB of vertex data and 100 KB of index data
are written per frame.

### Descriptor sets

One descriptor set per image, each containing a single combined image sampler.
Descriptor sets are created and updated once during initialization.  No
descriptor set updates occur during frame rendering.

### Push constants

MVP transforms (64 bytes) are passed via push constants.  For portal and
mirror views, an additional 64 bytes are used for the eye transform and
clipping plane (128 bytes total, the Vulkan guaranteed minimum).

### Pipelines

Standard pipelines (skybox, fog, dynamic light, shadow volumes, debug
overlays) are created at renderer startup.  Map-specific pipelines are created
during Q3 shader parsing at map load time.  For each Q3 shader, three
pipelines are created: regular view, portal view, and mirror view.

### Shaders

SPIR-V vertex and fragment shaders emulate the corresponding fixed-function
OpenGL operations.  Specialization constants are used for alpha test function
selection, texture blend operations, and optional clipping plane support,
avoiding the need for large numbers of shader permutations.


-------------------------------------------------------------------------------
  THANKS
-------------------------------------------------------------------------------

  - Id Software, for creating Quake 3 and releasing its source code under the
    GPL.

  - Sui Jingfeng, for the vkQuake3 Vulkan renderer implementation.

  - Kenny Cason (kennyalive), for the original Quake-III-Arena-Kenny-Edition
    Vulkan backend that vkQuake3 is based on.

  - The ioquake3 team and contributors, for maintaining and improving the
    engine.
