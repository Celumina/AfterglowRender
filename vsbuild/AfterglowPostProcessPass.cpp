#include "AfterglowPostProcessPass.h"
#include "AfterglowPhysicalDevice.h"
#include "SwapchainConfigurations.h"

AfterglowPostProcessPass::AfterglowPostProcessPass(AfterglowDevice& device, AfterglowPassInterface* prevPass) :
	AfterglowPassInterface(device, AfterglowPassInterface::ExtentMode::Swapchain) {

	auto msaaSampleCount = device.physicalDevice().msaaSampleCount();
	auto depthFormat = device.physicalDevice().findDepthFormat();

	// Create attachments (Reuse image if import attachments were exist. )
	uint32_t importColorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Import)
	);
	uint32_t depthAttachmentIndex = subpassContext().appendDepthAttachment(
		AfterglowSubpassContext::depthAttachment(depthFormat, AfterglowSubpassContext::PassUsage::Import, msaaSampleCount)
	);
	uint32_t exportColorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::presentAttachment(swapchain::presentFormat, AfterglowSubpassContext::PassUsage::Export)
	);

	// PassIO
	recordImportAttachments(
		prevPass, ColorAttachment::Color, importColorAttachmentIndex, depthAttachmentIndex
	);
	setExportColorAttachment(ColorAttachment::Color, exportColorAttachmentIndex, render::sceneColorTextureName, true);

	////////////////////////////////////////
	// Postprocess Subpass
	////////////////////////////////////////
	std::string postProcessSubpassName(passName());
	subpassContext().appendSubpass(postProcessSubpassName);

	subpassContext().bindColorAttachment(postProcessSubpassName, exportColorAttachmentIndex);

	auto& postProcessDependency = subpassContext().makeDependency(
		AfterglowSubpassContext::undefinedSubpassName(), postProcessSubpassName
	);
}