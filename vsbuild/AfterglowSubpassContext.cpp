#include "AfterglowSubpassContext.h"
#include "AfterglowUtilities.h"
#include "RenderDefinitions.h"
#include "ExceptionUtilities.h"
#include "RenderConfigurations.h"

VkAttachmentDescription AfterglowSubpassContext::transferAttachment(VkFormat format, PassUsage usage, VkSampleCountFlagBits sampleCount) {
	auto attachment = emptyAttachment(format, sampleCount);
	// This allows tilers to completely avoid writing out the multisampled attachment to memory,
	// a considerable performance and bandwidth improvement
	// attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // VK_ATTACHMENT_STORE_OP_STORE;
	modifyAttachmentByPassUsage(usage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, attachment);
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::nonTransferAttachment(VkFormat format, PassUsage usage, VkSampleCountFlagBits sampleCount) {
	auto attachment = emptyAttachment(format, sampleCount);
	modifyAttachmentByPassUsage(usage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, attachment);
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::depthAttachment(VkFormat format, PassUsage usage, VkSampleCountFlagBits sampleCount) {
	auto attachment = emptyAttachment(format, sampleCount);
	modifyAttachmentByPassUsage(usage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, attachment);
	
	// Make sure depth image be initialized when the first pass begin.
	if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
	if (attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	}

	attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::presentAttachment(VkFormat format, PassUsage usage) {
	auto attachment = emptyAttachment(format, VK_SAMPLE_COUNT_1_BIT);
	modifyAttachmentByPassUsage(usage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, attachment);
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::emptyAttachment(VkFormat format, VkSampleCountFlagBits sampleCount) {
	return VkAttachmentDescription{
		.format = format,
		.samples = sampleCount,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
}

VkSubpassDependency AfterglowSubpassContext::firstDependency(uint32_t destSubpassIndex) {
	return VkSubpassDependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = destSubpassIndex,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};
}

VkSubpassDependency AfterglowSubpassContext::fragmentRWDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex) {
	// Here src/dstStageMask means Before into dstSubpass fragment shader,  
	// dstSubpass must wait for srcSubpass color and depth attachment stage done.
	return VkSubpassDependency{
		.srcSubpass = srcSubpassIndex, 
		.dstSubpass = destSubpassIndex, 
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};
}

VkSubpassDependency AfterglowSubpassContext::fragmentRWColorRDepthDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex) {
	auto dependency = fragmentRWDependency(srcSubpassIndex, destSubpassIndex);
	dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	return dependency;
}

VkSubpassDependency AfterglowSubpassContext::fragmentRColorDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex) {
	return VkSubpassDependency{
		.srcSubpass = srcSubpassIndex,
		.dstSubpass = destSubpassIndex,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT 
	};
}

VkSubpassDependency AfterglowSubpassContext::fragmentWColorDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex) {
	return VkSubpassDependency{
		.srcSubpass = srcSubpassIndex, 
		.dstSubpass = destSubpassIndex, 
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // SrcSupass attachment {srcStageMask} is outputted, then
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , // DestSubpass {dstStageMask} attachment is able to write.
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,    // SrcSupass access {srcAccessMask} is completed, then
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT     // DestSubpass is able to access {dstAccessMask}
	};
}

VkSubpassDescription& AfterglowSubpassContext::appendSubpass(const std::string& subpassName) {
	auto iterator = _subpassAttachmentInfos.find(subpassName);
	// If subpass exists, return existed subpass directly. 
	if (iterator != _subpassAttachmentInfos.end()) {
		return _subpasses[iterator->second.subpassIndex];
	}
	_subpassAttachmentInfos.emplace(subpassName, SubpassAttachmentInfo{ .subpassIndex = subpassCount() });

	auto& subpass = _subpasses.emplace_back();
	// Default Settings
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	return subpass;
}

uint32_t AfterglowSubpassContext::subpassIndex(const std::string& subpassName) {
	auto* info = subpassAttachmentInfo(subpassName);
	if (!info) {
		EXCEPT_CLASS_RUNTIME("Subpass not found.");
	}
	return info->subpassIndex;
}

bool AfterglowSubpassContext::bindInputAttachment(const std::string& subpassName, uint32_t attachmentIndex, const std::string& attachmentName, VkImageLayout imageLayout) {
	if (!bindAttachment(AttachmentReferenceArrayID::Input, subpassName, attachmentIndex, imageLayout)) {
		return false;
	}

	auto inputAttachmentType = render::AttachmentType::Color;
	bool isMultiSample = false;
	// TODO: DepthStencil support.
	if (isDepthAttachmentIndex(attachmentIndex)) {
		inputAttachmentType = render::AttachmentType::Depth;
	}
	if (attachment(attachmentIndex).samples != VK_SAMPLE_COUNT_1_BIT) {
		isMultiSample = true;
	}

	_inputAttachmentInfos.emplace_back(
		attachmentIndex, inputAttachmentType, isMultiSample, attachmentName
	);

	return true;
}

