#pragma once
#include "AfterglowImage.h"

class AfterglowCommandPool;
class AfterglowGraphicsQueue;

class AfterglowDepthImage : public AfterglowImage<AfterglowDepthImage> {
public:
	AfterglowDepthImage(
		AfterglowDevice& device, 
		VkExtent2D extent, 
		VkSampleCountFlagBits sampleCount, 
		VkImageUsageFlags usage = defaultUsage()
	);

	// Optional operation.
	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	// @deprecated: Use AsElement::recreate instead.
	// void recreate(VkExtent2D extent);

	static constexpr VkImageUsageFlags defaultUsage();
	static constexpr VkImageUsageFlags inputAttachmentUsage();

private:
	bool hasStencilComponent();

	// Single Command.
	void cmdPipelineBarrier(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
};

constexpr VkImageUsageFlags AfterglowDepthImage::defaultUsage() {
	return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
}

constexpr VkImageUsageFlags AfterglowDepthImage::inputAttachmentUsage() {
	return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
}