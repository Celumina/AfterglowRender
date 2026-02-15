#include "AfterglowForwardPass.h"
#include "AfterglowPhysicalDevice.h"

AfterglowForwardPass::AfterglowForwardPass(
	AfterglowDevice& device, AfterglowPassInterface* prevPass) :
	AfterglowPassInterface(device, ExtentMode::Swapchain) {
	
	auto msaaSampleCount = device.physicalDevice().msaaSampleCount();
	auto depthFormat = device.physicalDevice().findDepthFormat();
	
	AfterglowSubpassContext::PassUsage colorUsage = AfterglowSubpassContext::PassUsage::Export;
	AfterglowSubpassContext::PassUsage depthUsage = AfterglowSubpassContext::PassUsage::Export;
	if (prevPass) {
		if (isValidAttachment(prevPass->exportColorAttachmentIndex(ColorAttachment::ColorMS))) {
			colorUsage = AfterglowSubpassContext::PassUsage::ImportExport;
		}
		if (isValidAttachment(prevPass->exportDepthAttachmentIndex())) {
			depthUsage = AfterglowSubpassContext::PassUsage::ImportExport;
		}
	}

	// TODO: Abstract create attachment and append subpass templates.
	// Create attachments
	uint32_t colorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), colorUsage, msaaSampleCount)
	);
	uint32_t depthAttachmentIndex = subpassContext().appendDepthAttachment(
		AfterglowSubpassContext::depthAttachment(depthFormat, depthUsage, msaaSampleCount)
	);
	uint32_t resolveAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Export)
	);

	// PassIO
	recordImportAttachments(
		prevPass, 
		ColorAttachment::ColorMS, 
		colorAttachmentIndex, 
		depthAttachmentIndex
	);
	recordImportAttachments(prevPass, ColorAttachment::Color);
	setExportDepthAttachment(depthAttachmentIndex, render::depthTextureName);
	setExportColorAttachment(ColorAttachment::ColorMS, colorAttachmentIndex, render::sceneColorMSTextureName);
	setExportColorAttachment(ColorAttachment::Color, resolveAttachmentIndex, render::sceneColorTextureName);

	////////////////////////////////////////
	// Forward Subpass
	////////////////////////////////////////
	std::string forwardSubpassName(passName());
	subpassContext().appendSubpass(forwardSubpassName);

	subpassContext().bindColorAttachment(forwardSubpassName, colorAttachmentIndex);
	subpassContext().bindDepthAttachment(forwardSubpassName, depthAttachmentIndex);
	subpassContext().bindResolveAttachment(forwardSubpassName, resolveAttachmentIndex);

	// Dependency is define that dstSubpass excution always later than srcSubpass
	// src and dst subpass index.
	auto& forwarDependency = subpassContext().makeDependency(
		AfterglowSubpassContext::undefinedSubpassName(), forwardSubpassName
	);
}