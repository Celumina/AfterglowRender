#pragma once
#include "AfterglowDevice.h"
class AfterglowDeviceMemory : public AfterglowProxyObject<AfterglowDeviceMemory, VkDeviceMemory, VkMemoryAllocateInfo> {
public:
	AfterglowDeviceMemory(AfterglowDevice& device);
	~AfterglowDeviceMemory();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};

