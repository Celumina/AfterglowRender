#pragma once
#include "AfterglowImage.h"

class AfterglowColorImage : public AfterglowImage<AfterglowColorImage> {
public:
	AfterglowColorImage(
		AfterglowDevice& device, 
		VkExtent2D extent, 
		VkFormat format, 
		VkSampleCountFlagBits sampleCount, 
		VkImageUsageFlags usage = defaultUsage()
		);

	constexpr static VkImageUsageFlags defaultUsage();
	constexpr static VkImageUsageFlags inputAttachmentUsage();
	constexpr static VkImageUsageFlags computeShaderUsage();

};


constexpr VkImageUsageFlags AfterglowColorImage::defaultUsage() {
	return VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

constexpr VkImageUsageFlags AfterglowColorImage::inputAttachmentUsage() {
	return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

constexpr VkImageUsageFlags AfterglowColorImage::computeShaderUsage() {
	return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
}