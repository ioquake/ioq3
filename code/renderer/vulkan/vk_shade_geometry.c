#include "vk_shade_geometry.h"
#include "vk_instance.h"
#include "tr_globals.h"
#include "tr_cvar.h"
#include "vk_image.h"
#include "vk_pipelines.h"
#include "matrix_multiplication.h"
#include "tr_backend.h"
#include "glConfig.h"
#include "R_PortalPlane.h"
#include "tr_light.h"
#include "tr_shader.h"

#define VERTEX_CHUNK_SIZE   (768 * 1024)
#define INDEX_BUFFER_SIZE   (2 * 1024 * 1024)

#define XYZ_SIZE            (4 * VERTEX_CHUNK_SIZE)
#define COLOR_SIZE          (1 * VERTEX_CHUNK_SIZE)
#define ST0_SIZE            (2 * VERTEX_CHUNK_SIZE)
#define ST1_SIZE            (2 * VERTEX_CHUNK_SIZE)

#define XYZ_OFFSET          0
#define COLOR_OFFSET        (XYZ_OFFSET + XYZ_SIZE)
#define ST0_OFFSET          (COLOR_OFFSET + COLOR_SIZE)
#define ST1_OFFSET          (ST0_OFFSET + ST0_SIZE)

struct ShadingData_t
{
    // Buffers represent linear arrays of data which are used for various purposes
    // by binding them to a graphics or compute pipeline via descriptor sets or 
    // via certain commands,  or by directly specifying them as parameters to 
    // certain commands. Buffers are represented by VkBuffer handles:
	VkBuffer vertex_buffer;
	unsigned char* vertex_buffer_ptr ; // pointer to mapped vertex buffer
	uint32_t xyz_elements;
	uint32_t color_st_elements;

	VkBuffer index_buffer;
	unsigned char* index_buffer_ptr; // pointer to mapped index buffer
	uint32_t index_buffer_offset;

	// host visible memory that holds both vertex and index data
	VkDeviceMemory vertex_buffer_memory;
	VkDeviceMemory index_buffer_memory;
    VkDescriptorSet curDescriptorSets[2];

    // This flag is used to decide whether framebuffer's depth attachment should be cleared
    // with vmCmdClearAttachment (dirty_depth_attachment == true), or it have just been
    // cleared by render pass instance clear op (dirty_depth_attachment == false).

    VkBool32 s_depth_attachment_dirty;
};

struct ShadingData_t shadingDat;


VkBuffer vk_getIndexBuffer(void)
{
    return shadingDat.index_buffer;
}


static float s_modelview_matrix[16] QALIGN(16);



void set_modelview_matrix(const float mv[16])
{
    memcpy(s_modelview_matrix, mv, 64);
}


const float * getptr_modelview_matrix()
{
    return s_modelview_matrix;
}


// TODO   : figure out the principle
static void vk_setViewportScissor(VkBool32 is2D, enum Vk_Depth_Range dR,
        VkViewport* const vp, VkRect2D* const pRect)
{
    int width, height;
    R_GetWinResolution(&width, &height);

	if (is2D)
	{

        pRect->offset.x = vp->x = 0;
        pRect->offset.y = vp->y = 0;
        
        pRect->extent.width = vp->width = width;
		pRect->extent.height = vp->height = height;
	}
	else
	{
		int X = backEnd.viewParms.viewportX;
		int Y = backEnd.viewParms.viewportY;
		int W = backEnd.viewParms.viewportWidth;
		int H = backEnd.viewParms.viewportHeight;

        //pRect->offset.x = backEnd.viewParms.viewportX;
        //pRect->offset.y = backEnd.viewParms.viewportY;
        //pRect->extent.width = backEnd.viewParms.viewportWidth;
		//pRect->extent.height = backEnd.viewParms.viewportHeight;

        if ( X < 0)
		    X = 0;
        if (Y < 0)
		    Y = 0;
        if (X + W > width)
		    W = width - X;
	    if (Y + H > height)
		    H = height - Y;

        pRect->offset.x = vp->x = X;
		pRect->offset.y = vp->y = Y;
		pRect->extent.width = vp->width = W;
		pRect->extent.height = vp->height = H;
	}

    switch(dR)
    {
        case DEPTH_RANGE_NORMAL:
        {
        	vp->minDepth = 0.0f;
		    vp->maxDepth = 1.0f;
        }break;

        case DEPTH_RANGE_ZERO:
        {
		    vp->minDepth = 0.0f;
		    vp->maxDepth = 0.0f;
	    }break;
        
        case DEPTH_RANGE_ONE:
        {
		    vp->minDepth = 1.0f;
		    vp->maxDepth = 1.0f;
	    }break;

        case DEPTH_RANGE_WEAPON:
        {
            vp->minDepth = 0.0f;
		    vp->maxDepth = 0.3f;
        }break;
    }
}


