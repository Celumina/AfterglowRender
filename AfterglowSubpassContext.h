#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include <set>
#include <unordered_map>
#include "RenderDefinitions.h"

class AfterglowSubpassContext {
public:
	enum class AttachmentReferenceArrayID : uint32_t {
		Input = 0, 
		Color = 1, 
		Resolve = 2
	};

	using AttachmentDescriptionArray = std::vector<VkAttachmentDescription>;
	using AttachmentReferenceArray = std::vector<VkAttachmentReference>;
	using ClearValueArray = std::vector<VkClearValue>;

	struct SubpassAttachmentInfo {
		// Input attachment, Color attachment, Resolve attachment.
		std::vector<AttachmentReferenceArray> attachmentRefs {{}, {}, {}};
		VkAttachmentReference depthAttachmentRef {0, VK_IMAGE_LAYOUT_UNDEFINED};
		uint32_t subpassIndex;
	};

	using SubpassDescriptionArray = std::vector<VkSubpassDescription>;
	using SubpassDependencyArray = std::vector<VkSubpassDependency>;

	// Attachment presets
	static VkAttachmentDescription transferAttachment(VkFormat format, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	static VkAttachmentDescription nonTransferAttachment(VkFormat format, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	static VkAttachmentDescription depthAttachment(VkFormat format, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	static VkAttachmentDescription presentAttachment(VkFormat format);
	static VkAttachmentDescription emptyAttachment(VkFormat format, VkSampleCountFlagBits sampleCount);

	// Dependency presets
	using DependencyPresetFunc = VkSubpassDependency(uint32_t, uint32_t);
	static VkSubpassDependency firstDependency(uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentRWDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentRWColorRDepthDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentWColorDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);

	// @return: If subpass exists, return target subpass description, otherwise append a new subpass and return its description.
	VkSubpassDescription& appendSubpass(render::Domain domain);

	uint32_t subpassIndex(render::Domain domain);
	
	// @return: Bind input attachment successfully.
	bool bindInputAttachment(render::Domain domain, uint32_t attachmentIndex, const std::string& attachmentName);
	bool bindColorAttachment(render::Domain domain, uint32_t attachmentIndex);
	bool bindResolveAttachment(render::Domain domain, uint32_t attachmentIndex);
	bool bindDepthAttachment(render::Domain domain, uint32_t attachmentIndex);

	// @return: Index of new attachment.
	uint32_t appendAttachment();
	uint32_t appendAttachment(VkAttachmentDescription&& attachment);

	// @brief: init or get depth attachment.
	// @return: Index of depth attachment.
	uint32_t appendDepthAttachment(VkAttachmentDescription&& attachment);
	// @return: If return value == -1, means depth attachment was not initialized.
	int32_t isDepthAttachmentIndex(uint32_t index);

	// @param srcSubpass: Where the last subpass from, if have not source, assign renderPass::Domain::Undefined.
	// @param destSubpass: Where the next subpass to.
	VkSubpassDependency& makeDependency(render::Domain srcSubpass, render::Domain destSubpass, DependencyPresetFunc dependencyPreset = nullptr);

	// @return: attachment reference by index.
	VkAttachmentDescription& attachment(uint32_t index);
	VkClearValue& clearValue(uint32_t index);

	// @brief: aquire subpass descriptions, it will also updaate subpass attachment references.
	const SubpassDescriptionArray& subpasses(bool updateAttachmentRefs = true);
	const SubpassDependencyArray& dependencies();
	const AttachmentDescriptionArray& attachments();
	const ClearValueArray& clearValues(); 

	uint32_t attachmentCount() const;
	uint32_t subpassCount() const;
	uint32_t dependencyCount() const;
	uint32_t clearValueCount() const;

	bool subpassExists(render::Domain domain) const;

	const render::InputAttachmentInfos& inputAttachmentInfos() const;
	VkSampleCountFlagBits rasterizationSampleCount(render::Domain domain) const;

private:
	// @return: if success, return attachment info, otherwise throw an runtime error
	inline SubpassAttachmentInfo* attachmentInfo(render::Domain domain);
	inline const SubpassAttachmentInfo* attachmentInfo(render::Domain domain) const;

	inline bool bindAttachment(
		AttachmentReferenceArrayID referenceID, 
		render::Domain domain, 
		uint32_t attachmentIndex, 
		VkImageLayout layout
	);

	inline void updateSubpassAttachmentReferences();
	
	std::unordered_map<render::Domain, SubpassAttachmentInfo> _attachmentInfos;
	SubpassDescriptionArray _subpasses;
	SubpassDependencyArray _dependencies;
	AttachmentDescriptionArray _attachments;
	ClearValueArray _clearValues;

	std::set<int32_t> _depthAttachmentIndices;
	render::Domain _lastAppendedDomain = render::Domain::Undefined;

	render::InputAttachmentInfos _inputAttachmentInfos;
};

