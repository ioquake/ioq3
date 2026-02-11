# vulkan backend info
* codes in this dir is "borrow" from https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition, I convert cpp to c so that it can compile.

* vulkan forder is copied form vulkan sdk, not all items in it is used, it need clean.

* I am a naive programmer, need help, doc and instructions.


# Rendering

## General setup. 

Single command buffer that records all the commands. Single render pass which specifies color and depth-stencil attachment. 
Stencil buffer is used to render Q3's stencil shadows (cg\_shadows=2).

## Geometry. 
Quake 3 renderer prepares geometry data for each draw call in tess.xyz and tess.indexes arrays.
OpenGL backend calls qglDrawElements to feed this geometry to the GPU. 
Vulkan backend appends this data to geometry buffers that are bound to host visible memory chunk.
At the end of the frame when command buffer is submitted to the queue the geometry buffers contain all the geometry data to render the frame.
Typically up to 500Kb of vertex data is copied to the vertex buffer and up to 100Kb of index data is copied to the index buffer per frame.

## Descriptor sets.
For each image used by the renderer separate descriptor set is created.
Each descriptor set contains single descriptor (combined image sampler).
For each draw call either one or two (if lightmap is available) descriptor sets are bound.
Descriptor sets are updated only once during initialization.
There are no descriptor set updates during frame.

## Per-primitive uniform data.
Vulkan guarantees that minimum size of push constants range is at least 128 bytes.
To render ordinary view we use 64 bytes to specify mvp transform.
For portaled/mirrored views additional 64 byte are used to specify eye transform and clipping plane.

Pipeline layout. 2 sets + 128 bytes push constant range.

## Pipelines. 
Standard pipelines are created when renderer starts. 
They are used for skybox rendering, fog/dynamic light effects, shadow volumes and various debug features.
Map specific pipelines are created as part of parsing Q3 shaders and are created during map load.
For each Q3 shader we create three pipelines: one pipeline to render regular view and two additional pipelines for portal and mirror views.

## Shaders.
Emulate corresponding fixed-function functionality. 
Vertex shaders are boring with the only thing to mention that for portaled/mirrored views
we additionally compute distance to the clipping plane. 
Fragment shaders do one or two texture lookups and modulate the results by the color.

## Draw calls. 
vkCmdDrawIndexed is used to draw geometry in most cases. Additionally there are few debug features that use vkCmdDraw to convey unindexed vertexes.

## Code
vk.h provides interface that brings Vulkan support to Q3 renderer. 
The interface is quite concise and consists of a dozen of functions that can be divided into 3 categories: 
initialization functions, resource management functions and rendering setup functions.

### Initialization:

* vk\_initialize : initialize Vulkan backend
* vk\_shutdown : shutdown Vulkan backend

### Resource management:

* images: vk\_create\_image/vk\_upload\_image\_data

* descriptor sets: vk\_update\_descriptor\_set

* samplers: vk\_find\_sampler

* pipelines: vk\_find\_pipeline

### Rendering setup:

* vk\_clear\_attachments : clears framebuffer¡¯s attachments.

* vk\_bind\_geometry : is called when we start drawing new geometry.

* vk\_shade\_geometry : is called to shade geometry specified with vk\_bind\_geometry. Can be called multiple times for Q3's multi-stage shaders.

* vk\_begin\_frame/vk\_end\_frame : frame setup.

* vk\_read\_pixels : takes a screenshot.


### about turn the intensity/gamma of the drawing wondow

* use r\_gamma in the pulldown window, which nonlinearly correct the image before the uploading to GPU.
`\r_gamma 1.5` then `vid_restart`

* you can also use r\_intensity which turn the intensity linearly.
```
# 1.5 ~ 2.0 give acceptable result
$ \r_intensity 1.8
$ \vid_restart
```

* but why, because original gamma setting program turn the light by setting entire destop window.
which works on newer computer on the market 
but not works on some machine. it is buggy and embarrasing when program abnormal quit or stall.

### new cmd

* pipelineList: list the pipeline we have created;
* gpuMem: image memmory allocated on GPU;
* printOR: print the value of backend.or;
* displayResoList: list of the display resolution you monitor supported
For example:
```
$ \displayResoList 

Mode  0: 320x240
Mode  1: 400x300
Mode  2: 512x384
Mode  3: 640x480 (480p)
Mode  4: 800x600
Mode  5: 960x720
Mode  6: 1024x768
Mode  7: 1152x864
Mode  8: 1280x1024
Mode  9: 1600x1200
Mode 10: 2048x1536
Mode 11: 856x480
Mode 12: 1280x720 (720p)
Mode 13: 1280x768
Mode 14: 1280x800
Mode 15: 1280x960
Mode 16: 1360x768
Mode 17: 1366x768
Mode 18: 1360x1024
Mode 19: 1400x1050
Mode 20: 1400x900
Mode 21: 1600x900
Mode 22: 1680x1050
Mode 23: 1920x1080 (1080p)
Mode 24: 1920x1200
Mode 25: 1920x1440
Mode 26: 2560x1080
Mode 27: 2560x1600
Mode 28: 3840x2160 (4K)

$ \r_mode 12
$ \vid_restart
```



### TODO:
* get cpu, gpu, memory usage
