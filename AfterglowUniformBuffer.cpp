#include "AfterglowUniformBuffer.h"

AfterglowUniformBuffer::AfterglowUniformBuffer(AfterglowDevice& device, const void* uniform, uint64_t uniformSize) :
	AfterglowBuffer(device), _uniform(uniform), _uniformSize(uniformSize) {
	info().usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	// info().size = _uniformSize;
	initMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// Do not unmap for Persistent Mapping.
	vkMapMemory(device, _memory, 0, _uniformSize, 0, &_mapped);
	updateMemory();
}

void AfterglowUniformBuffer::updateMemory() {
	memcpy(_mapped, _uniform, _uniformSize);
}

uint64_t AfterglowUniformBuffer::byteSize() {
	return _uniformSize;
}

const void* AfterglowUniformBuffer::sourceData() const {
	return _uniform;
}