VkRect2D get_scissor_rect(void)
{

	VkRect2D r;
	
    int width, height;
    R_GetWinResolution(&width, &height);
    
    if (backEnd.projection2D)
	{
		r.offset.x = 0.0f;
		r.offset.y = 0.0f;
		r.extent.width = width;
		r.extent.height = height;
	}
	else
	{
		assert(backEnd.viewParms.viewportWidth > 0);
		assert(backEnd.viewParms.viewportHeight > 0);
		r.offset.x = backEnd.viewParms.viewportX;
        r.offset.y = backEnd.viewParms.viewportY;
        r.extent.width = backEnd.viewParms.viewportWidth;
		r.extent.height = backEnd.viewParms.viewportHeight;

        // for draw model in setu manus       
        if (r.offset.x < 0)
		    r.offset.x = 0;
        if (r.offset.y < 0)
		    r.offset.y = 0;
        if (r.offset.x + r.extent.width > width)
		    r.extent.width = width - r.offset.x;
	    if (r.offset.y + r.extent.height > height)
		    r.extent.height = height - r.offset.y;

        // ri.Printf(PRINT_ALL, "(%d, %d, %d, %d)\n",
        // r.offset.x, r.offset.y, r.extent.width, r.extent.height);
    }

	return r;
}


// Vulkan memory is broken up into two categories, host memory and device memory.
// Host memory is memory needed by the Vulkan implementation for 
// non-device-visible storage. Allocations returned by vkAllocateMemory
// are guaranteed to meet any alignment requirement of the implementation
//
// Host access to buffer must be externally synchronized

void vk_createVertexBuffer(void)
{
    ri.Printf(PRINT_ALL, " Create vertex buffer: shadingDat.vertex_buffer \n");

    VkBufferCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    //VERTEX_BUFFER_SIZE
    desc.size = XYZ_SIZE + COLOR_SIZE + ST0_SIZE + ST1_SIZE;
    desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    VK_CHECK(qvkCreateBuffer(vk.device, &desc, NULL, &shadingDat.vertex_buffer));


    VkMemoryRequirements vb_memory_requirements;
    qvkGetBufferMemoryRequirements(vk.device, shadingDat.vertex_buffer, &vb_memory_requirements);
    
    uint32_t memory_type_bits = vb_memory_requirements.memoryTypeBits;

    VkMemoryAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = vb_memory_requirements.size;
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit specifies that memory allocated with
    // this type can be mapped for host access using vkMapMemory.
    //
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache
    // management commands vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges
    // are not needed to flush host writes to the device or make device writes visible
    // to the host, respectively.
    alloc_info.memoryTypeIndex = find_memory_type(memory_type_bits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ri.Printf(PRINT_ALL, " Allocate device memory for Vertex Buffer: %ld bytes. \n",
            alloc_info.allocationSize);

    VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &shadingDat.vertex_buffer_memory));

    qvkBindBufferMemory(vk.device, shadingDat.vertex_buffer, shadingDat.vertex_buffer_memory, 0);

    void* data;
    VK_CHECK(qvkMapMemory(vk.device, shadingDat.vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data));
    shadingDat.vertex_buffer_ptr = (unsigned char*)data;

}



void vk_createIndexBuffer(void)
{
    ri.Printf(PRINT_ALL, " Create index buffer: shadingDat.index_buffer \n");

    VkBufferCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    desc.size = INDEX_BUFFER_SIZE;
    desc.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VK_CHECK(qvkCreateBuffer(vk.device, &desc, NULL, &shadingDat.index_buffer));

    
    VkMemoryRequirements ib_memory_requirements;
    qvkGetBufferMemoryRequirements(vk.device, shadingDat.index_buffer, &ib_memory_requirements);

    uint32_t memory_type_bits = ib_memory_requirements.memoryTypeBits;


    VkMemoryAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = ib_memory_requirements.size;
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit specifies that memory allocated with
    // this type can be mapped for host access using vkMapMemory.
    //
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache
    // management commands vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges
    // are not needed to flush host writes to the device or make device writes visible
    // to the host, respectively.
    alloc_info.memoryTypeIndex = find_memory_type(memory_type_bits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ri.Printf(PRINT_ALL, " Allocate device memory for Index Buffer: %ld bytes. \n",
            alloc_info.allocationSize);

    VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &shadingDat.index_buffer_memory));
    qvkBindBufferMemory(vk.device, shadingDat.index_buffer, shadingDat.index_buffer_memory, 0);

    void* data;
    VK_CHECK(qvkMapMemory(vk.device, shadingDat.index_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data));
    shadingDat.index_buffer_ptr = (unsigned char*)data;
}


