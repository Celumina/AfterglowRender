#pragma once
#include "AfterglowFramebuffer.h"
#include "AfterglowColorImage.h"
#include "AfterglowDepthImage.h"
#include "AfterglowPassInterface.h"

class AfterglowSwapchain;
class AfterglowSynchronizer;
class AfterglowPassManager;

class AfterglowFramebufferManager :  public AfterglowObject {
public:
	struct AcquireState{
		enum {
			Invalid = -1
		};
	};

	//struct AttachmentImage {
	//	std::unique_ptr<AfterglowObject> _image;
	//	AfterglowPassInterface::ImportAttachment* importAttachment = nullptr;
	//};
	// using AttachmentImages = std::vector<AttachmentImage>;

	AfterglowFramebufferManager(AfterglowPassManager& passManager, AfterglowSwapchain& swapchain);

	inline AfterglowDevice& device() noexcept;
	inline AfterglowSwapchain& swapchain() noexcept;

	// If window size is changed, call this function.
	void recreateSwapchain();
	// If returns -1, means failed to acquire image index, should interrupt this draw.
	int acquireNextImage(AfterglowSynchronizer& synchronizer);

	//AfterglowFramebuffer& onScreenFramebuffer(uint32_t index);
	render::PassUnorderedMap<img::ImageReferences>& imageReferences() noexcept;
	// @brief: if flag is true, return true once and then set the flag to false.
	bool takeSwapchainImageSetOutdatedFlag() noexcept;

	void recreateAllFramebuffers();
	void recreateSwapchainFramebuffers();

private:
	using PerPassImages = std::vector<std::unique_ptr<AfterglowObject>>;

	void recreatePassFramebuffers(AfterglowPassInterface& pass);
	inline VkExtent2D passExtent(AfterglowPassInterface & pass);

	// @brief: RenderPass which is the last fixedPass has swapchin extent and export color attaachment.
	//inline AfterglowRenderPass* findOnScreenRenderPass();

	AfterglowPassManager& _passManager;
	AfterglowSwapchain& _swapchain;

	// Seperate to offscreen and onscreen framebuffers. recreate onscreen attachment and framebuffer only.
	// Multi-framebuffers for swapchain.
	//AfterglowFramebuffer::Array _onScreenFramebuffers;

	// Single framebuffer for off-screen render passes.
	render::PassUnorderedMap<AfterglowFramebuffer::Array> _framebuffers;

	render::PassUnorderedMap<PerPassImages> _images;
	render::PassUnorderedMap<img::ImageReferences> _imageReferences;
	
	bool _swapchainImageSetOutdatedFlag = false;
};
