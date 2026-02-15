#include "AfterglowDownSamplingPass.h"

AfterglowDownSamplingPass::AfterglowDownSamplingPass(AfterglowPassInterface& srcPass, float scale, uint32_t sequenceID) :
	AfterglowPassInterface(srcPass.device(), AfterglowPassInterface::ExtentMode::Swapchain),
	_passName(std::format("DownSampling{}", sequenceID)) {

	// Scale
	setScale({ scale, scale });

	// Create attachments
	uint32_t importColorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Import)
	);
	uint32_t exportColorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Export)
	);

	// PassIO
	recordImportAttachments(
		&srcPass, ColorAttachment::Color, importColorAttachmentIndex
	);
	// Handle First sequenceID import texture name.
	if (sequenceID == 0) {
		importAttachments()[0].attachmentName = downSampledTextureName();
	}
	setExportColorAttachment(ColorAttachment::Color, exportColorAttachmentIndex, downSampledTextureName());

	// Down sampling subpass
	std::string downSamplingSubpassName(passName());
	subpassContext().appendSubpass(downSamplingSubpassName);

	subpassContext().bindColorAttachment(downSamplingSubpassName, exportColorAttachmentIndex);

	auto& downSamplingDependency = subpassContext().makeDependency(
		AfterglowSubpassContext::undefinedSubpassName(), downSamplingSubpassName
	);
}
