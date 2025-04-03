#include "tr_local.h"
#include "tr_cvar.h"
#include "vk_instance.h"
#include "vk_shade_geometry.h"
#include "vk_frame.h"
#include "vk_swapchain.h"

//  Synchronization of access to resources is primarily the responsibility
//  of the application in Vulkan. The order of execution of commands with
//  respect to the host and other commands on the device has few implicit
//  guarantees, and needs to be explicitly specified. Memory caches and 
//  other optimizations are also explicitly managed, requiring that the
//  flow of data through the system is largely under application control.
//  Whilst some implicit guarantees exist between commands, five explicit
//  synchronization mechanisms are exposed by Vulkan:
//
//
//                          Fences
//
//  Fences can be used to communicate to the host that execution of some 
//  task on the device has completed. 
//
//  Fences are a synchronization primitive that can be used to insert a dependency
//  from a queue to the host. Fences have two states - signaled and unsignaled.
//  A fence can be signaled as part of the execution of a queue submission command. 
//  Fences can be unsignaled on the host with vkResetFences. Fences can be waited
//  on by the host with the vkWaitForFences command, and the current state can be
//  queried with vkGetFenceStatus.
//
//
//                          Semaphores
//
//  Semaphores can be used to control resource access across multiple queues.
//
//                          Events
//
//  Events provide a fine-grained synchronization primitive which can be
//  signaled either within a command buffer or by the host, and can be 
//  waited upon within a command buffer or queried on the host.
//
//                          Pipeline Barriers
//
//  Pipeline barriers also provide synchronization control within a command buffer,
//  but at a single point, rather than with separate signal and wait operations.
//
//                          Render Passes
//
//  Render passes provide a useful synchronization framework for most rendering tasks,
//  built upon the concepts in this chapter. Many cases that would otherwise need an
//  application to use other synchronization primitives can be expressed more 
//  efficiently as part of a render pass.
//
//

VkSemaphore sema_imageAvailable;
VkSemaphore sema_renderFinished;
VkFence fence_renderFinished;

/*
   Use of a presentable image must occur only after the image is
   returned by vkAcquireNextImageKHR, and before it is presented by
   vkQueuePresentKHR. This includes transitioning the image layout
   and rendering commands.


   The presentation engine is an abstraction for the platform¡¯s compositor
   or display engine. The presentation engine controls the order in which
   presentable images are acquired for use by the application.

   This allows the platform to handle situations which require out-of-order
   return of images after presentation. At the same time, it allows the 
   application to generate command buffers referencing all of the images in
   the swapchain at initialization time, rather than in its main loop.

   Host access to fence must be externally synchronized.

   When a fence is submitted to a queue as part of a queue submission command, 
   it defines a memory dependency on the batches that were submitted as part
   of that command, and defines a fence signal operation which sets the fence
   to the signaled state.
*/

void vk_create_sync_primitives(void)
{
    VkSemaphoreCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;

    // We need one semaphone to signal that an image has been acquired and
    // is ready for rendering; and another one to signal that rendering has
    // finished and presentation can happen.

    // vk.device is the logical device that creates the semaphore.
    // &desc is a pointer to an instance of the VkSemaphoreCreateInfo structure
    // which contains information about how the semaphore is to be created.
    // When created, the semaphore is in the unsignaled state.
    VK_CHECK(qvkCreateSemaphore(vk.device, &desc, NULL, &sema_imageAvailable));
    VK_CHECK(qvkCreateSemaphore(vk.device, &desc, NULL, &sema_renderFinished));


    VkFenceCreateInfo fence_desc;
    fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_desc.pNext = NULL;
    
    // VK_FENCE_CREATE_SIGNALED_BIT specifies that the fence object
    // is created in the signaled state. Otherwise, it is created 
    // in the unsignaled state.
    fence_desc.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // vk.device is the logical device that creates the fence.
    // fence_desc is an instance of the VkFenceCreateInfo structure
    // pAllocator controls host memory allocation as described
    // in the Memory Allocation chapter. which contains information
    // about how the fence is to be created.
    // "fence_renderFinished" is a handle in which the resulting
    // fence object is returned.

    VK_CHECK(qvkCreateFence(vk.device, &fence_desc, NULL, &fence_renderFinished));
}


void vk_destroy_sync_primitives(void)
{
    ri.Printf(PRINT_ALL, " Destroy sema_imageAvailable sema_renderFinished fence_renderFinished\n");

    qvkDestroySemaphore(vk.device, sema_imageAvailable, NULL);
	qvkDestroySemaphore(vk.device, sema_renderFinished, NULL);

    // To destroy a fence, 
	qvkDestroyFence(vk.device, fence_renderFinished, NULL);
}



