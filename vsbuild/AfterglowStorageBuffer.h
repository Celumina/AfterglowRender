#pragma once
#include "AfterglowBuffer.h"
#include "ComputeDefinitions.h"

class AfterglowCommandPool;
class AfterglowGraphicsQueue;
class AfterglowStagingBuffer;

class AfterglowStorageBuffer : public AfterglowBuffer<AfterglowStorageBuffer> {
public:
	AfterglowStorageBuffer(AfterglowDevice& device, const void* buffer, uint64_t bufferSize, compute::SSBOUsage usage);

	//static AfterglowStorageBuffer::AsElement makeIndirectCommandBuffer(AfterglowDevice& device);
	//static AfterglowStorageBuffer::AsElement makeIndexedIndirectCommandBuffer(AfterglowDevice& device);

	/**
	* @usage: 
	*	Submit single storage buffer, staging buffer will be created automatically.
	*/
	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	/**
	* @usage: 
	* 	If you want to create multiple buffers, use this interface to use shared staging buffer. 
	* Reducing staging buffer construction cost.
	*/
	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue, AfterglowStagingBuffer& stagingBuffer);

	uint64_t byteSize() override;

private:
	uint64_t _bufferSize;
	const void* _buffer;
};