#include "AfterglowImageView.h"

AfterglowImageView::AfterglowImageView(AfterglowDevice& device) : 
	_device(device) {
}

AfterglowImageView::~AfterglowImageView() {
	destroy(vkDestroyImageView, _device, data(), nullptr);
}

void AfterglowImageView::initCreateInfo() {
	// Here is Default settings for simplify imageview creating.
	// Assign these attributes explicitly is necessary:
	// ->image
	// ->format

	info().sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info().viewType = VK_IMAGE_VIEW_TYPE_2D;
	info().subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	info().subresourceRange.baseMipLevel = 0;
	info().subresourceRange.levelCount = 1;
	info().subresourceRange.baseArrayLayer = 0;
	info().subresourceRange.layerCount = 1;
}

void AfterglowImageView::create() {
	if (vkCreateImageView(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create image view.");
	}
}