bool AfterglowSubpassContext::bindInputDepthAttachment(const std::string& subpassName, uint32_t attachmentIndex, const std::string& attachmentName) {
	return bindInputAttachment(subpassName, attachmentIndex, attachmentName, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
}

bool AfterglowSubpassContext::bindColorAttachment(const std::string& subpassName, uint32_t attachmentIndex) {
	bool bindSuccessfully = bindAttachment(
		AttachmentReferenceArrayID::Color,
		subpassName,
		attachmentIndex,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);
	return bindSuccessfully;
}

bool AfterglowSubpassContext::bindResolveAttachment(const std::string& subpassName, uint32_t attachmentIndex) {
	if (isDepthAttachmentIndex(attachmentIndex)) {
		DEBUG_CLASS_WARNING("Can not resolve a depth attachment.");
		return false;
	}
	bool bindSuccessfully = bindAttachment(
		AttachmentReferenceArrayID::Resolve,
		subpassName,
		attachmentIndex,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);
	return bindSuccessfully;
}

bool AfterglowSubpassContext::bindDepthAttachment(const std::string& subpassName, uint32_t attachmentIndex) {
	auto* info = subpassAttachmentInfo(subpassName);
	if (!isDepthAttachmentIndex(attachmentIndex) || !info) {
		DEBUG_CLASS_WARNING("Depth attachment or subpass was not initialized.");
		return false;
	}
	info->depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	info->depthAttachmentRef.attachment = attachmentIndex;
	return true;
}

uint32_t AfterglowSubpassContext::appendAttachment() {
	_attachments.emplace_back();
	_clearValues.emplace_back(VkClearValue{{0.0f, 0.0f, 0.0f, 0.0f}});
	return static_cast<uint32_t>(_attachments.size() - 1);
}

uint32_t AfterglowSubpassContext::appendAttachment(VkAttachmentDescription&& attachment) {
	_attachments.emplace_back(std::forward<VkAttachmentDescription>(attachment));
	_clearValues.emplace_back(VkClearValue{ {0.0f, 0.0f, 0.0f, 0.0f} });
	return static_cast<uint32_t>(_attachments.size() - 1);
}

uint32_t AfterglowSubpassContext::appendDepthAttachment(VkAttachmentDescription&& attachment) {
	uint32_t index = appendAttachment(std::forward<VkAttachmentDescription>(attachment));
	_depthAttachmentIndices.insert(index);
	// If reverseDpeth, clear depth to 0.0f, otherwise clear to 1.0f.
	_clearValues[index] = VkClearValue{ {cfg::reverseDepth ? 0.0f : 1.0f, 0} };
	
	return index;
}

int32_t AfterglowSubpassContext::isDepthAttachmentIndex(uint32_t index) const {
	return _depthAttachmentIndices.find(index) != _depthAttachmentIndices.end();
}

VkSubpassDependency& AfterglowSubpassContext::makeDependency(const std::string& srcSubpass, const std::string& destSubpass, DependencyPresetFunc dependencyPreset) {
	if (destSubpass.empty()) {
		EXCEPT_CLASS_RUNTIME("Failed to make dependency, due to dest subpass is undefined.");
	}

	SubpassAttachmentInfo* srcSubpassInfo = nullptr;
	if (!srcSubpass.empty()) {
		srcSubpassInfo = subpassAttachmentInfo(srcSubpass);
	}

	SubpassAttachmentInfo* destSubpassInfo = subpassAttachmentInfo(destSubpass);

	bool srcSubpassValid = srcSubpass.empty() || srcSubpassInfo;

	if (!srcSubpassValid || !destSubpassInfo) {
		EXCEPT_CLASS_RUNTIME("Failed to make dependency, due to srcSubpass or destSubpass is not exists.");
	}

	auto& dependency = _dependencies.emplace_back();
	if (!srcSubpassInfo) {
		dependency = firstDependency(destSubpassInfo->subpassIndex);
	}
	else if (dependencyPreset) {
		dependency = dependencyPreset(srcSubpassInfo->subpassIndex, destSubpassInfo->subpassIndex);
	}
	else {
		dependency.srcSubpass = srcSubpassInfo->subpassIndex;
		dependency.dstSubpass = destSubpassInfo->subpassIndex;
	}
	return dependency;
}

VkAttachmentDescription& AfterglowSubpassContext::attachment(uint32_t index) {
	return _attachments[index];
}

void AfterglowSubpassContext::setClearValue(uint32_t index, const VkClearValue& clearValue) {
	_clearValues[index] = clearValue;
}

const AfterglowSubpassContext::SubpassDescriptionArray& AfterglowSubpassContext::subpasses(bool updateAttachmentRefs){
	if (updateAttachmentRefs) {
		updateSubpassAttachmentReferences();
	}
	return _subpasses;
}

const AfterglowSubpassContext::SubpassDependencyArray& AfterglowSubpassContext::dependencies() const noexcept {
	return _dependencies;
}

uint32_t AfterglowSubpassContext::attachmentCount() const {
	return static_cast<uint32_t>(_attachments.size());
}

uint32_t AfterglowSubpassContext::subpassCount() const {
	return static_cast<uint32_t>(_subpasses.size());
}

uint32_t AfterglowSubpassContext::dependencyCount() const {
	return static_cast<uint32_t>(_dependencies.size());
}

uint32_t AfterglowSubpassContext::clearValueCount() const {
	return _clearValues.size();
}

bool AfterglowSubpassContext::subpassExists(const std::string& subpassName) const {
	return _subpassAttachmentInfos.find(subpassName) != _subpassAttachmentInfos.end();
}

const render::InputAttachmentInfos& AfterglowSubpassContext::inputAttachmentInfos() const {
	return _inputAttachmentInfos;
}

VkSampleCountFlagBits AfterglowSubpassContext::rasterizationSampleCount(const std::string& subpassName) const {
	auto* info = subpassAttachmentInfo(subpassName);
	if (!info) {
		DEBUG_CLASS_WARNING("Domain attachment info was not found, return a default value.");
		return VK_SAMPLE_COUNT_1_BIT;
	}
	auto& colorAttachment = info->attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Color)];
	if (!colorAttachment.empty()) {
		return _attachments[colorAttachment[0].attachment].samples;
	}
		
	DEBUG_CLASS_WARNING("Color attachment reference was not found, return a default value.");
	return VK_SAMPLE_COUNT_1_BIT;
}