// Descriptors and Descriptor Sets
// A descriptor is a special opaque shader variable that shaders use to access buffer 
// and image resources in an indirect fashion. It can be thought of as a "pointer" to
// a resource.  The Vulkan API allows these variables to be changed between draw
// operations so that the shaders can access different resources for each draw.

// A descriptor set is called a "set" because it can refer to an array of homogenous
// resources that can be described with the same layout binding. one possible way to
// use multiple descriptors is to construct a descriptor set with two descriptors, 
// with each descriptor referencing a separate texture. Both textures are therefore
// available during a draw. A command in a command buffer could then select the texture
// to use by specifying the index of the desired texture. To describe a descriptor set,
// you use a descriptor set layout.

// Descriptor sets corresponding to bound texture images.

// outside of TR since it shouldn't be cleared during ref re-init
// the renderer front end should never modify glstate_t
//typedef struct {



void updateCurDescriptor( VkDescriptorSet curDesSet, uint32_t tmu)
{
    shadingDat.curDescriptorSets[tmu] = curDesSet;
}



void vk_shade_geometry(VkPipeline pipeline, VkBool32 multitexture, enum Vk_Depth_Range depRg, VkBool32 indexed)
{
	// configure vertex data stream
	VkBuffer bufs[3] = { shadingDat.vertex_buffer, shadingDat.vertex_buffer, shadingDat.vertex_buffer };
	VkDeviceSize offs[3] = {
		COLOR_OFFSET + shadingDat.color_st_elements * sizeof(color4ub_t),
		ST0_OFFSET   + shadingDat.color_st_elements * sizeof(vec2_t),
		ST1_OFFSET   + shadingDat.color_st_elements * sizeof(vec2_t)
	};

    // color
    if ((shadingDat.color_st_elements + tess.numVertexes) * sizeof(color4ub_t) > COLOR_SIZE)
        ri.Error(ERR_DROP, "vulkan: vertex buffer overflow (color) %ld \n", 
                (shadingDat.color_st_elements + tess.numVertexes) * sizeof(color4ub_t));

    unsigned char* dst_color = shadingDat.vertex_buffer_ptr + offs[0];
    memcpy(dst_color, tess.svars.colors, tess.numVertexes * sizeof(color4ub_t));
    // st0

    unsigned char* dst_st0 = shadingDat.vertex_buffer_ptr + offs[1];
    memcpy(dst_st0, tess.svars.texcoords[0], tess.numVertexes * sizeof(vec2_t));

	// st1
	if (multitexture)
    {
		unsigned char* dst = shadingDat.vertex_buffer_ptr + offs[2];
		memcpy(dst, tess.svars.texcoords[1], tess.numVertexes * sizeof(vec2_t));
	}

	qvkCmdBindVertexBuffers(vk.command_buffer, 1, multitexture ? 3 : 2, bufs, offs);
	shadingDat.color_st_elements += tess.numVertexes;

	// bind descriptor sets

//    vkCmdBindDescriptorSets causes the sets numbered [firstSet.. firstSet+descriptorSetCount-1] to use
//    the bindings stored in pDescriptorSets[0..descriptorSetCount-1] for subsequent rendering commands 
//    (either compute or graphics, according to the pipelineBindPoint).
//    Any bindings that were previously applied via these sets are no longer valid.

	qvkCmdBindDescriptorSets(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        vk.pipeline_layout, 0, (multitexture ? 2 : 1), shadingDat.curDescriptorSets, 0, NULL);

    // bind pipeline
	qvkCmdBindPipeline(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// configure pipeline's dynamic state

    VkViewport viewport;
    VkRect2D scissor; // = get_scissor_rect();

    vk_setViewportScissor(backEnd.projection2D, depRg, &viewport, &scissor);

    qvkCmdSetScissor(vk.command_buffer, 0, 1, &scissor);
    qvkCmdSetViewport(vk.command_buffer, 0, 1, &viewport);


	if (tess.shader->polygonOffset) {
		qvkCmdSetDepthBias(vk.command_buffer, r_offsetUnits->value, 0.0f, r_offsetFactor->value);
	}

	// issue draw call
	if (indexed)
		qvkCmdDrawIndexed(vk.command_buffer, tess.numIndexes, 1, 0, 0, 0);
	else
		qvkCmdDraw(vk.command_buffer, tess.numVertexes, 1, 0, 0);
	
    shadingDat.s_depth_attachment_dirty = VK_TRUE;
}



void updateMVP(VkBool32 isPortal, VkBool32 is2D, const float mvMat4x4[16])
{
	if (isPortal)
    {
        // mvp transform + eye transform + clipping plane in eye space
        float push_constants[32] QALIGN(16);
    
        // Eye space transform.
        MatrixMultiply4x4_SSE(mvMat4x4, backEnd.viewParms.projectionMatrix, push_constants);

        // NOTE: backEnd.or.modelMatrix incorporates s_flipMatrix,
        // so it should be taken into account when computing clipping plane too.

		push_constants[16] = backEnd.or.modelMatrix[0];
		push_constants[17] = backEnd.or.modelMatrix[4];
		push_constants[18] = backEnd.or.modelMatrix[8];
		push_constants[19] = backEnd.or.modelMatrix[12];

		push_constants[20] = backEnd.or.modelMatrix[1];
		push_constants[21] = backEnd.or.modelMatrix[5];
		push_constants[22] = backEnd.or.modelMatrix[9];
		push_constants[23] = backEnd.or.modelMatrix[13];

		push_constants[24] = backEnd.or.modelMatrix[2];
		push_constants[25] = backEnd.or.modelMatrix[6];
		push_constants[26] = backEnd.or.modelMatrix[10];
		push_constants[27] = backEnd.or.modelMatrix[14];
	
        // Clipping plane in eye coordinates.
		struct rplane_s eye_plane;

        R_TransformPlane(backEnd.viewParms.or.axis, backEnd.viewParms.or.origin, &eye_plane);
        
        // Apply s_flipMatrix to be in the same coordinate system as push_constants.
        
        push_constants[28] = -eye_plane.normal[1];
		push_constants[29] =  eye_plane.normal[2];
		push_constants[30] = -eye_plane.normal[0];
		push_constants[31] =  eye_plane.dist;


        // As described above in section Pipeline Layouts, the pipeline layout defines shader push constants
        // which are updated via Vulkan commands rather than via writes to memory or copy commands.
        // Push constants represent a high speed path to modify constant data in pipelines
        // that is expected to outperform memory-backed resource updates.
	    qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 128, push_constants);
	}
    else
    {
      	// push constants are another way of passing dynamic values to shaders
		// Specify push constants.
		float mvp[16] QALIGN(16); // mvp transform + eye transform + clipping plane in eye space
        
        if (is2D)
        {            
            float width, height;
            R_GetWinResolutionF(&width, &height);

            mvp[0] = 2.0f / width; 
            mvp[1] = 0.0f; 
            mvp[2] = 0.0f;
            mvp[3] = 0.0f;

            mvp[4] = 0.0f; 
            mvp[5] = 2.0f / height; 
            mvp[6] = 0.0f;
            mvp[7] = 0.0f;

            mvp[8] = 0.0f; 
            mvp[9] = 0.0f; 
            mvp[10] = 1.0f; 
            mvp[11] = 0.0f;
            
            mvp[12] = -1.0f; 
            mvp[13] = -1.0f; 
            mvp[14] = 0.0f;
            mvp[15] = 1.0f;
        }
        else
        {
            // update q3's proj matrix (opengl) to vulkan conventions:
            // z - [0, 1] instead of [-1, 1] and invert y direction
            MatrixMultiply4x4_SSE(mvMat4x4, backEnd.viewParms.projectionMatrix, mvp);
        }

        // As described above in section Pipeline Layouts, the pipeline layout defines shader push constants
        // which are updated via Vulkan commands rather than via writes to memory or copy commands.
        // Push constants represent a high speed path to modify constant data in pipelines
        // that is expected to outperform memory-backed resource updates.
		qvkCmdPushConstants(vk.command_buffer, vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, mvp);
    }
}


