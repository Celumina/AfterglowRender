#pragma once
#include "AfterglowDevice.h"

// Graphics pipeline layout
// TODO: Devision with Compute pipeline
class AfterglowPipelineLayout : public AfterglowProxyObject<AfterglowPipelineLayout, VkPipelineLayout, VkPipelineLayoutCreateInfo> {
public:
	AfterglowPipelineLayout(AfterglowDevice& device);
	~AfterglowPipelineLayout();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};

