#include "AfterglowFramebufferManager.h"

AfterglowFramebufferManager::AfterglowFramebufferManager(AfterglowRenderPass& renderPass) :
	_renderPass(renderPass) {
	initFramebuffers();
}

inline AfterglowDevice& AfterglowFramebufferManager::device() {
	return _renderPass.device();
}

inline AfterglowSwapchain& AfterglowFramebufferManager::swapchain() {
	return _renderPass.swapchain();
}

inline AfterglowRenderPass& AfterglowFramebufferManager::renderPass() {
	return _renderPass;
};

void AfterglowFramebufferManager::recreate() {
	// Make sure resource is not be used.
	swapchain().device().waitIdle();
	swapchain().recreate();
	// When the array call clear(), AfterglowFramebuffer would destory automatically, so never destory manully.
	initFramebuffers();
}

int AfterglowFramebufferManager::acquireNextImage(AfterglowSynchronizer& synchronizer) {
	// Obstruct automatically and reset fence if acquire imageIndex successfully.
	//synchronizer.wait();
	uint32_t imageIndex;
	VkResult state = vkAcquireNextImageKHR(
		_renderPass.device(), 
		_renderPass.swapchain(), 
		UINT64_MAX, 
		synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::ImageAvaliable), 
		VK_NULL_HANDLE, 
		&imageIndex
	);
	if (state == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate();
		return AcquireState::Invalid;
	}
	else if (state != VK_SUCCESS && state != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("[AfterglowFramebufferManager] Failed to create acquire swapchain image.");
	}
	return imageIndex;
}

AfterglowFramebuffer& AfterglowFramebufferManager::framebuffer(uint32_t index) {
	return _framebuffers[index];
}

img::WriteInfoArray& AfterglowFramebufferManager::imageWriteInfos() {
	return _imageWriteInfos;
}

void AfterglowFramebufferManager::initFramebuffers() {
	// Use recreate to create. Beacuse we will create them again when the window size was changed.
	auto& subpassContext = _renderPass.subpassContext();
	
	_images.clear();
	_imageWriteInfos.clear();

	uint32_t presentIndex = 0;
	for (int32_t attachmentIndex = 0; attachmentIndex < subpassContext.attachmentCount(); ++attachmentIndex) {
		// TODO: Sync subpass attachment info into images.
		auto& attachment = subpassContext.attachment(attachmentIndex);
		// DepthImage
		if (subpassContext.isDepthAttachmentIndex(attachmentIndex)) {
			auto usage = AfterglowDepthImage::defaultUsage();
			if (attachment.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				usage = AfterglowDepthImage::inputAttachmentUsage();
			}
			auto depthImage = 
				std::make_unique<AfterglowDepthImage>(device(), swapchain().extent(), attachment.samples, usage);
			_imageWriteInfos.emplace_back((*depthImage).sampler(), (*depthImage).imageView());
			_images.push_back(std::move(depthImage));
		}
		// ColorImage
		else {
			auto usage = AfterglowColorImage::defaultUsage();
			if (attachment.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				usage = AfterglowColorImage::inputAttachmentUsage();
			}
			else if (attachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
				presentIndex = attachmentIndex;
			}
			auto colorImage = std::make_unique<AfterglowColorImage>(
			device(), swapchain().extent(), attachment.format, attachment.samples, usage
			);	
			_imageWriteInfos.emplace_back((*colorImage).sampler(), (*colorImage).imageView());
			_images.push_back(std::move(colorImage));
		}
	}

	_framebuffers.clear();
	for (uint32_t swapchainImageIndex = 0; swapchainImageIndex < swapchain().imageViews().size(); ++swapchainImageIndex) {
		_framebuffers.push_back(AfterglowFramebuffer::makeElement(_renderPass));
		AfterglowFramebuffer& framebuffer = _framebuffers.back();
		// TODO: Decrease here extent could implement downsample bloom. (In specified attachment).
		framebuffer->width = swapchain().extent().width;
		framebuffer->height = swapchain().extent().height;
		framebuffer->layers = 1;

		// TODO: ...
		// [Order dependency] These attachments coresponding to renderPass's attachments
		for (int32_t attachmentIndex = 0; attachmentIndex < subpassContext.attachmentCount(); ++attachmentIndex) {
			if (subpassContext.isDepthAttachmentIndex(attachmentIndex)) {
				// Depth attachment
				framebuffer.appendImageView(dynamic_cast<AfterglowDepthImage*>(_images[attachmentIndex].get())->imageView());
			}
			else if (attachmentIndex == presentIndex) {
				// For write data to swapchain image, I found this issue for all day long.
				framebuffer.appendImageView(swapchain().imageView(swapchainImageIndex));
			}
			else {
				// Color attachment
				framebuffer.appendImageView(dynamic_cast<AfterglowColorImage*>(_images[attachmentIndex].get())->imageView());
			}
		}
	}
}