// =========================================================
// Vertex fetching is controlled via configurable state, 
// as a logically distinct graphics pipeline stage.
//  
//  Vertex Attributes
//
//  Vertex shaders can define input variables, which receive vertex attribute data
//  transferred from one or more VkBuffer(s) by drawing commands. Vertex shader 
//  input variables are bound to buffers via an indirect binding where the vertex 
//  shader associates a vertex input attribute number with each variable, vertex 
//  input attributes are associated to vertex input bindings on a per-pipeline basis, 
//  and vertex input bindings are associated with specific buffers on a per-draw basis
//  via the vkCmdBindVertexBuffers command. 
//
//  Vertex input attribute and vertex input binding descriptions also
//  contain format information controlling how data is extracted from
//  buffer memory and converted to the format expected by the vertex shader.
//
//  There are VkPhysicalDeviceLimits::maxVertexInputAttributes number of vertex
//  input attributes and VkPhysicalDeviceLimits::maxVertexInputBindings number of
//  vertex input bindings (each referred to by zero-based indices), where there 
//  are at least as many vertex input attributes as there are vertex input bindings.
//  Applications can store multiple vertex input attributes interleaved in a single 
//  buffer, and use a single vertex input binding to access those attributes.
//
//  In GLSL, vertex shaders associate input variables with a vertex input attribute
//  number using the location layout qualifier. The component layout qualifier
//  associates components of a vertex shader input variable with components of
//  a vertex input attribute.

