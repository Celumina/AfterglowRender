#include "AfterglowFramebuffer.h"

AfterglowFramebuffer::AfterglowFramebuffer(AfterglowRenderPass& renderPass) :
	_renderPass(renderPass), _imageViewAttachments(std::make_unique<ImageViewArray>()) {
}

AfterglowFramebuffer::~AfterglowFramebuffer() {
	destroy(vkDestroyFramebuffer, _renderPass.device(), data(), nullptr);
}

void AfterglowFramebuffer::appendImageView(VkImageView imageView) {
	_imageViewAttachments->push_back(imageView);
}

void AfterglowFramebuffer::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info().renderPass = _renderPass;
}

void AfterglowFramebuffer::create() {
	// Delay to aquire, due to vector data address could be changed.
	info().pAttachments = _imageViewAttachments->data();
	info().attachmentCount = static_cast<uint32_t>(_imageViewAttachments->size());

	_extent = { info().width, info().height }; 

	if (vkCreateFramebuffer(_renderPass.device(), &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create framebuffer.");
	}
}
