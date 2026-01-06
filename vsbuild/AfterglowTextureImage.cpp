#include "AfterglowTextureImage.h"
#include <cmath>


#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowStagingBuffer.h"


AfterglowTextureImage::AfterglowTextureImage(AfterglowDevice& device) : 
	AfterglowImage(device), _mipLevels(1) {
}

AfterglowTextureImage::~AfterglowTextureImage() {
}

void AfterglowTextureImage::bind(const img::Info& imageInfo, std::weak_ptr<std::vector<char>> imageData) {
	if (_memory) {
		throw runtimeError("Indices had been created early.");
	}

	_imageInfo = imageInfo;
	_imageData = imageData;

	// Calculate the number of mipmaps.
	_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(_imageInfo.width, _imageInfo.height))) + 1);

	info().extent.width = _imageInfo.width;
	info().extent.height = _imageInfo.height;
	info().mipLevels = _mipLevels;
	info().format = vulkanFormat(_imageInfo);

	_imageView->subresourceRange.levelCount = _mipLevels;
	_sampler->maxLod = static_cast<float>(_mipLevels);

	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

std::weak_ptr<img::DataArray> AfterglowTextureImage::imageData() {
	return _imageData;
}

void AfterglowTextureImage::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) {
	auto lockedPtr = imageData().lock();
	if (!lockedPtr) {
		throw runtimeError("Image data not found, due to the image data source was destructed.");
	}
	AfterglowStagingBuffer stagingBuffer(_device, lockedPtr->data(), size());

	commandPool.allocateSingleCommand(
		graphicsQueue,
		[this](VkCommandBuffer commandBuffer) {
			cmdPipelineBarrier(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); 
		}
	);

	commandPool.allocateSingleCommand(
		graphicsQueue,
		[this, &stagingBuffer](VkCommandBuffer commandBuffer) {
			cmdCopyBufferToImage(commandBuffer, stagingBuffer);
		}
	);

	// Transitioned to VK_IMAGE_LAYOUT_SHDAER_READ_ONLY_OPTIMAL while generating mipmaps.
	//commandPool.allocateSingleCommand(
	//	graphicsQueue,
	//	[this](VkCommandBuffer commandBuffer) {
	//		cmdPipelineBarrier(
	//			commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	//		);
	//	}
	//);

	// TODO: Generate mipmap when Offline (or save as cache).
	commandPool.allocateSingleCommand(
		graphicsQueue,
		[this](VkCommandBuffer commandBuffer) {
			cmdGenerateMipmaps(commandBuffer);
		}
	);
}

void AfterglowTextureImage::cmdCopyBufferToImage(VkCommandBuffer commandBuffer, AfterglowStagingBuffer& srcStagingBuffer) {
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<uint32_t>(_imageInfo.width), static_cast<uint32_t>(_imageInfo.height), 1 };

	vkCmdCopyBufferToImage(commandBuffer, srcStagingBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void AfterglowTextureImage::cmdPipelineBarrier(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) {
	auto barrier = makeBarrier(oldLayout, newLayout);
	barrier.subresourceRange.levelCount = _mipLevels;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		// Transfer texture form host.
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (
		oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
		&& newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		) {
		// Transfer texture to shaders.
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("[AfterglowTextureImage] Unsupported layout transition.");
	}

	vkCmdPipelineBarrier(
		commandBuffer, 
		sourceStage, 
		destinationStage, 
		0, 
		0, 
		nullptr, 
		0, 
		nullptr, 
		1, 
		&barrier
	);
}

void AfterglowTextureImage::cmdGenerateMipmaps(VkCommandBuffer commandBuffer) {
	VkFormatProperties formatProperties = 
		_device.physicalDevice().formatProperties(vulkanFormat(_imageInfo));

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw runtimeError("Texture image format does not support linear bliting.");
	}

	VkImageMemoryBarrier barrier = makeBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED);
	int32_t mipWidth = _imageInfo.width;
	int32_t mipHeight = _imageInfo.height;
	// Level 0 source image, do not generate it.
	for (uint32_t index = 1; index < _mipLevels; ++index) {
		barrier.subresourceRange.baseMipLevel = index - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, 
			0, nullptr, 
			0, nullptr, 
			1, &barrier
		);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = index - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		// Next mip level extent.
		mipWidth = std::max(mipWidth / 2, 1);
		mipHeight = std::max(mipHeight / 2, 1);

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = index;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		
		vkCmdBlitImage(
			commandBuffer, 
			*this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			1, &blit, 
			VK_FILTER_LINEAR
		);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		// TODO: Here new and next mip old is not corresponding?
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		commandBuffer, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
		0, 
		0, nullptr, 
		0, nullptr, 
		1, &barrier
	);
}
