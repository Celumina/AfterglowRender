#pragma once
#include <glm/glm.hpp>
#include "AfterglowRenderPass.h"
#include "AfterglowUtilities.h"
#include "Configurations.h"

class AfterglowPassInterface;
class AfterglowFramebuffer;

namespace render {
	template<typename Type>
	using PassUnorderedMap = std::unordered_map<AfterglowPassInterface*, Type>;
}

class AfterglowPassInterface : public AfterglowObject {
public:
	enum class ColorAttachment {
		ColorMS, 
		Color, 
		GBufferA, // Unused
		GBufferB, // Unused
		GBufferC, // Unused
		GBufferD, // Unused
		GBufferE, // Unused
		GBufferF, // Unused

		EnumCount, 
		Unused
	};

	enum class ExtentMode {
		Fixed,
		Swapchain
	};

	struct ExtentScale {
		union {
			glm::vec2 scale;
			glm::u32vec2 extent;
		};
	};

	// @note: term Import/Export were used between RenderPasses, and Input/Output were used between Subpasses.
	struct ImportAttachment {
		std::string srcPassName;
		std::string attachmentName;
		uint32_t srcAttachmentIndex;
		uint32_t destAttachmentIndex;
		bool isMultipleSample;
	};
	using ImportAttachments = std::vector<ImportAttachment>;
	using ImageArray = std::vector<VkImage>;
	using BarrierArray = std::vector<VkImageMemoryBarrier>;
	using AttachmentIndexArray = std::array<int32_t, util::EnumValue(ColorAttachment::EnumCount)>;
	using AttachmentNameArray = std::array<std::unique_ptr<std::string>, util::EnumValue(ColorAttachment::EnumCount)>;
	using BindingFramebufferArray = std::vector<AfterglowFramebuffer*>;

	AfterglowPassInterface(AfterglowDevice& device, ExtentMode extentMode, VkFormat colorFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32);

	constexpr static inline int32_t invalidAttachmentIndex() noexcept { return -1; }
	constexpr static inline bool isValidAttachment(int32_t attachmentIndex) noexcept { return attachmentIndex != invalidAttachmentIndex(); }

	// Implicit convertion.
	operator AfterglowRenderPass& () noexcept {return _renderPass; }
	operator AfterglowRenderPass* () noexcept { return &_renderPass; }

	inline AfterglowRenderPass& renderPass() noexcept { return _renderPass; }
	inline const AfterglowRenderPass& renderPass() const noexcept { return _renderPass; }

	inline AfterglowDevice& device() noexcept { return _renderPass.device(); }

	inline AfterglowSubpassContext& subpassContext() noexcept { return renderPass().subpassContext(); }
	inline const AfterglowSubpassContext& subpassContext() const noexcept { return renderPass().subpassContext(); }

	inline ImportAttachments& importAttachments() noexcept { return _importAttachments; }
	inline const ImportAttachments& importAttachments() const noexcept { return _importAttachments; }

	/**
	* @desc: If ExtentMode == Fixed, renderpass use absolute resolution e.g. 1280.
	*	If ExtentMode == Swapchain, renderpass would use swapchain extent with scale.
	*/
	glm::u32vec2 extent() const;
	void setExtent(glm::u32vec2 extent);

	glm::vec2 scale() const;
	void setScale(glm::vec2 scale);

	inline ExtentMode extentMode() const noexcept { return _extentMode; }
	inline VkFormat colorFormat() const noexcept { return _colorFormat; }

	// Bind Framebuffer to avoid unordered map indexing.
	inline AfterglowFramebuffer& framebuffer(uint32_t imageIndex) { return *_framebuffers[imageIndex]; }
	inline void appendFramebuffer(AfterglowFramebuffer& framebuffer) { _framebuffers.push_back(&framebuffer); }
	inline BindingFramebufferArray& framebuffers() { return _framebuffers; }
	inline void clearFramebufferBindings() { _framebuffers.clear(); }

	/**
	* @brief: Find the current attachment is / isn't from a import attachment which is in a external render pass.
	* @return: If found, return import attachment info, otherwise return nullptr.
	*/
	ImportAttachment* findImportAttachment(uint32_t currentAttachment);

