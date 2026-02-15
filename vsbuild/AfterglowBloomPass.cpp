#include "AfterglowBloomPass.h"

AfterglowBloomPass::AfterglowBloomPass(AfterglowPassInterface& srcPass, float scale, uint32_t sequenceID) : 
	AfterglowPassInterface(srcPass.device(), AfterglowPassInterface::ExtentMode::Swapchain),
	_passName(std::format("Bloom{}", sequenceID)) {
	setScale({ scale, scale });

	// Create attachments
	uint32_t importColorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Import)
	);
	uint32_t horizontalBlurAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat())
	);
	uint32_t combinationAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(colorFormat(), AfterglowSubpassContext::PassUsage::Export)
	);

	// PassIO
	recordImportAttachments(&srcPass, ColorAttachment::Color, importColorAttachmentIndex);
	setExportColorAttachment(ColorAttachment::Color, combinationAttachmentIndex, combinedTextureName());

	// Subpasses
	// Horizontal blur
	subpassContext().appendSubpass(horizontalBlurSubpassName());
	subpassContext().bindColorAttachment(horizontalBlurSubpassName(), horizontalBlurAttachmentIndex);
	subpassContext().makeDependency(AfterglowSubpassContext::undefinedSubpassName(), horizontalBlurSubpassName());

	// Vertical blur and combination
	subpassContext().appendSubpass(verticalBlurCombinationSubpassName());
	subpassContext().bindInputAttachment(verticalBlurCombinationSubpassName(), horizontalBlurAttachmentIndex, std::string(horizontalBlurTextureName()));
	subpassContext().bindColorAttachment(verticalBlurCombinationSubpassName(), combinationAttachmentIndex);
	subpassContext().makeDependency(
		horizontalBlurSubpassName(), verticalBlurCombinationSubpassName(), AfterglowSubpassContext::fragmentRColorDependency
	);
}