//  NOTE: Render Pass Compatibility
//  Framebuffers and graphics pipelines are created based on
//  a specific render pass object. They must only be used with
//  that render pass object, or one compatible with it.
//
//  Two attachment references are compatible if they have matching
//  format and sample count, or are both VK_ATTACHMENT_UNUSED
//  or the pointer that would contain the reference is NULL.
//
//  Two arrays of attachment references are compatible if all
//  corresponding pairs of attachments are compatible. If the arrays
//  are of different lengths, attachment references not present in
//  the smaller array are treated as VK_ATTACHMENT_UNUSED.
//
//  Two render passes are compatible if their corresponding color,
//  input, resolve, and depth/stencil attachment references are 
//  compatible and if they are otherwise identical except for:
//  1) Initial and final image layout in attachment descriptions
//  2) Load and store operations in attachment descriptions
//  3) Image layout in attachment references
//
//  A framebuffer is compatible with a render pass if it was created
//  using the same render pass or a compatible render pass.
static void vk_createRenderPass(VkDevice device)
{

// Before we can finish creating the pipeline, we need to tell vulkan
// about the framebuffer attachment that will be used while rendering.
// We need to specify how many color and depth buffers there will be,
// how many samples to use for each of them and how their contents 
// should be handled throughout the rendering operations.

	VkAttachmentDescription attachments[2];
	attachments[0].flags = 0;

//  The format of the color attachment should match the format of the
//  swap chain images. 
	attachments[0].format = vk.surface_format.format;
//  have something with the multisampling
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].flags = 0;
	attachments[1].format = vk.fmt_DepthStencil;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;

//  Attachments can also be cleared at the beginning of a render pass
//  instance by setting loadOp/stencilLoadOp of VkAttachmentDescription
//  to VK_ATTACHMENT_LOAD_OP_CLEAR, as described for vkCreateRenderPass.
//  loadOp and stencilLoadOp, specifying how the contents of the attachment
//  are treated before rendering and after rendering.
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

//  specifies that the contents within the render area will be cleared to
//  a uniform value, which is specified when a render pass instance is begun    
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Textueres and framebuffers in Vulkan are represented by VkImage
    // objects with a certain pixel format. however the layout of the
    // pixels in memory can change based on what you're trying to do
    // with an image.

    // Images used as color attachment
	VkAttachmentReference color_attachment_ref;
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref;
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo desc;
	desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	
    desc.attachmentCount = 2;
	desc.pAttachments = attachments;
	
    desc.subpassCount = 1;
	desc.pSubpasses = &subpass;
    
    // Subpass dependencies
    // Remember that the subpasses in a render pass automatically take care of
    // image layout transitions. These transitions are controlled by subpass
    // dependensies, which specify memory and execution dependencies between
    // subpasses. Operations right before and right after this subpass also
    // count as inplicit "subpasses".

	desc.dependencyCount = 0;
	desc.pDependencies = NULL;

	VK_CHECK(qvkCreateRenderPass(device, &desc, NULL, &vk.render_pass));
}



