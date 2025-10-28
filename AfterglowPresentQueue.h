#pragma once
#include "AfterglowQueue.h"

class AfterglowFramebufferManager;
class AfterglowWindow;

class AfterglowPresentQueue : public AfterglowQueue {
public:
	AfterglowPresentQueue(AfterglowDevice& device);
	
	void submit(AfterglowWindow& window, AfterglowFramebufferManager& framebufferManager, AfterglowSynchronizer& synchronizer, uint32_t imageIndex);	

private:
	uint32_t presentCount = 0;

};

