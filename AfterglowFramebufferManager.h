#pragma once
#include "AfterglowFramebuffer.h"
#include "AfterglowRenderPass.h"
#include "AfterglowColorImage.h"
#include "AfterglowDepthImage.h"
class AfterglowFramebufferManager :  public AfterglowObject {
public:
	struct AcquireState{
		enum {
			Invalid = -1
		};
	};

	using ImageArray = std::vector<std::unique_ptr<AfterglowObject>>;

	AfterglowFramebufferManager(AfterglowRenderPass& renderPass);

	inline AfterglowDevice& device() noexcept;
	inline AfterglowSwapchain& swapchain() noexcept;
	inline AfterglowRenderPass& renderPass() noexcept;

	// If window size is changed, call this function.
	void recreate();
	// If returns -1, means failed to acquire image index, should interrupt this draw.
	int acquireNextImage(AfterglowSynchronizer& synchronizer);

	AfterglowFramebuffer& framebuffer(uint32_t index);
	img::WriteInfoArray& imageWriteInfos();

private:
	void initFramebuffers();

	AfterglowFramebuffer::Array _framebuffers;
	AfterglowRenderPass& _renderPass;

	ImageArray _images;

	img::WriteInfoArray _imageWriteInfos;
};