void vk_createFrameBuffers(uint32_t w, uint32_t h) 
{
    ri.Printf(PRINT_ALL, " Create RenderPass:  vk.render_pass \n");
	//
	// Renderpass.
	// A render pass represents a collection of attachments, subpasses,
    // and dependencies between the subpasses, and describes how the 
    // attachments are used over the course of the subpasses. The use
    // of a render pass in a command buffer is a render pass instance.
    //
    // An attachment description describes the properties of an attachment
    // including its format, sample count, and how its contents are treated
    // at the beginning and end of each render pass instance.
    //
    // A subpass represents a phase of rendering that reads and writes
    // a subset of the attachments in a render pass. Rendering commands
    // are recorded into a particular subpass of a render pass instance.
    //
    // A subpass description describes the subset of attachments that 
    // is involved in the execution of a subpass. Each subpass can read
    // from some attachments as input attachments, write to some as
    // color attachments or depth/stencil attachments, and perform
    // multisample resolve operations to resolve attachments. A subpass
    // description can also include a set of preserve attachments, 
    // which are attachments that are not read or written by the subpass
    // but whose contents must be preserved throughout the subpass.
    //
    // A subpass uses an attachment if the attachment is a color, 
    // depth/stencil, resolve, or input attachment for that subpass
    // (as determined by the pColorAttachments, pDepthStencilAttachment,
    // pResolveAttachments, and pInputAttachments members of 
    // VkSubpassDescription, respectively). A subpass does not use an
    // attachment if that attachment is preserved by the subpass.
    // The first use of an attachment is in the lowest numbered subpass
    // that uses that attachment. Similarly, the last use of an attachment
    // is in the highest numbered subpass that uses that attachment.
    //
    // The subpasses in a render pass all render to the same dimensions, 
    // and fragments for pixel (x,y,layer) in one subpass can only read
    // attachment contents written by previous subpasses at that same
    // (x,y,layer) location.
    //
    // By describing a complete set of subpasses in advance, render passes
    // provide the implementation an opportunity to optimize the storage 
    // and transfer of attachment data between subpasses. In practice, 
    // this means that subpasses with a simple framebuffer-space dependency
    // may be merged into a single tiled rendering pass, keeping the
    // attachment data on-chip for the duration of a render pass instance. 
    // However, it is also quite common for a render pass to only contain
    // a single subpass.
    //
    // Subpass dependencies describe execution and memory dependencies 
    // between subpasses. A subpass dependency chain is a sequence of
    // subpass dependencies in a render pass, where the source subpass
    // of each subpass dependency (after the first) equals the destination
    // subpass of the previous dependency.
    //
    // Execution of subpasses may overlap or execute out of order with
    // regards to other subpasses, unless otherwise enforced by an 
    // execution dependency. Each subpass only respects submission order
    // for commands recorded in the same subpass, and the vkCmdBeginRenderPass
    // and vkCmdEndRenderPass commands that delimit the render pass - 
    // commands within other subpasses are not included. This affects 
    // most other implicit ordering guarantees.
    //
    // A render pass describes the structure of subpasses and attachments
    // independent of any specific image views for the attachments. 
    // The specific image views that will be used for the attachments,
    // and their dimensions, are specified in VkFramebuffer objects. 
    // Framebuffers are created with respect to a specific render pass
    // that the framebuffer is compatible with (see Render Pass Compatibility).
    // Collectively, a render pass and a framebuffer define the complete
    // render target state for one or more subpasses as well as the 
    // algorithmic dependencies between the subpasses.
    //
    // The various pipeline stages of the drawing commands for a given
    // subpass may execute concurrently and/or out of order, both within 
    // and across drawing commands, whilst still respecting pipeline order.
    // However for a given (x,y,layer,sample) sample location, certain
    // per-sample operations are performed in rasterization order.
    
	vk_createRenderPass(vk.device);


    // Framebuffers for each swapchain image.
	// The attachments specified during render pass creation are bound
    // by wrapping them into a VkFramebuffer object. A framebuffer object
    // references all of the VkImageView objects that represent the attachments
    // The image that we have to use as attachment depends on which image
    // the the swap chain returns when we retrieve one for presentation
    // this means that we have to create a framebuffer for all of the images
    // in the swap chain and use the one that corresponds to the retrieved
    // image at draw time.

    ri.Printf(PRINT_ALL, " Create vk.framebuffers \n");

    // Render passes operate in conjunction with framebuffers. 
    // Framebuffers represent a collection of specific memory
    // attachments that a render pass instance uses.

    uint32_t i;
    for (i = 0; i < vk.swapchain_image_count; i++)
    {
        // set color and depth attachment
        VkImageView attachments[2] = {
            vk.swapchain_image_views[i], vk.depth_image_view };
        
        VkFramebufferCreateInfo desc;
        desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;

        // renderPass is a render pass that defines what render
        // passes the framebuffer will be compatible with. See 
        // Render Pass Compatibility for details.
        desc.renderPass = vk.render_pass;
        desc.attachmentCount = 2;
        // pAttachments is an array of VkImageView handles, each 
        // of which will be used as the corresponding attachment
        // in a render pass instance.
        desc.pAttachments = attachments;
        desc.width = w;
        desc.height = h;
        desc.layers = 1;

        VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers[i]));

        // Applications must ensure that all accesses to memory that backs
        // image subresources used as attachments in a given renderpass instance 
        // either happen-before the load operations for those attachments,
        // or happen-after the store operations for those attachments.
        //
        // For depth/stencil attachments, each aspect can be used separately
        // as attachments and non-attachments as long as the non-attachment 
        // accesses are also via an image subresource in either the
        // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL layout
        // or the VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        // layout, and the attachment resource uses whichever of those two 
        // layouts the image accesses do not. Use of non-attachment aspects
        // in this case is only well defined if the attachment is used in the
        // subpass where the non-attachment access is being made, or the layout
        // of the image subresource is constant throughout the entire render
        // pass instance, including the initialLayout and finalLayout.
        //
        // These restrictions mean that the render pass has full knowledge of
        // all uses of all of the attachments, so that the implementation is
        // able to make correct decisions about when and how to perform layout
        // transitions, when to overlap execution of subpasses, etc.
    }
}


