#pragma once
#include "AfterglowDevice.h"

class AfterglowSampler : public AfterglowProxyObject<AfterglowSampler, VkSampler, VkSamplerCreateInfo> {
public:
	AfterglowSampler(AfterglowDevice& device);
	~AfterglowSampler();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};

