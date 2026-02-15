#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include <set>
#include <unordered_map>

#include "RenderDefinitions.h"

class AfterglowSubpassContext {
public:
	enum class AttachmentReferenceArrayID : uint32_t {
		Input, 
		Color, 
		Resolve
	};

	// @note: term Import/Export were used between RenderPasses, and Input/Output were used between Subpasses.
	enum class PassUsage {
		Local = 0, 
		Import = 1 << 0, 
		Export = 1 << 1,
		ImportExport = Import | Export
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

	// @note: It seems is a hack.
	inline static VkImageLayout depthAttachmentRWLayout() { return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL; }

	// Attachment presets
	static VkAttachmentDescription transferAttachment(VkFormat format, PassUsage usage = PassUsage::Local, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	static VkAttachmentDescription nonTransferAttachment(VkFormat format, PassUsage usage = PassUsage::Local, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	static VkAttachmentDescription depthAttachment(VkFormat format, PassUsage usage = PassUsage::Local, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	static VkAttachmentDescription presentAttachment(VkFormat format, PassUsage usage = PassUsage::Export);
	static VkAttachmentDescription emptyAttachment(VkFormat format, VkSampleCountFlagBits sampleCount);

	// Dependency presets
	using DependencyPresetFunc = VkSubpassDependency(*)(uint32_t, uint32_t);
	static VkSubpassDependency firstDependency(uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentRWDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentRWColorRDepthDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentRColorDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);
	static VkSubpassDependency fragmentWColorDependency(uint32_t srcSubpassIndex, uint32_t destSubpassIndex);

	static const std::string& undefinedSubpassName() noexcept { static std::string name; return name; }

	// @return: If subpass exists, return target subpass description, otherwise append a new subpass and return its description.
	VkSubpassDescription& appendSubpass(const std::string& subpassName);

	uint32_t subpassIndex(const std::string& subpassName);
	
	// @return: Bind input attachment successfully.
	bool bindInputAttachment(const std::string& subpassName, uint32_t attachmentIndex, const std::string& attachmentName, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	bool bindInputDepthAttachment(const std::string& subpassName, uint32_t attachmentIndex, const std::string& attachmentName);
	bool bindColorAttachment(const std::string& subpassName, uint32_t attachmentIndex);
	bool bindResolveAttachment(const std::string& subpassName, uint32_t attachmentIndex);
	bool bindDepthAttachment(const std::string& subpassName, uint32_t attachmentIndex);

	// @return: Index of new attachment.
	uint32_t appendAttachment();
	uint32_t appendAttachment(VkAttachmentDescription&& attachment);

	// @brief: init or get depth attachment.
	// @return: Index of depth attachment.
	uint32_t appendDepthAttachment(VkAttachmentDescription&& attachment);
	// @return: If return value == -1, means depth attachment was not initialized.
	int32_t isDepthAttachmentIndex(uint32_t index) const;
	
	/**
	* @param srcSubpass: Where the last subpass from, if have not source, assign renderPass::Domain::Undefined.
	* @param destSubpass: Where the next subpass to.
	*/
	VkSubpassDependency& makeDependency(const std::string& srcSubpass, const std::string& destSubpass, DependencyPresetFunc dependencyPreset = nullptr);

	// @return: attachment reference by index.
	VkAttachmentDescription& attachment(uint32_t index);
	void setClearValue(uint32_t index, const VkClearValue& clearValue);

	// @brief: aquire subpass descriptions, it will also updaate subpass attachment references.
	const SubpassDescriptionArray& subpasses(bool updateAttachmentRefs = true);
	const SubpassDependencyArray& dependencies() const noexcept;
	const AttachmentDescriptionArray& attachments() const noexcept;
	const ClearValueArray& clearValues() const noexcept;

	// TODO: Thread-safety
	uint32_t attachmentCount() const;
	uint32_t subpassCount() const;
	uint32_t dependencyCount() const;
	uint32_t clearValueCount() const;

	bool subpassExists(const std::string& subpassName) const;

	const render::InputAttachmentInfos& inputAttachmentInfos() const;
	VkSampleCountFlagBits rasterizationSampleCount(const std::string& subpassName) const;

	const std::string& firstSubpassName() const;

private:
	static inline void modifyAttachmentByPassUsage(
		PassUsage usage, 
		VkImageLayout importAttachmentLayout, 
		VkAttachmentDescription& destAttachment
	);

	// @return: if success, return attachment info, otherwise throw an runtime error
	inline SubpassAttachmentInfo* subpassAttachmentInfo(const std::string& subpassName);
	inline SubpassAttachmentInfo* subpassAttachmentInfo(const std::string& subpassName) const;

	inline bool bindAttachment(
		AttachmentReferenceArrayID referenceID, const std::string& subpassName, uint32_t attachmentIndex, VkImageLayout layout
	);

	inline void updateSubpassAttachmentReferences();

	// Material Subpasses
	std::unordered_map<std::string, SubpassAttachmentInfo> _subpassAttachmentInfos;

	// Subpass data for submission
	SubpassDescriptionArray _subpasses;
	SubpassDependencyArray _dependencies;

	// Attachment data for submission
	AttachmentDescriptionArray _attachments;
	ClearValueArray _clearValues;

	// Attachment additional infos
	std::set<int32_t> _depthAttachmentIndices;
	render::InputAttachmentInfos _inputAttachmentInfos;
};

