#include "AfterglowTransparencyPass.h"
#include "AfterglowPhysicalDevice.h"

AfterglowTransparencyPass::AfterglowTransparencyPass(
	AfterglowDevice& device, AfterglowPassInterface* prevPass) :
	AfterglowPassInterface(device, AfterglowPassInterface::ExtentMode::Swapchain) {
	
	auto msaaSampleCount = device.physicalDevice().msaaSampleCount();
	auto depthFormat = device.physicalDevice().findDepthFormat();

	AfterglowSubpassContext::PassUsage colorUsage = AfterglowSubpassContext::PassUsage::Local;
	AfterglowSubpassContext::PassUsage depthUsage = AfterglowSubpassContext::PassUsage::Export;
	int32_t importColorAttachmentIndex = invalidAttachmentIndex();
	if (prevPass) {
		if (isValidAttachment(prevPass->exportColorAttachmentIndex(ColorAttachment::ColorMS))) {
			colorUsage = AfterglowSubpassContext::PassUsage::Import;
		}
		if (isValidAttachment(prevPass->exportDepthAttachmentIndex())) {
			depthUsage = AfterglowSubpassContext::PassUsage::ImportExport;
		}
		// For transparncy pass sample scene color.
		if (isValidAttachment(prevPass->exportColorAttachmentIndex(ColorAttachment::Color))) {
			importColorAttachmentIndex = subpassContext().appendAttachment(
				AfterglowSubpassContext::nonTransferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Import)
			);
		}
	}

	// Create attachments (Reuse image if import attachments were exist. )
	uint32_t colorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), colorUsage, msaaSampleCount)
	);
	uint32_t depthAttachmentIndex = subpassContext().appendDepthAttachment(
		AfterglowSubpassContext::depthAttachment(depthFormat, depthUsage, msaaSampleCount)
	);
	uint32_t resolveAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Export)
	);

	// Pass IO
	recordImportAttachments(prevPass, ColorAttachment::ColorMS, colorAttachmentIndex, depthAttachmentIndex);
	recordImportAttachments(prevPass, ColorAttachment::Color, importColorAttachmentIndex);
	setExportDepthAttachment(depthAttachmentIndex, render::depthTextureName);
	// TransparencyPass is the last object based pass, so do not export ColorMS attachment.
	setExportColorAttachment(ColorAttachment::Color, resolveAttachmentIndex, render::sceneColorTextureName);

	////////////////////////////////////////
	// Transparency Subpass
	////////////////////////////////////////
	std::string transparencySubpassName(passName());
	subpassContext().appendSubpass(transparencySubpassName);

	subpassContext().bindColorAttachment(transparencySubpassName, colorAttachmentIndex);
	subpassContext().bindDepthAttachment(transparencySubpassName, depthAttachmentIndex);
	subpassContext().bindResolveAttachment(transparencySubpassName, resolveAttachmentIndex);

	auto& transparencyDependency = subpassContext().makeDependency(
		AfterglowSubpassContext::undefinedSubpassName(), transparencySubpassName
	);
}
