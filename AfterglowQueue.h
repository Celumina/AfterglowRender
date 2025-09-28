#pragma once
#include "AfterglowDevice.h"
#include "AfterglowSynchronizer.h"

class AfterglowQueue : public AfterglowObject {
public:
	AfterglowQueue(AfterglowDevice& device, uint32_t queueFamilyIndex);

	operator VkQueue& ();

protected:
	VkQueue _queue;
};