void vk_UploadXYZI(float (*pXYZ)[4], uint32_t nVertex, uint32_t* pIdx, uint32_t nIndex)
{
	// xyz stream
	{
        const VkDeviceSize xyz_offset = XYZ_OFFSET + shadingDat.xyz_elements * sizeof(vec4_t);
		
        unsigned char* vDst = shadingDat.vertex_buffer_ptr + xyz_offset;

        // 4 float in the array, with each 4 bytes.
		memcpy(vDst, pXYZ, nVertex * 16);

		qvkCmdBindVertexBuffers(vk.command_buffer, 0, 1, &shadingDat.vertex_buffer, &xyz_offset);
		
        shadingDat.xyz_elements += tess.numVertexes;

        assert (shadingDat.xyz_elements * sizeof(vec4_t) < XYZ_SIZE);
	}

	// indexes stream
    if(nIndex != 0)
	{
		const uint32_t indexes_size = nIndex * sizeof(uint32_t);        

		unsigned char* iDst = shadingDat.index_buffer_ptr + shadingDat.index_buffer_offset;
		memcpy(iDst, pIdx, indexes_size);

		qvkCmdBindIndexBuffer(vk.command_buffer, shadingDat.index_buffer, shadingDat.index_buffer_offset, VK_INDEX_TYPE_UINT32);
		
        shadingDat.index_buffer_offset += indexes_size;

        assert (shadingDat.index_buffer_offset < INDEX_BUFFER_SIZE);
	}
}


void vk_resetGeometryBuffer(void)
{
	// Reset geometry buffer's current offsets.
	shadingDat.xyz_elements = 0;
	shadingDat.color_st_elements = 0;
	shadingDat.index_buffer_offset = 0;
    shadingDat.s_depth_attachment_dirty = VK_FALSE;

    Mat4Identity(s_modelview_matrix);
}


void vk_destroy_shading_data(void)
{
    ri.Printf(PRINT_ALL, " Destroy vertex/index buffer: shadingDat.vertex_buffer shadingDat.index_buffer. \n");
    ri.Printf(PRINT_ALL, " Free device memory: vertex_buffer_memory index_buffer_memory. \n");

    qvkUnmapMemory(vk.device, shadingDat.vertex_buffer_memory);
	qvkFreeMemory(vk.device, shadingDat.vertex_buffer_memory, NULL);

    qvkUnmapMemory(vk.device, shadingDat.index_buffer_memory);
	qvkFreeMemory(vk.device, shadingDat.index_buffer_memory, NULL);

    qvkDestroyBuffer(vk.device, shadingDat.vertex_buffer, NULL);
	qvkDestroyBuffer(vk.device, shadingDat.index_buffer, NULL);

    memset(&shadingDat, 0, sizeof(shadingDat));


    VK_CHECK(qvkResetDescriptorPool(vk.device, vk.descriptor_pool, 0));
}



void vk_clearDepthStencilAttachments(void)
{
    if(shadingDat.s_depth_attachment_dirty)
    {
        VkClearAttachment attachments;
        memset(&attachments, 0, sizeof(VkClearAttachment));

        attachments.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        attachments.clearValue.depthStencil.depth = 1.0f;

        if (r_shadows->integer == 2) {
            attachments.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            attachments.clearValue.depthStencil.stencil = 0;
        }

        VkClearRect clear_rect;
        clear_rect.rect = get_scissor_rect();
        clear_rect.baseArrayLayer = 0;
        clear_rect.layerCount = 1;


        qvkCmdClearAttachments(vk.command_buffer, 1, &attachments, 1, &clear_rect);
    }
}




