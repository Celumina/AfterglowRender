#pragma once
#include "AfterglowDevice.h"
#include "AfterglowProxyArray.h"
class AfterglowFences : public AfterglowProxyArray<AfterglowFences, VkFence, VkFenceCreateInfo> {
public:
	AfterglowFences(AfterglowDevice& device, uint32_t numFences);
	~AfterglowFences();

// proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;

};

