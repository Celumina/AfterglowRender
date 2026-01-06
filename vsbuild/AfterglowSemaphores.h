#pragma once
#include "AfterglowDevice.h"
#include "AfterglowProxyArray.h"

class AfterglowSemaphores : public AfterglowProxyArray<AfterglowSemaphores, VkSemaphore, VkSemaphoreCreateInfo> {
public:
	AfterglowSemaphores(AfterglowDevice& device, uint32_t numSemaphores);
	~AfterglowSemaphores();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};