void vk_destroyFrameBuffers(void)
{
    // we should delete the framebuffers before the image views
    // and the render pass that they are based on.
    ri.Printf(PRINT_ALL, " Destroy vk.framebuffers vk.swapchain_image_views vk.swapchain\n");

    uint32_t i;
	for (i = 0; i < vk.swapchain_image_count; i++)
    {
		qvkDestroyFramebuffer(vk.device, vk.framebuffers[i], NULL);
		qvkDestroyImageView(vk.device, vk.swapchain_image_views[i], NULL);
    }

    qvkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);

    qvkDestroyRenderPass(vk.device, vk.render_pass, NULL);
}


void vk_begin_frame(void)
{
  
    // An application can acquire use of a presentable image with vkAcquireNextImageKHR. 
    // After acquiring a presentable image and before modifying it, the application must
    // use a synchronization primitive to ensure that the presentation engine has 
    // finished reading from the image. The application can then transition the image's
    // layout, queue rendering commands to it, etc. Finally, the application presents 
    // the image with vkQueuePresentKHR, which releases the acquisition of the image.

    // To acquire an available presentable image to use, and retrieve the index of 
    // that image If timeout is UINT64_MAX, the timeout period is treated as infinite,
    // and vkAcquireNextImageKHR will block until an image is acquired or an error occurs.
    
    // An application must wait until either the semaphore or fence is signaled
    // before accessing the image's data.
	VK_CHECK(qvkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX,
        sema_imageAvailable, VK_NULL_HANDLE, &vk.idx_swapchain_image));


    //  User could call method vkWaitForFences to wait for completion. A fence is a 
    //  very heavyweight synchronization primitive as it requires the GPU to flush
    //  all caches at least, and potentially some additional synchronization. Due to
    //  those costs, fences should be used sparingly. In particular, try to group
    //  per-frame resources and track them together. To wait for one or more fences
    //  to enter the signaled state on the host, call qvkWaitForFences.

    //  If the condition is satisfied when vkWaitForFences is called, then 
    //  vkWaitForFences returns immediately. If the condition is not satisfied at 
    //  the time vkWaitForFences is called, then vkWaitForFences will block and 
    //  wait up to timeout nanoseconds for the condition to become satisfied.

	VK_CHECK(qvkWaitForFences(vk.device, 1, &fence_renderFinished, VK_FALSE, 1e9));
 
    //  To set the state of fences to unsignaled from the host
    //  "1" is the number of fences to reset. 
    //  "fence_renderFinished" is the fence handle to reset.
	VK_CHECK(qvkResetFences(vk.device, 1, &fence_renderFinished));

    //  commandBuffer must not be in the recording or pending state.
    
    // begin_info is an instance of the VkCommandBufferBeginInfo structure,
    // which defines additional information about how the command buffer 
    // begins recording.
	VkCommandBufferBeginInfo begin_info;
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that
    // each recording of the command buffer will only be submitted
    // once, and the command buffer will be reset and recorded again
    // between each submission.
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

    // To begin recording a command buffer
	VK_CHECK(qvkBeginCommandBuffer(vk.command_buffer, &begin_info));

	// Ensure visibility of geometry buffers writes.


