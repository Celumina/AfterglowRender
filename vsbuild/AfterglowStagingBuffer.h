#pragma once
#include "AfterglowBuffer.h"
class AfterglowStagingBuffer : public AfterglowBuffer<AfterglowStagingBuffer> {
public:
	AfterglowStagingBuffer(AfterglowDevice& device, const void* bufferSource, uint64_t bufferSize);

protected:
	uint64_t byteSize() override;

private:
	// Fill memory with data.
	inline void fillMemory(const void* bufferSource, size_t bufferSize);
	uint64_t _size;
};

