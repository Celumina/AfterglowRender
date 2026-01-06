#pragma once
#include "AfterglowQueue.h"

class AfterglowGraphicsQueue : public AfterglowQueue {
public:
	AfterglowGraphicsQueue(AfterglowDevice& device);

	void submit(VkCommandBuffer* commandBuffers, AfterglowSynchronizer& synchronizer);
};

