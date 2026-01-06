#include "AfterglowStorageImage.h"
#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowStorageBuffer.h"
#include "AfterglowStagingBuffer.h"

AfterglowStorageImage::AfterglowStorageImage(
	AfterglowDevice& device, 
	VkExtent3D extent, 
	VkFormat format, 
	compute::SSBOTextureDimension dimension, 
	compute::SSBOTextureSampleMode sampleMode) :
	AfterglowImage(device) {
	info().format = format;
	// TODO: If storage image is pure GPU image, don't use transfer dst.
	info().usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	info().extent = extent;
	_imageInfo.width = extent.width;
	_imageInfo.height = extent.height;
	_imageInfo.depth = extent.depth;

	// 3D Image support
	switch (dimension) {
	case compute::SSBOTextureDimension::Texture1D:
		info().imageType = VK_IMAGE_TYPE_1D;
		imageView()->viewType = VK_IMAGE_VIEW_TYPE_1D;
		break;
	case compute::SSBOTextureDimension::Texture2D:
		info().imageType = VK_IMAGE_TYPE_2D;
		imageView()->viewType = VK_IMAGE_VIEW_TYPE_2D;
		break;
	case compute::SSBOTextureDimension::Texture3D:
		info().imageType = VK_IMAGE_TYPE_3D;
		imageView()->viewType = VK_IMAGE_VIEW_TYPE_3D;
		break;
	default:
		DEBUG_CLASS_WARNING("Unknown texture dimension, the storage image will apply the default texture dimension: Texture");
	}

	// Sample modes
	switch (sampleMode) {
	case compute::SSBOTextureSampleMode::LinearRepeat:
		sampler().setAddressModes(VK_SAMPLER_ADDRESS_MODE_REPEAT);
		break;
	case compute::SSBOTextureSampleMode::LinearClamp:
		sampler().setAddressModes(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		break;
	default:
		DEBUG_CLASS_WARNING("Unknown sample mode, the storage image will apply the default sample mode: LinearRepeat.");
	}

	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void AfterglowStorageImage::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue, AfterglowStagingBuffer& stagingBuffer) {
	commandPool.allocateSingleCommand(graphicsQueue, [this](VkCommandBuffer commandBuffer) {
		auto barrier = makeBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(
			commandBuffer, 
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, 
			0, 
			nullptr, 
			0, 
			nullptr, 
			1, 
			&barrier
		);
	});

	commandPool.allocateSingleCommand(graphicsQueue, [this, &stagingBuffer](VkCommandBuffer commandBuffer) {
		VkBufferImageCopy region {
			.bufferOffset = 0, 
			.bufferRowLength = 0, 
			.bufferImageHeight = 0, 
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1, 
			},
			.imageOffset = { 0, 0, 0 }, 
			.imageExtent = { 
				static_cast<uint32_t>(_imageInfo.width), 
				static_cast<uint32_t>(_imageInfo.height), 
				static_cast<uint32_t>(_imageInfo.depth) 
			}
		};
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	});

	commandPool.allocateSingleCommand(graphicsQueue, [this](VkCommandBuffer commandBuffer) {
		auto barrier = makeBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // TODO: Or whatever stage uses it
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	});
}
