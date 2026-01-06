#include "AfterglowStorageBuffer.h"

#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowStagingBuffer.h"



AfterglowStorageBuffer::AfterglowStorageBuffer(AfterglowDevice& device, const void* buffer, uint64_t bufferSize, compute::SSBOUsage usage) :
	AfterglowBuffer(device), _buffer(buffer), _bufferSize(bufferSize) {
	// TODO: Here usage vertex buffer should be optional?
	info().usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	
	switch (usage) {
	case compute::SSBOUsage::IndexInput:
		info().usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		break;
	case compute::SSBOUsage::VertexInput:
		info().usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		break;
	case compute::SSBOUsage::Indirect:
		info().usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		break;
	default:
		break;
	}

	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

//AfterglowStorageBuffer::AsElement AfterglowStorageBuffer::makeIndirectCommandBuffer(AfterglowDevice& device) {
//	VkDrawIndirectCommand command{};
//	return AfterglowStorageBuffer::makeElement(device, &command, sizeof(VkDrawIndirectCommand), compute::SSBOUsage::Indirect);
//}
//
//AfterglowStorageBuffer::AsElement AfterglowStorageBuffer::makeIndexedIndirectCommandBuffer(AfterglowDevice& device) {
//	VkDrawIndexedIndirectCommand command{};
//	return AfterglowStorageBuffer::makeElement(device, &command, sizeof(VkDrawIndexedIndirectCommand), compute::SSBOUsage::Indirect);
//}

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