void vk_clearColorAttachments(const float* color)
{

    // ri.Printf(PRINT_ALL, "vk_clearColorAttachments\n");

    VkClearAttachment attachments[1];
    memset(attachments, 0, sizeof(VkClearAttachment));
    
    // aspectMask is a mask selecting the color, depth and/or stencil aspects
    // of the attachment to be cleared. aspectMask can include 
    // VK_IMAGE_ASPECT_COLOR_BIT for color attachments,
    // VK_IMAGE_ASPECT_DEPTH_BIT for depth/stencil attachments with a depth
    // component, and VK_IMAGE_ASPECT_STENCIL_BIT for depth/stencil attachments
    // with a stencil component. If the subpass¡¯s depth/stencil attachment
    // is VK_ATTACHMENT_UNUSED, then the clear has no effect.

    attachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT
    // is set in aspectMask, in which case it is an index to 
    // the pColorAttachments array in the VkSubpassDescription structure
    // of the current subpass which selects the color attachment to clear.
    attachments[0].colorAttachment = 0;
    attachments[0].clearValue.color.float32[0] = color[0];
    attachments[0].clearValue.color.float32[1] = color[1];
    attachments[0].clearValue.color.float32[2] = color[2];
    attachments[0].clearValue.color.float32[3] = color[3];

/* 
	VkClearRect clear_rect[2];
	clear_rect[0].rect = get_scissor_rect();
	clear_rect[0].baseArrayLayer = 0;
	clear_rect[0].layerCount = 1;
	uint32_t rect_count = 1;
  
	// Split viewport rectangle into two non-overlapping rectangles.
	// It's a HACK to prevent Vulkan validation layer's performance warning:
	//		"vkCmdClearAttachments() issued on command buffer object XXX prior to any Draw Cmds.
	//		 It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw."
	// 
	// NOTE: we don't use LOAD_OP_CLEAR for color attachment when we begin renderpass
	// since at that point we don't know whether we need color buffer clear (usually we don't).
    uint32_t h = clear_rect[0].rect.extent.height / 2;
    clear_rect[0].rect.extent.height = h;
    clear_rect[1] = clear_rect[0];
    clear_rect[1].rect.offset.y = h;
    rect_count = 2;
*/

    VkClearRect clear_rect[1];
	clear_rect[0].rect = get_scissor_rect();
	clear_rect[0].baseArrayLayer = 0;
	clear_rect[0].layerCount = 1;

	qvkCmdClearAttachments(vk.command_buffer, 1, attachments, 1, clear_rect);

}




static void ComputeColors( shaderStage_t *pStage )
{
	int		i, nVerts;
	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
			break;
		case CGEN_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor( tess.svars.colors );
			break;
		case CGEN_EXACT_VERTEX:
			memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			break;
		case CGEN_CONST:
            
            nVerts = tess.numVertexes;

			for ( i = 0; i < nVerts; i++ )
            {
				memcpy(tess.svars.colors[i], pStage->constantColor, 4);
			}
			break;
		case CGEN_VERTEX:
			if ( tr.identityLight == 1 )
			{
				memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
					tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
					tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			if ( tr.identityLight == 1 )
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
					tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
					tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
				}
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
					tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
					tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
				}
			}
			break;
		case CGEN_FOG:
		{
			fog_t* fog = tr.world->fogs + tess.fogNum;

            nVerts = tess.numVertexes;

			for (i = 0; i < nVerts; i++)
			{
				tess.svars.colors[i][0] = fog->colorRGBA[0];
				tess.svars.colors[i][1] = fog->colorRGBA[1];
				tess.svars.colors[i][2] = fog->colorRGBA[2];
				tess.svars.colors[i][3] = fog->colorRGBA[3];
			}
		}break;
		case CGEN_WAVEFORM:
			RB_CalcWaveColor( &pStage->rgbWave, tess.svars.colors );
			break;
		case CGEN_ENTITY:
			RB_CalcColorFromEntity( tess.svars.colors );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity( tess.svars.colors );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( i = 0; i < tess.numVertexes; i++ ) {
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
		break;
    case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; i++ )
        {
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        }
        break;
	case AGEN_PORTAL:
		{
			unsigned char alpha;

			for ( i = 0; i < tess.numVertexes; i++ )
			{
				vec3_t v;

				VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
				float len = VectorLength( v );

				len /= tess.shader->portalRange;

				if ( len < 0 )
				{
					alpha = 0;
				}
				else if ( len > 1 )
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				tess.svars.colors[i][3] = alpha;
			}
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum )
	{
		switch ( pStage->adjustColorsForFog )
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_NONE:
			break;
		}
	}
}

static void ComputeTexCoords( shaderStage_t *pStage )
{
	uint32_t i;
	uint32_t b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ )
    {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen )
		{
            case TCGEN_IDENTITY:
                memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
                break;
            case TCGEN_TEXTURE:
                for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
                }
                break;
            case TCGEN_LIGHTMAP:
                for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
                }
                break;
            case TCGEN_VECTOR:
                for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                    tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
                    tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
                }
                break;
            case TCGEN_FOG:
                RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_ENVIRONMENT_MAPPED:
                RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_BAD:
                return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ )
        {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale, ( float * ) tess.svars.texcoords[b] );
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm], ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed, ( float * ) tess.svars.texcoords[b] );
				break;

			default:
				ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name );
				break;
			}
		}
	}
}