const std::string& AfterglowSubpassContext::firstSubpassName() const {
	for (const auto& [name, info] : _subpassAttachmentInfos) {
		if (info.subpassIndex == 0) {
			return name;
		}
	}
	EXCEPT_CLASS_RUNTIME("No valid subpass be found.");
}

inline void AfterglowSubpassContext::modifyAttachmentByPassUsage(
	PassUsage usage,
	VkImageLayout importAttachmentLayout,
	VkAttachmentDescription& destAttachment
) {
	destAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	destAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	destAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	destAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	if (util::EnumValue(usage) & util::EnumValue(PassUsage::Import)) {
		destAttachment.initialLayout = importAttachmentLayout;
		destAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		destAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	if (util::EnumValue(usage) & util::EnumValue(PassUsage::Export)) {
		destAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		destAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	}
}

const AfterglowSubpassContext::AttachmentDescriptionArray& AfterglowSubpassContext::attachments() const noexcept {
	return _attachments;
}

const AfterglowSubpassContext::ClearValueArray& AfterglowSubpassContext::clearValues() const noexcept {
	return _clearValues;
}

inline AfterglowSubpassContext::SubpassAttachmentInfo* AfterglowSubpassContext::subpassAttachmentInfo(const std::string& subpassName) {
	auto iterator = _subpassAttachmentInfos.find(subpassName);
	if (iterator == _subpassAttachmentInfos.end()) {
		DEBUG_CLASS_WARNING("Custom subpass not found, try to appendSubpass() before append attachemnt.");
		return nullptr;
	}
	return &iterator->second;
}

inline AfterglowSubpassContext::SubpassAttachmentInfo* AfterglowSubpassContext::subpassAttachmentInfo(const std::string& subpassName) const {
	return const_cast<AfterglowSubpassContext*>(this)->subpassAttachmentInfo(subpassName);
}

inline bool AfterglowSubpassContext::bindAttachment(AttachmentReferenceArrayID referenceID, const std::string& subpassName, uint32_t attachmentIndex, VkImageLayout layout) {
	auto* info = subpassAttachmentInfo(subpassName);
	if (!info || attachmentIndex >= _attachments.size() || attachmentIndex < 0) {
		DEBUG_CLASS_WARNING("Subpass have not been created, or attachment index out of range.");
		return false;
	}
	auto& attachmentRef = info->attachmentRefs[util::EnumValue(referenceID)].emplace_back();
	attachmentRef.attachment = attachmentIndex;
	attachmentRef.layout = layout;
	return true;
}

inline void AfterglowSubpassContext::updateSubpassAttachmentReferences() {
	for (auto& [key, attachmentInfo] : _subpassAttachmentInfos) {
		auto& subpass = _subpasses[attachmentInfo.subpassIndex];
		auto& inputAttachmentRefs =
			attachmentInfo.attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Input)];
		auto& colorAttachmentRefs =
			attachmentInfo.attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Color)];
		auto& resolveAttachmentRefs =
			attachmentInfo.attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Resolve)];
		if (inputAttachmentRefs.size() > 0) {
			subpass.pInputAttachments = inputAttachmentRefs.data();
			subpass.inputAttachmentCount = static_cast<uint32_t>(inputAttachmentRefs.size());
		}
		if (colorAttachmentRefs.size() > 0) {
			subpass.pColorAttachments = colorAttachmentRefs.data();
			subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
		}
		if (resolveAttachmentRefs.size() > 0) {
			subpass.pResolveAttachments = resolveAttachmentRefs.data();
		}
		if (attachmentInfo.depthAttachmentRef.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
			subpass.pDepthStencilAttachment = &attachmentInfo.depthAttachmentRef;
		}
	}
}