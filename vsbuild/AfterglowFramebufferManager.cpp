#include "AfterglowFramebufferManager.h"

#include "AfterglowPassManager.h"
#include "AfterglowSwapchain.h"
#include "AfterglowSynchronizer.h"
#include "ExceptionUtilities.h"

AfterglowFramebufferManager::AfterglowFramebufferManager(AfterglowPassManager& passManager, AfterglowSwapchain& swapchain) :
	_passManager(passManager), _swapchain(swapchain) {
	// @note: Invoke this function manually for late initialize.
}

inline AfterglowDevice& AfterglowFramebufferManager::device() noexcept {
	return _swapchain.device();
}

inline AfterglowSwapchain& AfterglowFramebufferManager::swapchain() noexcept {
	return _swapchain;
}

void AfterglowFramebufferManager::recreateSwapchain() {
	// Make sure resource is not be used.
	swapchain().device().waitIdle();
	swapchain().recreate();
	// When the array call clear(), AfterglowFramebuffer would destory automatically, so never destory manully.
	recreateSwapchainFramebuffers();
}

int AfterglowFramebufferManager::acquireNextImage(AfterglowSynchronizer& synchronizer) {
	// Obstruct automatically and reset fence if acquire imageIndex successfully.
	uint32_t imageIndex;
	VkResult state = vkAcquireNextImageKHR(
		device(), 
		swapchain(), 
		UINT64_MAX, 
		synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::ImageAvaliable), 
		VK_NULL_HANDLE, 
		&imageIndex
	);
	if (state == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return AcquireState::Invalid;
	}
	else if (state != VK_SUCCESS && state != VK_SUBOPTIMAL_KHR) {
		EXCEPT_CLASS_RUNTIME("Failed to create acquire swapchain image.");
	}
	return imageIndex;
}

//AfterglowFramebuffer& AfterglowFramebufferManager::onScreenFramebuffer(uint32_t index) {
//	return _onScreenFramebuffers[index];
//}

render::PassUnorderedMap<img::ImageReferences>& AfterglowFramebufferManager::imageReferences() noexcept {
	return _imageReferences;
}

bool AfterglowFramebufferManager::takeSwapchainImageSetOutdatedFlag() noexcept {
	if (_swapchainImageSetOutdatedFlag) {
		_swapchainImageSetOutdatedFlag = false;
		return true;
	}
	return false;
}

void AfterglowFramebufferManager::recreateAllFramebuffers() {
	//_images.clear();
	//_imageReferences.clear();

	_passManager.forEachPass([this](AfterglowPassInterface& pass) {
		recreatePassFramebuffers(pass);
	});
	_swapchainImageSetOutdatedFlag = true;
}

void AfterglowFramebufferManager::recreateSwapchainFramebuffers() {
	// TODO: Try scale present only and late to recreate others when the window size is stable.
	_passManager.forEachPass([this](AfterglowPassInterface& pass){
		if (pass.extentMode() == AfterglowPassInterface::ExtentMode::Swapchain) {
			recreatePassFramebuffers(pass);
		}
	});
	_swapchainImageSetOutdatedFlag = true;
}