/*
===================
ProjectDlightTexture
Perform dynamic lighting with another rendering pass
===================
*/
static void ProjectDlightTexture( void )
{
	byte	clipBits[SHADER_MAX_VERTEXES];

	if ( !backEnd.refdef.num_dlights ) {
		return;
	}

    uint32_t l;
	for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ )
    {
		//dlight_t	*dl;

		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}
		float* texCoords = tess.svars.texcoords[0][0];
		//colors = tess.svars.colors[0];

		//dl = &backEnd.refdef.dlights[l];
        vec3_t	origin;

		VectorCopy( backEnd.refdef.dlights[l].transformed, origin );


        float radius = backEnd.refdef.dlights[l].radius;
		float scale = 1.0f / radius;
    	float modulate;

        float floatColor[3] = {
		    backEnd.refdef.dlights[l].color[0] * 255.0f,
		    backEnd.refdef.dlights[l].color[1] * 255.0f,
		    backEnd.refdef.dlights[l].color[2] * 255.0f
        };

        uint32_t i;
		for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2)
        {
			vec3_t	dist;

			backEnd.pc.c_dlightVertexes++;

			VectorSubtract( origin, tess.xyz[i], dist );
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			uint32_t clip = 0;
			if ( texCoords[0] < 0.0f ) {
				clip |= 1;
			}
            else if ( texCoords[0] > 1.0f ) {
				clip |= 2;
			}

			if ( texCoords[1] < 0.0f ) {
				clip |= 4;
			}
            else if ( texCoords[1] > 1.0f ) {
				clip |= 8;
			}

			// modulate the strength based on the height and color
			if ( dist[2] > radius )
            {
				clip |= 16;
				modulate = 0.0f;
			}
            else if ( dist[2] < -radius )
            {
				clip |= 32;
				modulate = 0.0f;
			}
            else
            {
				dist[2] = fabs(dist[2]);
				if ( dist[2] < radius * 0.5f )
                {
					modulate = 1.0f;
				}
                else
                {
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;
            
            // += 4 
			tess.svars.colors[i][0] = (floatColor[0] * modulate);
			tess.svars.colors[i][1] = (floatColor[1] * modulate);
			tess.svars.colors[i][2] = (floatColor[2] * modulate);
			tess.svars.colors[i][3] = 255;
		}

      
		// build a list of triangles that need light
		uint32_t numIndexes = 0;
		for ( i = 0 ; i < tess.numIndexes ; i += 3 )
        {
			uint32_t a, b, c;

			a = tess.indexes[i];
			b = tess.indexes[i+1];
			c = tess.indexes[i+2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue;	// not lighted
			}
			numIndexes += 3;
		}

		if ( numIndexes == 0 ) {
			continue;
		}


		updateCurDescriptor( tr.dlightImage->descriptor_set, 0 );
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light where they aren't rendered
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;

		// VULKAN

		vk_shade_geometry(g_stdPipelines.dlight_pipelines[backEnd.refdef.dlights[l].additive > 0 ? 1 : 0][tess.shader->cullType][tess.shader->polygonOffset],
                VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);

	}
}



