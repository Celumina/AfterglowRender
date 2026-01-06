#pragma once
#include <vector>

#include "AfterglowDevice.h"

// Fundanmantal Class, So createInfo is not filled.
class AfterglowImageView : public AfterglowProxyObject<AfterglowImageView, VkImageView, VkImageViewCreateInfo> {
public:
	AfterglowImageView(AfterglowDevice& device);
	~AfterglowImageView();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};

