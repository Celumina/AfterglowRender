#include "AfterglowRenderPass.h"
#include "AfterglowPhysicalDevice.h"

AfterglowRenderPass::AfterglowRenderPass(AfterglowDevice& device) :
	_device(device) {

}

AfterglowRenderPass::~AfterglowRenderPass() {
	destroy(vkDestroyRenderPass, device(), data(), nullptr);
}

void AfterglowRenderPass::initCreateInfo() {
	// RenderPass
	// RenderPass to specify: 
	// how many color and depth buffers there will be, how many samples to use
	// for each of them and how their contents should be handled throughout the rendering operations
	info().sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info().attachmentCount = _subpassContext.attachmentCount();
	info().pAttachments = _subpassContext.attachments().data();
	info().subpassCount = _subpassContext.subpassCount();
	info().pSubpasses = _subpassContext.subpasses().data();
	info().dependencyCount = _subpassContext.dependencyCount();
	info().pDependencies = _subpassContext.dependencies().data();
}

void AfterglowRenderPass::create() {
	if (vkCreateRenderPass(device(), &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create render pass.");
	}
	// Don't destroy it so fast, we use it to verify command buffer.
	// _subpassContext.reset();
}