/*
===================
RB_FogPass
Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {

	unsigned int i;

	fog_t* fog = tr.world->fogs + tess.fogNum;

    const unsigned int nVerts = tess.numVertexes;
	for (i = 0; i < nVerts; i++)
	{
		tess.svars.colors[i][0] = fog->colorRGBA[0];
		tess.svars.colors[i][1] = fog->colorRGBA[1];
		tess.svars.colors[i][2] = fog->colorRGBA[2];
		tess.svars.colors[i][3] = fog->colorRGBA[3];
	}

	RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[0] );

	updateCurDescriptor( tr.fogImage->descriptor_set, 0);

	// VULKAN

    assert(tess.shader->fogPass > 0);
    VkPipeline pipeline = g_stdPipelines.fog_pipelines[tess.shader->fogPass - 1][tess.shader->cullType][tess.shader->polygonOffset];
    vk_shade_geometry(pipeline, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);
}


void RB_StageIteratorGeneric( void )
{
//	shaderCommands_t *input = &tess;

	RB_DeformTessGeometry();

	// call shader function
	//
	// VULKAN
   
    vk_UploadXYZI(tess.xyz, tess.numVertexes, tess.indexes, tess.numIndexes);

    updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, 
            getptr_modelview_matrix() );
    

    uint32_t stage = 0;

	for ( stage = 0; stage < MAX_SHADER_STAGES; ++stage )
	{
		if ( NULL == tess.xstages[stage])
		{
			break;
		}

		ComputeColors( tess.xstages[stage] );
		ComputeTexCoords( tess.xstages[stage] );

        // base
        // set state
		//R_BindAnimatedImage( &tess.xstages[stage]->bundle[0] );
        VkBool32 multitexture = (tess.xstages[stage]->bundle[1].image[0] != NULL);

    {        
	    if ( tess.xstages[stage]->bundle[0].isVideoMap )
        {
		    ri.CIN_RunCinematic(tess.xstages[stage]->bundle[0].videoMapHandle);
		    ri.CIN_UploadCinematic(tess.xstages[stage]->bundle[0].videoMapHandle);
		    goto ENDANIMA;
	    }

        int numAnimaImg = tess.xstages[stage]->bundle[0].numImageAnimations;

        if ( numAnimaImg <= 1 )
        {
		    updateCurDescriptor( tess.xstages[stage]->bundle[0].image[0]->descriptor_set, 0);
            //GL_Bind(tess.xstages[stage]->bundle[0].image[0]);
            goto ENDANIMA;
	    }

        // it is necessary to do this messy calc to make sure animations line up
        // exactly with waveforms of the same frequency
	    int index = (int)( tess.shaderTime * tess.xstages[stage]->bundle[0].imageAnimationSpeed * FUNCTABLE_SIZE ) >> FUNCTABLE_SIZE2;
        
        if ( index < 0 ) {
		    index = 0;	// may happen with shader time offsets
	    }

	    index %= numAnimaImg;

	    updateCurDescriptor( tess.xstages[stage]->bundle[0].image[ index ]->descriptor_set, 0);
        //GL_Bind(tess.xstages[stage]->bundle[0].image[ index ]);
    }
    
ENDANIMA:
		//
		// do multitexture
		//

		if ( multitexture )
		{
            // DrawMultitextured( input, stage );
            // output = t0 * t1 or t0 + t1

            // t0 = most upstream according to spec
            // t1 = most downstream according to spec
            // this is an ugly hack to work around a GeForce driver
            // bug with multitexture and clip planes


            if ( tess.xstages[stage]->bundle[1].isVideoMap )
            {
                ri.CIN_RunCinematic(tess.xstages[stage]->bundle[1].videoMapHandle);
                ri.CIN_UploadCinematic(tess.xstages[stage]->bundle[1].videoMapHandle);
                goto END_ANIMA2;
            }

            if ( tess.xstages[stage]->bundle[1].numImageAnimations <= 1 ) {
                updateCurDescriptor( tess.xstages[stage]->bundle[1].image[0]->descriptor_set, 1);
                goto END_ANIMA2;
            }

            // it is necessary to do this messy calc to make sure animations line up
            // exactly with waveforms of the same frequency
            int index2 = (int)( tess.shaderTime * tess.xstages[stage]->bundle[1].imageAnimationSpeed * FUNCTABLE_SIZE ) >> FUNCTABLE_SIZE2;

            if ( index2 < 0 ) {
                index2 = 0;	// may happen with shader time offsets
            }
	        
            index2 %= tess.xstages[stage]->bundle[1].numImageAnimations;

            updateCurDescriptor( tess.xstages[stage]->bundle[1].image[ index2 ]->descriptor_set , 1);

END_ANIMA2:

            if (r_lightmap->integer)
                updateCurDescriptor(tr.whiteImage->descriptor_set, 0); 
            
            // replace diffuse texture with a white one thus effectively render only lightmap
		}

       
        enum Vk_Depth_Range depth_range = DEPTH_RANGE_NORMAL;
        if (tess.shader->isSky)
        {
            depth_range = DEPTH_RANGE_ONE;
            if (r_showsky->integer)
                depth_range = DEPTH_RANGE_ZERO;
        }
        else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
        {
            depth_range = DEPTH_RANGE_WEAPON;
        }
 
        
        if (backEnd.viewParms.isMirror)
        {
            vk_shade_geometry(tess.xstages[stage]->vk_mirror_pipeline, multitexture, depth_range, VK_TRUE);
        }
        else if (backEnd.viewParms.isPortal)
        {
            vk_shade_geometry(tess.xstages[stage]->vk_portal_pipeline, multitexture, depth_range, VK_TRUE);
        }
        else
        {
            vk_shade_geometry(tess.xstages[stage]->vk_pipeline, multitexture, depth_range, VK_TRUE);
        }

                
		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( tess.xstages[stage]->bundle[0].isLightmap || tess.xstages[stage]->bundle[1].isLightmap ) )
		{
			break;
		}
	}

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
		&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}
}