	// Export attachment indices.
	// @brief: variant color attachments support.
	inline int32_t exportColorAttachmentIndex(ColorAttachment attachment) const { return _exportColorAttachmentIndices[util::EnumValue(attachment)]; }
	inline const AttachmentIndexArray& exportColorAttachmentIndices() const noexcept { return _exportColorAttachmentIndices; }
	// @return: attachment index from current subpass context, -1 if attachment is not exists.
	inline int32_t exportDepthAttachmentIndex() const noexcept { return _exportDepthAttachmentIndex; }

	bool isExportColorAttachment(uint32_t inAttachmentIndex) const noexcept;

	inline int32_t presentAttachmentIndex() const noexcept { return _presentAttachmentIndex; }

	// Export attachment names.
	inline const std::string* exportColorAttachmentName(ColorAttachment attachment) const { return _exportColorAttachmentNames[util::EnumValue(attachment)].get(); }
	inline const AttachmentNameArray& exportColorAttachmentNames() const noexcept { return _exportColorAttachmentNames; }
	inline const std::string& exportDepthAttachmentName() const noexcept { return _exportDepthAttachmentName; }

	// Barriers
	VkPipelineStageFlags exportBarrierSrcPipelineStage() const noexcept { return _exportBarrierSrcPipelineStage; }
	//VkPipelineStageFlagBits exportBarrierDstPipelineStage() const noexcept { return VkPipelineStageFlagBits(_exportBarrierDstPipelineStage); }

	const BarrierArray* exportBarriers(); 
	// Multiple export color image support.
	const BarrierArray& recreateExportBarriers(const ImageArray* exportColorImages = nullptr, const VkImage exportDepthImage = nullptr);
	VkImageMemoryBarrier& appendColorBarrier(const VkImage exportColorImage, uint32_t exportAttachmentIndex);
	//VkImageMemoryBarrier& appendPresentBarrier(const VkImage exportColorImage, uint32_t exportAttachmentIndex);
	VkImageMemoryBarrier& appendDepthBarrier(const VkImage exportDepthImage);
	void clearBarriers();

	virtual std::string_view passName() const = 0;

	// @brief: Record [optional]importColorAttachment and [optional]importDepthAttachment.
	void recordImportAttachments(
		AfterglowPassInterface* srcPass,
		ColorAttachment importColorAttachmentIndex = ColorAttachment::Unused, 
		int32_t destColorAttachmentIndex = invalidAttachmentIndex(),
		int32_t destDepthAttachmentIndex = invalidAttachmentIndex()
	);

protected:
	void setExportDepthAttachment(uint32_t attachmentIndex, std::string_view name);
	void setExportColorAttachment(ColorAttachment attachment, uint32_t attachmentIndex, std::string_view name, bool isPresentAttachment = false);

private:
	AfterglowRenderPass _renderPass;
	ImportAttachments _importAttachments;
	ExtentScale _extentScale{};
	ExtentMode _extentMode;

	int32_t _exportDepthAttachmentIndex = invalidAttachmentIndex();
	AttachmentIndexArray _exportColorAttachmentIndices;

	std::string _exportDepthAttachmentName;
	AttachmentNameArray _exportColorAttachmentNames;

	/**
	* @note: For linear rendering before Tonemapping.
	* @desc:
	*	Candidates:
	*		VK_FORMAT_R8G8B8A8_SRGB			     <32bit/px>		SDR non-linear rendering directly, no Tonemapping.
	*		VK_FORMAT_B10G11R11_UFLOAT_PACK32	 <32bit/px>		LHDR linear rendering, were used in most Game Engines.
	*/
	VkFormat _colorFormat;

	int32_t _presentAttachmentIndex = invalidAttachmentIndex();

	VkPipelineStageFlags _exportBarrierSrcPipelineStage = VK_PIPELINE_STAGE_NONE;
	//uint32_t _exportBarrierDstPipelineStage = VK_PIPELINE_STAGE_NONE;
	std::unique_ptr<BarrierArray> _exportBarriers;

	BindingFramebufferArray _framebuffers;
};

