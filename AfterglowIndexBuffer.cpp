#include "AfterglowIndexBuffer.h"

#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowStagingBuffer.h"


AfterglowIndexBuffer::AfterglowIndexBuffer(AfterglowDevice& device) : 
	AfterglowBuffer(device), _indexInfo({}) {
	info().usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info().sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}

//void AfterglowIndexBuffer::initIndices(uint64_t indexCount) {
//	_indices.resize(indexCount);
//	initIndicesImplimentation();
//}

void AfterglowIndexBuffer::bind(std::weak_ptr<IndexArray> data) {
	_indices = data;
	initIndicesImplimentation();
}

void AfterglowIndexBuffer::setIndex(Size pos, Index value) {
	(*safeIndices())[pos] = value;
}

uint64_t AfterglowIndexBuffer::byteSize() {
	return sizeof(Index) * safeIndices()->size();
}

AfterglowIndexBuffer::Size AfterglowIndexBuffer::indexCount() const {
	return _indexInfo.count;
}

std::weak_ptr<const AfterglowIndexBuffer::IndexArray> AfterglowIndexBuffer::indexData() const {
	return _indices;
}

void AfterglowIndexBuffer::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) {
	AfterglowStagingBuffer stagingBuffer(_device, safeIndices()->data(), byteSize());
	commandPool.allocateSingleCommand(
		graphicsQueue, 
		[this, &stagingBuffer](VkCommandBuffer commandBuffer) {cmdCopyBuffer(commandBuffer, stagingBuffer); }
	);
	// Indices is presistantly map to GPU, so NEVER clear then.
}

inline void AfterglowIndexBuffer::initIndicesImplimentation() {
	if (_memory) {
		throw runtimeError("Indices had been created early.");
	}
	info().size = byteSize();
	_indexInfo.count = static_cast<Size>(safeIndices()->size());

	initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

inline std::shared_ptr<AfterglowIndexBuffer::IndexArray> AfterglowIndexBuffer::safeIndices() {
	auto lockedPtr = _indices.lock();
	if (!lockedPtr) {
		throw runtimeError("Indices was not found, due to vertex data source was destructed.");
	}
	return lockedPtr;
}
