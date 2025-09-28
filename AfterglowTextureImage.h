#pragma once
#include "AfterglowImage.h"
#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowStagingBuffer.h"

class AfterglowTextureImage : public AfterglowImage<AfterglowTextureImage> {
public:
	AfterglowTextureImage(AfterglowDevice& device);
	~AfterglowTextureImage();
	
	void bind(const img::Info& info, std::weak_ptr<void> imageData);
	std::weak_ptr<void> imageData();

	// Creating a staging buffer to transfer data to GPU and then free imageData automatically.
	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

private:
	// These cmd functin use for single time command.
	void cmdCopyBufferToImage(VkCommandBuffer commandBuffer, AfterglowStagingBuffer& srcStagingBuffer);
	void cmdPipelineBarrier(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
	void cmdGenerateMipmaps(VkCommandBuffer commandBuffer);

	// Different with IndexBuffer and VertexBuffer, this _imageData just a ref, this class doesn't not manage imageData manually.
	std::weak_ptr<void> _imageData;
	uint32_t _mipLevels;
};