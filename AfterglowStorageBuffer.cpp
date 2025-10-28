#include "AfterglowStorageBuffer.h"

#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowStagingBuffer.h"


AfterglowStorageBuffer::AfterglowStorageBuffer(AfterglowDevice& device, const void* buffer, uint64_t bufferSize) :
	AfterglowBuffer(device), _buffer(buffer), _bufferSize(bufferSize) {
	// TODO: Here usage vertex buffer should be optional.
	info().usage =
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void AfterglowStorageBuffer::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) {
	AfterglowStagingBuffer stagingBuffer(_device, _buffer, _bufferSize);
	submit(commandPool, graphicsQueue, stagingBuffer);
}

void AfterglowStorageBuffer::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue, AfterglowStagingBuffer& stagingBuffer) {
	commandPool.allocateSingleCommand(
		graphicsQueue,
		[this, &stagingBuffer](VkCommandBuffer commandBuffer) {cmdCopyBuffer(commandBuffer, stagingBuffer); }
	);
}

uint64_t AfterglowStorageBuffer::byteSize() {
	return _bufferSize;
}
