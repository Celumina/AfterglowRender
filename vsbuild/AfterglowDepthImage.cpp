#include "AfterglowDepthImage.h"

#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "ExceptionUtilities.h"
#include "Configurations.h"

AfterglowDepthImage::AfterglowDepthImage(
	AfterglowDevice& device, 
	VkExtent2D extent, 
	VkSampleCountFlagBits sampleCount, 
	VkImageUsageFlags usage
	) : AfterglowImage(device) {
	info().format = _device.physicalDevice().findDepthFormat();
	info().extent.width = extent.width;
	info().extent.height = extent.height;
	info().samples = sampleCount;
	info().usage = usage;

	if (hasStencilComponent()) {
		_imageInfo.format = img::Format::DepthStencil;
	}
	else {
		_imageInfo.format = img::Format::DepthOnly;
	}

	imageView()->format = info().format;
	imageView()->subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void AfterglowDepthImage::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) {
	commandPool.allocateSingleCommand(
		graphicsQueue,
		[this](VkCommandBuffer commandBuffer) {
			cmdPipelineBarrier(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}
	);
}

//void AfterglowDepthImage::recreate(VkExtent2D extent) {
//	// _memory will be recrerate in initMemory() so only the imageView should to be recreated manually.
//	_imageView.recreate(_device);
//	destroy(vkDestroyImage, _device, data(), nullptr);
//	initDepthImage(extent);
//}

bool AfterglowDepthImage::hasStencilComponent() {
	return info().format == VK_FORMAT_D32_SFLOAT_S8_UINT || info().format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void AfterglowDepthImage::cmdPipelineBarrier(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) {
	auto barrier = makeBarrier(oldLayout, newLayout);

	// Depth buffer image only.
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		// Also you can acquire this value form imageView.
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (_imageInfo.format == img::Format::DepthStencil) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		EXCEPT_CLASS_INVALID_ARG("Unsupported layout transition.");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
