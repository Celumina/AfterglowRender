#pragma once
#include "AfterglowQueue.h"
#include "AfterglowFramebufferManager.h"
class AfterglowPresentQueue : public AfterglowQueue {
public:
	AfterglowPresentQueue(AfterglowDevice& device);
	
	void submit(AfterglowWindow& window, AfterglowFramebufferManager& framebufferManager, AfterglowSynchronizer& synchronizer, uint32_t imageIndex);	
};