void AfterglowFramebufferManager::recreatePassFramebuffers(AfterglowPassInterface& pass) {
	auto& passImages = _images[&pass];
	auto& passImageRefs = _imageReferences[&pass];
	passImages.clear();
	passImageRefs.clear();

	// Recreate attachment images
	auto& subpassContext = pass.subpassContext();
	//int32_t presentIndex = AfterglowPassInterface::invalidAttachmentIndex();
	VkExtent2D extent = passExtent(pass);
	for (uint32_t attachmentIndex = 0; attachmentIndex < subpassContext.attachmentCount(); ++attachmentIndex) {
		auto& attachment = subpassContext.attachment(attachmentIndex);
		auto* importAttachment = pass.findImportAttachment(attachmentIndex);
		// Image from other renderpass.
		if (importAttachment) {
			// @note: Here import image ref was dependent on the renderpass, make sure recreate renderPasses in order.
			auto* srcPass = _passManager.findPass(importAttachment->srcPassName);
			if (!srcPass) {
				EXCEPT_CLASS_RUNTIME("Import attachment source pass was not found.");
			}
			auto& srcImageRef = _imageReferences.at(srcPass)[importAttachment->srcAttachmentIndex];
			passImageRefs.push_back(srcImageRef);
			// Image referenced from another pass, so don't create a real image buffer.
			passImages.emplace_back(nullptr);
		}
		// DepthImage
		else if (subpassContext.isDepthAttachmentIndex(attachmentIndex)) {
			// @note: One depth image only, for read and write.
			auto usage = AfterglowDepthImage::inputAttachmentUsage();
			auto depthImage = std::make_unique<AfterglowDepthImage>(
				device(), extent, attachment.samples, usage
			);
			depthImage->sampler().setAddressModes(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
			passImageRefs.emplace_back(img::MakeImageReference(*depthImage));
			passImages.emplace_back(std::move(depthImage));
		}
		// ColorImage
		else {
			auto usage = AfterglowColorImage::defaultUsage();
			if (attachment.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL /* Input attachment and vast majority export*/
				|| pass.isExportColorAttachment(attachmentIndex) /* Present export case*/) {
				usage = AfterglowColorImage::inputAttachmentUsage();
			}
			auto colorImage = std::make_unique<AfterglowColorImage>(
			device(), extent, attachment.format, attachment.samples, usage
			);	
			colorImage->sampler().setAddressModes(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
			passImageRefs.emplace_back(img::MakeImageReference(*colorImage));
			passImages.emplace_back(std::move(colorImage));
		}
		// find present index.
		//if (attachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		//	presentIndex = attachmentIndex;
		//}
	}

	// Recreate pass barriers
	pass.clearBarriers();
	if (!_passManager.isFinalPass(pass)) {
		for (uint32_t exportColorAttachmentIndex : pass.exportColorAttachmentIndices()) {
			if (!AfterglowPassInterface::isValidAttachment(exportColorAttachmentIndex)) {
				continue;
			}
			VkImage exportColorImage = passImageRefs[exportColorAttachmentIndex].image;
			pass.appendColorBarrier(exportColorImage, exportColorAttachmentIndex);
		}
		if (AfterglowPassInterface::isValidAttachment(pass.exportDepthAttachmentIndex())) {
			VkImage exportDepthImage = passImageRefs[pass.exportDepthAttachmentIndex()].image;
			pass.appendDepthBarrier(exportDepthImage);
		}
	}

	// Recreate framebuffer(s)
	pass.clearFramebufferBindings();
	auto& passFramebuffers = _framebuffers[&pass];
	passFramebuffers.clear();
	uint32_t framebufferCount = 1;
	// On screen case
	bool onScreen = AfterglowPassInterface::isValidAttachment(pass.presentAttachmentIndex());
	if (onScreen) {
		framebufferCount = swapchain().imageViews().size();
	}

	for (uint32_t framebufferIndex = 0; framebufferIndex < framebufferCount; ++framebufferIndex) {
		AfterglowFramebuffer::AsElement* framebufferPtr = nullptr;
		framebufferPtr = &_framebuffers[&pass].emplace_back(AfterglowFramebuffer::makeElement(pass));
		AfterglowFramebuffer& framebuffer = *framebufferPtr;
		
		framebuffer->width = extent.width;
		framebuffer->height = extent.height;
		framebuffer->layers = 1;

		// [Order dependency] These attachments coresponding to renderPass's attachments
		for (uint32_t attachmentIndex = 0; attachmentIndex < subpassContext.attachmentCount(); ++attachmentIndex) {
			if (attachmentIndex == pass.presentAttachmentIndex()) {
				// Present attachment
				framebuffer.appendImageView(swapchain().imageView(framebufferIndex));
			}
			else {
				// Regular color and depth attachment
				framebuffer.appendImageView(passImageRefs[attachmentIndex].imageView);
			}
		}

		// Rebind pass framebuffer(s)
		pass.appendFramebuffer(framebuffer);
	}
}

inline VkExtent2D AfterglowFramebufferManager::passExtent(AfterglowPassInterface& pass) {
	switch (pass.extentMode()) {
	case (AfterglowPassInterface::ExtentMode::Fixed):
		return { pass.extent().x, pass.extent().y };
		break;
	case (AfterglowPassInterface::ExtentMode::Swapchain):
		return { 
			static_cast<uint32_t>(pass.scale().x * _swapchain.extent().width),
			static_cast<uint32_t>(pass.scale().y * _swapchain.extent().height),
		};
	default:
		EXCEPT_CLASS_RUNTIME(std::format("Unsupported extent mode: {}", util::EnumValue(pass.extentMode())));
	}
}

//inline AfterglowRenderPass* AfterglowFramebufferManager::findOnScreenRenderPass() {
//	for (int32_t index = _passManager.fixedPasses().size() - 1; index >= 0; --index) {
//		auto& fixedPass = _passManager.fixedPasses()[index];
//		if (fixedPass
//			&& fixedPass->extentMode() == AfterglowPassInterface::ExtentMode::Swapchain
//			&& fixedPass->exportColorAttachmentIndex() != AfterglowPassInterface::invalidAttachmentIndex()) {
//			return &fixedPass->renderPass();
//		}
//	}
//	return nullptr;
//}
