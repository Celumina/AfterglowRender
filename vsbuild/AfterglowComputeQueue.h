#pragma once
#include "AfterglowQueue.h"

class AfterglowComputeQueue : public AfterglowQueue {
public:
	AfterglowComputeQueue(AfterglowDevice& device);

	/*
	@desc:
		If semaphore is signaled but in some reasom graphics queue will not submit, 
		cancel semaphore to avoid repeative signal.
		This operation includes fence sychronzation, so you don't need to wait and reset them.
	*/
	void cancelSemaphore(AfterglowSynchronizer& synchronizer);

	void submit(VkCommandBuffer* commandBuffers, AfterglowSynchronizer& synchronizer);
};

