#include "AfterglowRenderPass.h"
#include "AfterglowPhysicalDevice.h"

AfterglowRenderPass::AfterglowRenderPass(AfterglowSwapchain& swapchain) :
	_swapchain(swapchain) {
}

AfterglowRenderPass::~AfterglowRenderPass() {
	destroy(vkDestroyRenderPass, device(), data(), nullptr);
}

inline AfterglowDevice& AfterglowRenderPass::device() noexcept {
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

	/**
	* @note: For linear rendering before Tonemapping.
	* @desc:
	*	Candidates:
	*		VK_FORMAT_R8G8B8A8_SRGB               <32bit/px>        SDR non-linear rendering directly, no Tonemapping.
	*		VK_FORMAT_B10G11R11_UFLOAT_PACK32     <32bit/px>        LHDR linear rendering, were used in most Game Engines.
	*/
	VkFormat colorFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	VkFormat presentFormat = _swapchain.imageFormat();
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
		AfterglowSubpassContext::presentAttachment(presentFormat)
	);

	////////////////////////////////////////
	// Forward
	////////////////////////////////////////
	_subpassContext->appendSubpass(render::Domain::Forward);
	_subpassContext->bindColorAttachment(render::Domain::Forward, colorIndex);
	_subpassContext->bindDepthAttachment(render::Domain::Forward, depthIndex);

	_subpassContext->bindResolveAttachment(render::Domain::Forward, forwardColorResolveIndex);

	// Dependency is define that dstSubpass excution always later than srcSubpass
	// src and dst subpass index.
	auto& forwarDependency = 
		_subpassContext->makeDependency(render::Domain::Undefined, render::Domain::Forward);

	////////////////////////////////////////
	// Transparency
	////////////////////////////////////////
	_subpassContext->appendSubpass(render::Domain::Transparency);
	// Both support Input and output
	_subpassContext->bindInputAttachment(render::Domain::Transparency, forwardColorResolveIndex, "sceneColorTexture");
	_subpassContext->bindInputDepthAttachment(render::Domain::Transparency, depthIndex, "depthTexture");
	_subpassContext->bindColorAttachment(render::Domain::Transparency, colorIndex);
	_subpassContext->bindDepthAttachment(render::Domain::Transparency, depthIndex);

	_subpassContext->bindResolveAttachment(render::Domain::Transparency, transparencyColorResolveIndex);

	auto& transparencyDependency = _subpassContext->makeDependency(
		render::Domain::Forward, render::Domain::Transparency, AfterglowSubpassContext::fragmentRWDependency
	);
	
	////////////////////////////////////////
	// Postprocess
	////////////////////////////////////////
	_subpassContext->appendSubpass(render::Domain::PostProcess);
	_subpassContext->bindInputAttachment(render::Domain::PostProcess, transparencyColorResolveIndex, "sceneColorTexture");
	_subpassContext->bindInputDepthAttachment(render::Domain::PostProcess, depthIndex, "depthTexture");
	_subpassContext->bindColorAttachment(render::Domain::PostProcess, presentColorIndex);

	auto& postProcessDependency = _subpassContext->makeDependency(
		render::Domain::Transparency, render::Domain::PostProcess, AfterglowSubpassContext::fragmentRWColorRDepthDependency
	);

	////////////////////////////////////////
	// UserInterface
	////////////////////////////////////////
	_subpassContext->appendSubpass(render::Domain::UserInterface);
	_subpassContext->bindColorAttachment(render::Domain::UserInterface, presentColorIndex);
	auto& userInterfaceDependency = _subpassContext->makeDependency(
		render::Domain::PostProcess, render::Domain::UserInterface, AfterglowSubpassContext::fragmentWColorDependency
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