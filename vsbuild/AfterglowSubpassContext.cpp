#include "AfterglowSubpassContext.h"

#include <stdexcept>

#include "AfterglowUtilities.h"
#include "RenderConfigurations.h"
#include "DebugUtilities.h"

VkAttachmentDescription AfterglowSubpassContext::transferAttachment(VkFormat format, VkSampleCountFlagBits sampleCount) {
	auto attachment = emptyAttachment(format, sampleCount);
	// This allows tilers to completely avoid writing out the multisampled attachment to memory,
	// a considerable performance and bandwidth improvement
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // VK_ATTACHMENT_STORE_OP_STORE;
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::nonTransferAttachment(VkFormat format, VkSampleCountFlagBits sampleCount) {
	auto attachment = emptyAttachment(format, sampleCount);
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::depthAttachment(VkFormat format, VkSampleCountFlagBits sampleCount) {
	auto attachment = emptyAttachment(format, sampleCount);
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //VK_ATTACHMENT_STORE_OP_STORE;
	attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return attachment;
}

VkAttachmentDescription AfterglowSubpassContext::presentAttachment(VkFormat format) {
	auto attachment = emptyAttachment(format, VK_SAMPLE_COUNT_1_BIT);
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

VkSubpassDependency AfterglowSubpassContext::fragmentWColorDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex) {
	return VkSubpassDependency{
		.srcSubpass = srcSubpassIndex, 
		.dstSubpass = destSubpassIndex, 
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // SrcSupass attachment {srcStageMask} is outputted, then
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // DestSubpass {dstStageMask} attachment is able to write.
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,    // SrcSupass access {srcAccessMask} is completed, then
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT     // DestSubpass is able to access {dstAccessMask}
	};
}

VkSubpassDescription& AfterglowSubpassContext::appendSubpass(render::Domain domain) {
	auto iterator = _attachmentInfos.find(domain);
	// If subpasss exists: 
	if (iterator != _attachmentInfos.end()) {
		auto& attachmentInfo = iterator->second;
		return _subpasses[attachmentInfo.subpassIndex];
	}
	// Subpass not exists:
	// Initialize attachment info
	// Initialize subpass description

	// Force subpass in order of Domain.
	if (util::EnumValue(domain) <= util::EnumValue(_lastAppendedDomain)) {
		DEBUG_CLASS_FATAL("Make sure append subpass in domain order.");
		throw std::runtime_error("Make sure append subpass in domain order.");
	}
	_lastAppendedDomain = domain;

	auto& subpass = _subpasses.emplace_back();
	_attachmentInfos.emplace(domain, SubpassAttachmentInfo{ .subpassIndex = subpassCount() - 1});

	// Default Settings
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	return subpass;
}

uint32_t AfterglowSubpassContext::subpassIndex(render::Domain domain) {
	auto* info = attachmentInfo(domain);
	if (!info) {
		throw std::runtime_error("[AfterglowSubpassContext] Can not aquire subpass, due to subpass of this domain have not been created.");
	}
	return info->subpassIndex;
}

bool AfterglowSubpassContext::bindInputAttachment(render::Domain domain, uint32_t attachmentIndex, const std::string& attachmentName, VkImageLayout imageLayout) {
	if (!bindAttachment(AttachmentReferenceArrayID::Input, domain, attachmentIndex, imageLayout)) {
		return false;
	}

	auto inputAttachmentType = render::InputAttachmentType::Color;
	bool isMultiSample = false;
	if (isDepthAttachmentIndex(attachmentIndex)) {
		inputAttachmentType = render::InputAttachmentType::Depth;
	}
	if (attachment(attachmentIndex).samples != VK_SAMPLE_COUNT_1_BIT) {
		isMultiSample = true;
	}
	_inputAttachmentInfos.emplace_back(
		domain, attachmentIndex, inputAttachmentType, isMultiSample, attachmentName
	);
	return true;
}

bool AfterglowSubpassContext::bindInputDepthAttachment(render::Domain domain, uint32_t attachmentIndex, const std::string& attachmentName) {
	return bindInputAttachment(domain, attachmentIndex, attachmentName, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
}

bool AfterglowSubpassContext::bindColorAttachment(render::Domain domain, uint32_t attachmentIndex) {
	bool bindSuccessfully = bindAttachment(
		AttachmentReferenceArrayID::Color,
		domain,
		attachmentIndex,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);
	return bindSuccessfully;
}

bool AfterglowSubpassContext::bindResolveAttachment(render::Domain domain, uint32_t attachmentIndex) {
	if (isDepthAttachmentIndex(attachmentIndex)) {
		DEBUG_CLASS_WARNING("Can not resolve a depth attachment.");
		return false;
	}
	bool bindSuccessfully = bindAttachment(
		AttachmentReferenceArrayID::Resolve, 
		domain, 
		attachmentIndex, 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);
	return bindSuccessfully;
}

bool AfterglowSubpassContext::bindDepthAttachment(render::Domain domain, uint32_t attachmentIndex) {
	auto* info = attachmentInfo(domain);
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

int32_t AfterglowSubpassContext::isDepthAttachmentIndex(uint32_t index) {
	return _depthAttachmentIndices.find(index) != _depthAttachmentIndices.end();
}

VkSubpassDependency& AfterglowSubpassContext::makeDependency(render::Domain srcSubpass, render::Domain destSubpass, DependencyPresetFunc dependencyPreset) {
	auto srcSubpassInfoInterator = _attachmentInfos.find(srcSubpass);
	auto destSubpassInfoIterator = _attachmentInfos.find(destSubpass);
	if ((srcSubpass != render::Domain::Undefined && srcSubpassInfoInterator == _attachmentInfos.end())
		|| (destSubpassInfoIterator == _attachmentInfos.end())) {
		throw std::runtime_error("[AfterglowSubpassContext] Failed to make dependency, due to srcSubpass or destSubpass is not exists.");
	}
	if (destSubpass == render::Domain::Undefined) {
		throw std::runtime_error("[AfterglowSubpassContext] Failed to make dependency, due to dest subpass is undefined.");
	}

	auto& dependency = _dependencies.emplace_back();
	auto& destSubpassInfo = destSubpassInfoIterator->second;
	if (srcSubpass == render::Domain::Undefined) {
		dependency = firstDependency(destSubpassInfo.subpassIndex);
	}
	else if (dependencyPreset) {
		dependency = dependencyPreset(srcSubpassInfoInterator->second.subpassIndex, destSubpassInfo.subpassIndex);
	}
	else {
		dependency.srcSubpass = srcSubpassInfoInterator->second.subpassIndex;
		dependency.dstSubpass = destSubpassInfo.subpassIndex;
	}
	return dependency;
}

VkAttachmentDescription& AfterglowSubpassContext::attachment(uint32_t index) {
	return _attachments[index];
}

VkClearValue& AfterglowSubpassContext::clearValue(uint32_t index) {
	return _clearValues[index];
}

const AfterglowSubpassContext::SubpassDescriptionArray& AfterglowSubpassContext::subpasses(bool updateAttachmentRefs){
	if (updateAttachmentRefs) {
		updateSubpassAttachmentReferences();
	}
	return _subpasses;
}

const AfterglowSubpassContext::SubpassDependencyArray& AfterglowSubpassContext::dependencies() {
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

bool AfterglowSubpassContext::subpassExists(render::Domain domain) const {
	return _attachmentInfos.find(domain) != _attachmentInfos.end();
}

const render::InputAttachmentInfos& AfterglowSubpassContext::inputAttachmentInfos() const {
	return _inputAttachmentInfos;
}

VkSampleCountFlagBits AfterglowSubpassContext::rasterizationSampleCount(render::Domain domain) const {
	auto* info = attachmentInfo(domain);
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

const AfterglowSubpassContext::AttachmentDescriptionArray& AfterglowSubpassContext::attachments() {
	return _attachments;
}

const AfterglowSubpassContext::ClearValueArray& AfterglowSubpassContext::clearValues() {
	return _clearValues;
}

inline AfterglowSubpassContext::SubpassAttachmentInfo* AfterglowSubpassContext::attachmentInfo(render::Domain domain) {
	auto iterator = _attachmentInfos.find(domain);
	if (iterator == _attachmentInfos.end()) {
		DEBUG_CLASS_WARNING("Subpass of target domain not found, try to appendSubpass() before append attachemnt.");
		return nullptr;
	}
	return &iterator->second;
}

inline const AfterglowSubpassContext::SubpassAttachmentInfo* AfterglowSubpassContext::attachmentInfo(render::Domain domain) const {
	return const_cast<AfterglowSubpassContext*>(this)->attachmentInfo(domain);
}

inline bool AfterglowSubpassContext::bindAttachment(
	AttachmentReferenceArrayID referenceID, 
	render::Domain domain, 
	uint32_t attachmentIndex, 
	VkImageLayout layout
	) {
	auto* info = attachmentInfo(domain);
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
	for (auto& [domain, attachmentInfo] : _attachmentInfos) {
		auto& subpass = _subpasses[attachmentInfo.subpassIndex];
		auto& inputAttachmentRefs =
			attachmentInfo.attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Input)];
		auto& colorAttachmentRefs =
			attachmentInfo.attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Color)];
		auto& resolveAttachmentRefs =
			attachmentInfo.attachmentRefs[util::EnumValue(AttachmentReferenceArrayID::Resolve)];
		if (inputAttachmentRefs.size() > 0) {
			subpass.pInputAttachments = inputAttachmentRefs.data();
			subpass.inputAttachmentCount = inputAttachmentRefs.size();
		}
		if (colorAttachmentRefs.size() > 0) {
			subpass.pColorAttachments = colorAttachmentRefs.data();
			subpass.colorAttachmentCount = colorAttachmentRefs.size();
		}
		if (resolveAttachmentRefs.size() > 0) {
			subpass.pResolveAttachments = resolveAttachmentRefs.data();
		}
		if (attachmentInfo.depthAttachmentRef.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
			subpass.pDepthStencilAttachment = &attachmentInfo.depthAttachmentRef;
		}
	}
}