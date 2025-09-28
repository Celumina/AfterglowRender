#include "AfterglowRenderPass.h"

AfterglowRenderPass::AfterglowRenderPass(AfterglowSwapchain& swapchain) :
	_swapchain(swapchain) {
}

AfterglowRenderPass::~AfterglowRenderPass() {
	destroy(vkDestroyRenderPass, device(), data(), nullptr);
}

inline AfterglowDevice& AfterglowRenderPass::device() {
	return _swapchain.device();
}

AfterglowSwapchain& AfterglowRenderPass::swapchain() {
	return _swapchain;
}

AfterglowSubpassContext& AfterglowRenderPass::subpassContext() {
	if (!_subpassContext) {
		initCreateInfo();
	}
	return *_subpassContext.get();
}

void AfterglowRenderPass::initCreateInfo() {
	// TODO: 
	//_subpassContext >> {
	//		.domain = render::Domain::Forward, 
	//		.tempAttachments = {
	//			Attachment::MsaaColor, 
	//			Attachment::MsaaDepth, 
	//		}, 
	//		.outputAttachment = {
	//			Attachment::ResolveColor,
	//			Attachment::ResolveDepth
	//		}
	//	} >> {
	//		.domain = render::Domain::PostProcess,
	//		.inputAttachments
	//		.inputAttachments = {
	//			Attachment::ResolveColor,
	//			Attachment::ResolveDepth
	//		}
	//		.outputAttachment = {
	//			Attachment::PresentColor
	//		}
	//	};

	//_subpassContext.recreate(forward >> transparency >> postProcess >> userInterface);

	auto& physicalDevice = device().physicalDevice();
	VkFormat colorFormat = _swapchain.imageFormat();
	VkFormat depthFormat = physicalDevice.findDepthFormat();
	VkSampleCountFlagBits msaaSampleCount = physicalDevice.msaaSampleCount();

	// TODO: move subpass to genernal definition file.
	_subpassContext = std::make_unique<AfterglowSubpassContext>();

	// Attachments
	uint32_t colorIndex = _subpassContext->appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat, msaaSampleCount)
	);
	uint32_t depthIndex = _subpassContext->appendDepthAttachment(
		AfterglowSubpassContext::depthAttachment(depthFormat, msaaSampleCount)
	);
	uint32_t forwardColorResolveIndex = _subpassContext->appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat)
	);
	uint32_t transparencyColorResolveIndex = _subpassContext->appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat)
	);
	uint32_t presentColorIndex = _subpassContext->appendAttachment(
		AfterglowSubpassContext::presentAttachment(colorFormat)
	);

	// Forward
	_subpassContext->appendSubpass(render::Domain::Forward);
	_subpassContext->bindColorAttachment(render::Domain::Forward, colorIndex);
	_subpassContext->bindDepthAttachment(render::Domain::Forward, depthIndex);

	_subpassContext->bindResolveAttachment(render::Domain::Forward, forwardColorResolveIndex);

	// Dependency is define that dstSubpass excution always later than srcSubpass
	// src and dst subpass index.
	auto& forwarDependency = 
		_subpassContext->makeDependency(render::Domain::Undefined, render::Domain::Forward);

	// Transparency
	_subpassContext->appendSubpass(render::Domain::Transparency);
	// Both support Input and output
	_subpassContext->bindInputAttachment(render::Domain::Transparency, forwardColorResolveIndex, "sceneColorTexture");
	_subpassContext->bindInputAttachment(render::Domain::Transparency, depthIndex, "depthTexture");
	_subpassContext->bindColorAttachment(render::Domain::Transparency, colorIndex);
	_subpassContext->bindDepthAttachment(render::Domain::Transparency, depthIndex);

	_subpassContext->bindResolveAttachment(render::Domain::Transparency, transparencyColorResolveIndex);

	auto& transparencyDependency = _subpassContext->makeDependency(
		render::Domain::Forward, render::Domain::Transparency, AfterglowSubpassContext::fragmentRWDependency
	);
	
	// Postprocess
	_subpassContext->appendSubpass(render::Domain::PostProcess);
	_subpassContext->bindInputAttachment(render::Domain::PostProcess, transparencyColorResolveIndex, "sceneColorTexture");
	_subpassContext->bindInputAttachment(render::Domain::PostProcess, depthIndex, "depthTexture");
	_subpassContext->bindColorAttachment(render::Domain::PostProcess, presentColorIndex);

	auto& postProcessDependency = _subpassContext->makeDependency(
		render::Domain::Transparency, render::Domain::PostProcess, AfterglowSubpassContext::fragmentRWColorRDepthDependency
	);

	// RenderPass
	// RenderPass to specify: 
	// how many color and depth buffers there will be, how many samples to use
	// for each of them and how their contents should be handled throughout the rendering operations
	info().sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info().attachmentCount = _subpassContext->attachmentCount();
	info().pAttachments = _subpassContext->attachments().data();
	info().subpassCount = _subpassContext->subpassCount();
	info().pSubpasses = _subpassContext->subpasses().data();
	info().dependencyCount = _subpassContext->dependencyCount();
	info().pDependencies = _subpassContext->dependencies().data();
}
void AfterglowRenderPass::create() {
	if (vkCreateRenderPass(device(), &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create render pass.");
	}
	// Don't destroy it so fast, we use it to verify command buffer.
	// _subpassContext.reset();
}