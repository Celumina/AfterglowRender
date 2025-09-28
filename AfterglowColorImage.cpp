#include "AfterglowColorImage.h"
#include "Configurations.h"

AfterglowColorImage::AfterglowColorImage(
	AfterglowDevice& device, 
	VkExtent2D extent, 
	VkFormat format, 
	VkSampleCountFlagBits sampleCount,
	VkImageUsageFlags usage
	) : AfterglowImage(device) {
	info().format = format;
	info().samples = sampleCount;
	info().usage = usage;
	info().extent.width = extent.width;
	info().extent.height = extent.height;
	
	_imageView->format = format;
	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