{

    // vkCmdPipelineBarrier is a synchronization command that inserts 
    // a dependency between commands submitted to the same queue, or 
    // between commands in the same subpass. When vkCmdPipelineBarrier
    // is submitted to a queue, it defines a memory dependency between
    // commands that were submitted before it, and those submitted
    // after it.
	VkBufferMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = vk_getIndexBuffer();
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;

    // If vkCmdPipelineBarrier was recorded outside a render pass instance, 
    // the first synchronization scope includes all commands that occur earlier
    // in submission order. The second synchronization scope includes all
    // commands that occur later in submission order.  
    //
    
    // VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT specifies read access 
    // to a vertex buffer as part of a drawing command, bound by
    // vkCmdBindVertexBuffers.
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	
    // To record a pipeline barrier
    qvkCmdPipelineBarrier(vk.command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier, 0, NULL);

    // VK_ACCESS_INDEX_READ_BIT specifies read access to an index buffer 
    // as part of an indexed drawing command, bound by vkCmdBindIndexBuffer.
    barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
    qvkCmdPipelineBarrier(vk.command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier, 0, NULL);

}


	// Begin render pass.
	VkClearValue clear_values[2];
	/// ignore clear_values[0] which corresponds to color attachment
	clear_values[1].depthStencil.depth = 1.0;
	clear_values[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo renderPass_beginInfo;
	renderPass_beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPass_beginInfo.pNext = NULL;
	renderPass_beginInfo.renderPass = vk.render_pass;
	renderPass_beginInfo.framebuffer = vk.framebuffers[vk.idx_swapchain_image];

    renderPass_beginInfo.renderArea = get_scissor_rect();

    renderPass_beginInfo.clearValueCount = 2;
	renderPass_beginInfo.pClearValues = clear_values;

	qvkCmdBeginRenderPass(vk.command_buffer, &renderPass_beginInfo, VK_SUBPASS_CONTENTS_INLINE);

}


void vk_end_frame(void)
{
	qvkCmdEndRenderPass(vk.command_buffer);
	
    VK_CHECK(qvkEndCommandBuffer(vk.command_buffer));


	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    // Queue submission and synchronization
	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &sema_imageAvailable;
	submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk.command_buffer;
	submit_info.signalSemaphoreCount = 1;
    // specify which semaphones to signal once the command buffers
    // have finished execution
	submit_info.pSignalSemaphores = &sema_renderFinished;


    //  queue is the queue that the command buffers will be submitted to.
    //  1 is the number of elements in the pSubmits array.
    //  pSubmits is a pointer to an array of VkSubmitInfo structures,
    //  each specifying a command buffer submission batch.
    //
    //  fence_renderFinished is an optional handle to a fence to be signaled 
    //  once all submitted command buffers have completed execution. 
    //  If fence is not VK_NULL_HANDLE, it defines a fence signal operation.
    //
    //  Submission can be a high overhead operation, and applications should 
    //  attempt to batch work together into as few calls to vkQueueSubmit as possible.
    //
    //  vkQueueSubmit is a queue submission command, with each batch defined
    //  by an element of pSubmits as an instance of the VkSubmitInfo structure.
    //  Batches begin execution in the order they appear in pSubmits, but may
    //  complete out of order.
    //
    //  Fence and semaphore operations submitted with vkQueueSubmit 
    //  have additional ordering constraints compared to other 
    //  submission commands, with dependencies involving previous and
    //  subsequent queue operations. 
    //
    //  The order that batches appear in pSubmits is used to determine
    //  submission order, and thus all the implicit ordering guarantees
    //  that respect it. Other than these implicit ordering guarantees
    //  and any explicit synchronization primitives, these batches may
    //  overlap or otherwise execute out of order. If any command buffer
    //  submitted to this queue is in the executable state, it is moved
    //  to the pending state. Once execution of all submissions of a 
    //  command buffer complete, it moves from the pending state,
    //  back to the executable state.
    //
    //  If a command buffer was recorded with the 
    //  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT flag,
    //  it instead moves back to the invalid state.
       
    //  To submit command buffers to a queue 
    
    VK_CHECK(qvkQueueSubmit(vk.queue, 1, &submit_info, fence_renderFinished));

    VkPresentInfoKHR present_info;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &sema_renderFinished;

    // specify the swap chains to present images to
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk.swapchain;
    // specify the index of the image for each swap chain
	present_info.pImageIndices = &vk.idx_swapchain_image;
	present_info.pResults = NULL;

    // Each element of pSwapchains member of pPresentInfo must be a 
    // swapchain that is created for a surface for which presentation
    // is supported from queue as determined using a call to 
    // vkGetPhysicalDeviceSurfaceSupportKHR

    
    // After queueing all rendering commands and transitioning the
    // image to the correct layout, to queue an image for presentation.
    // queue is a queue that is capable of presentation to the target 
    // surface's platform on the same device as the image's swapchain.
    VkResult result = qvkQueuePresentKHR(vk.queue, &present_info);
    if(result == VK_SUCCESS)
    {
        return;
    }
    else if( (result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_ERROR_SURFACE_LOST_KHR))
    {
        // we first call vkDeviceWaitIdle because we 
        // shouldn't touch resources that still be in use
        qvkDeviceWaitIdle(vk.device);
        // recreate the objects that depend on the swap chain and the window size

        vk_recreateSwapChain();
    }
}

