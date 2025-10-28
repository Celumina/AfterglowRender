#pragma once
#include "AfterglowDevice.h"

class AfterglowSynchronizer;

class AfterglowQueue : public AfterglowObject {
public:
	AfterglowQueue(AfterglowDevice& device, uint32_t queueFamilyIndex);

	operator VkQueue& ();

protected:
	VkQueue _queue;
};