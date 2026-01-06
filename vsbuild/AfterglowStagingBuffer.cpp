#include "AfterglowStagingBuffer.h"

AfterglowStagingBuffer::AfterglowStagingBuffer(AfterglowDevice& device, const void* bufferSource, uint64_t bufferSize) :
	AfterglowBuffer(device), _size(bufferSize) {
	info().size = bufferSize;
	info().usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	initMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// Fill in vertex data.
	fillMemory(bufferSource, bufferSize);
}

uint64_t AfterglowStagingBuffer::byteSize() {
	return _size;
}

inline void AfterglowStagingBuffer::fillMemory(const void* bufferSource, size_t bufferSize) {
	// Fill data to device(vk) memory
	void* data;
	vkMapMemory(_device, _memory, 0, bufferSize, 0, &data);
	memcpy(data, bufferSource, bufferSize);
	vkUnmapMemory(_device, _memory);
}
