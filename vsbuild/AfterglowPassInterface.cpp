#include "AfterglowPassInterface.h"
#include "ExceptionUtilities.h"

AfterglowPassInterface::AfterglowPassInterface(AfterglowDevice& device, ExtentMode extentMode, VkFormat colorFormat) : 
	_renderPass(device), 
	_extentMode(extentMode), 
	_colorFormat(colorFormat) {
	if (_extentMode == ExtentMode::Swapchain) {
		_extentScale.scale = { 1.0f, 1.0f };
	}
	_exportColorAttachmentIndices.fill(invalidAttachmentIndex());
}

glm::u32vec2 AfterglowPassInterface::extent() const {
	if (_extentMode != ExtentMode::Fixed) {
		EXCEPT_CLASS_RUNTIME("Swapchain frame bufer have not fixed extent size.");
	}
	return _extentScale.extent;
}

void AfterglowPassInterface::setExtent(glm::u32vec2 extent) {
	if (_extentMode != ExtentMode::Fixed) {
		EXCEPT_CLASS_RUNTIME("Swapchain frame bufer can not adjust the extent size.");
	}
	_extentScale.extent = extent;
}

glm::vec2 AfterglowPassInterface::scale() const {
	if (_extentMode != ExtentMode::Swapchain) {
		EXCEPT_CLASS_RUNTIME("Fixed frame bufer have not relavant scale. ");
	}
	return _extentScale.scale;
}

void AfterglowPassInterface::setScale(glm::vec2 scale) {
	if (_extentMode != ExtentMode::Swapchain) {
		EXCEPT_CLASS_RUNTIME("Fixed frame bufer can not adjust the relavant scale. ");
	}
	_extentScale.scale = scale;
}

AfterglowPassInterface::ImportAttachment* AfterglowPassInterface::findImportAttachment(uint32_t currentAttachment) {
	for (auto& importAttachment : _importAttachments) {
		if (importAttachment.destAttachmentIndex == currentAttachment) {
			return &importAttachment;
		}
	}
	return nullptr;
}

bool AfterglowPassInterface::isExportColorAttachment(uint32_t inAttachmentIndex) const noexcept {
	for (int32_t attachmentIndex : _exportColorAttachmentIndices) {
		if (attachmentIndex == inAttachmentIndex) {
			return true;
		}
	}
	return false;
}

const AfterglowPassInterface::BarrierArray* AfterglowPassInterface::exportBarriers() {
	return _exportBarriers.get();
}

const AfterglowPassInterface::BarrierArray& AfterglowPassInterface::recreateExportBarriers(const ImageArray* exportColorImages, const VkImage exportDepthImage) {
	clearBarriers();
	if (exportColorImages->size() != exportColorAttachmentIndices().size()) {
		EXCEPT_CLASS_INVALID_ARG("Color image array size is not match the export color attachments size.");
	}
	else {
		for (uint32_t index = 0; index < exportColorImages->size(); ++index) {
			appendColorBarrier((*exportColorImages)[index], exportColorAttachmentIndices()[index]);
		}
	}
	if (isValidAttachment(exportDepthAttachmentIndex())) {
		appendDepthBarrier(exportDepthImage);
	}
}

VkImageMemoryBarrier& AfterglowPassInterface::appendColorBarrier(const VkImage exportColorImage, uint32_t exportAttachmentIndex) {
	auto& barrier = _exportBarriers->emplace_back(VkImageMemoryBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = subpassContext().attachment(exportAttachmentIndex).finalLayout,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.image = exportColorImage,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
	});
	_exportBarrierSrcPipelineStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// Color to Present transition.
	if (isValidAttachment(presentAttachmentIndex())) {
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	return barrier;
}

VkImageMemoryBarrier& AfterglowPassInterface::appendDepthBarrier(const VkImage exportDepthImage) {
	if (!isValidAttachment(exportDepthAttachmentIndex())) {
		EXCEPT_CLASS_RUNTIME("Pass has not export depth attachment.");
	}
	auto& barrier = _exportBarriers->emplace_back(VkImageMemoryBarrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = subpassContext().attachment(exportDepthAttachmentIndex()).finalLayout,
			.newLayout = AfterglowSubpassContext::depthAttachmentRWLayout(),
			.image = exportDepthImage,
			.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 },
		});
	_exportBarrierSrcPipelineStage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	return barrier;
}

void AfterglowPassInterface::clearBarriers() {
	_exportBarriers = std::make_unique<BarrierArray>();
	_exportBarrierSrcPipelineStage = VK_PIPELINE_STAGE_NONE;
}

void AfterglowPassInterface::recordImportAttachments(
	AfterglowPassInterface* srcPass, 
	ColorAttachment importColorAttachmentIndex,
	int32_t destColorAttachmentIndex, 
	int32_t destDepthAttachmentIndex
) {
	// Record input attachments
	if (!srcPass) {
		return;
	}

	if (srcPass->exportColorAttachmentIndices().size() > util::EnumValue(importColorAttachmentIndex)
		&& isValidAttachment(srcPass->exportColorAttachmentIndex(importColorAttachmentIndex))
		&& isValidAttachment(destColorAttachmentIndex)) {
		bool isMultipleSample = 
			srcPass->subpassContext().attachment(srcPass->exportColorAttachmentIndex(importColorAttachmentIndex)).samples
			!= VK_SAMPLE_COUNT_1_BIT;

		importAttachments().emplace_back(
			std::string(srcPass->passName()), 
			*srcPass->exportColorAttachmentName(importColorAttachmentIndex),
			srcPass->exportColorAttachmentIndex(importColorAttachmentIndex),
			destColorAttachmentIndex, 
			isMultipleSample
		);
	}
	if (isValidAttachment(srcPass->exportDepthAttachmentIndex())
		&& isValidAttachment(destDepthAttachmentIndex)) {
		bool isMultipleSample = 
			srcPass->subpassContext().attachment(srcPass->exportDepthAttachmentIndex()).samples != VK_SAMPLE_COUNT_1_BIT;

		importAttachments().emplace_back(
			std::string(srcPass->passName()), 
			srcPass->exportDepthAttachmentName(),
			srcPass->exportDepthAttachmentIndex(), 
			destDepthAttachmentIndex, 
			isMultipleSample
		);
	}
}

void AfterglowPassInterface::setExportDepthAttachment(uint32_t attachmentIndex, std::string_view name) {
	_exportDepthAttachmentIndex = attachmentIndex;
	_exportDepthAttachmentName = name;
}

void AfterglowPassInterface::setExportColorAttachment(ColorAttachment attachment, uint32_t attachmentIndex, std::string_view name, bool isPresentAttachment) {
	_exportColorAttachmentIndices[util::EnumValue(attachment)] = attachmentIndex;
	_exportColorAttachmentNames[util::EnumValue(attachment)] = std::make_unique<std::string>(name);
	if (isPresentAttachment) {
		_presentAttachmentIndex = attachmentIndex;
	}
}
