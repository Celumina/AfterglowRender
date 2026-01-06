#pragma once
#include "AfterglowDevice.h"

class AfterglowSampler : public AfterglowProxyObject<AfterglowSampler, VkSampler, VkSamplerCreateInfo> {
public:
	AfterglowSampler(AfterglowDevice& device);
	~AfterglowSampler();

	// Set address mode to uvw dimensions.
	void setAddressModes(VkSamplerAddressMode mode) noexcept;

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};

