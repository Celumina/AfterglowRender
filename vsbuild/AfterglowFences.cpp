#include "AfterglowFences.h"

AfterglowFences::AfterglowFences(AfterglowDevice& device, uint32_t numFences) : 
	AfterglowProxyArray(numFences), _device(device) {
}

AfterglowFences::~AfterglowFences() {
	for (uint32_t index = 0; index < size(); ++index) {
		vkDestroyFence(_device, data()[index], nullptr);
	}
	destroy();
}

void AfterglowFences::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// Be make sure fence is signaled when first draw call 
	info().flags = VK_FENCE_CREATE_SIGNALED_BIT;
}

void AfterglowFences::create() {
	for (uint32_t index = 0; index < size(); ++index) {
		if (vkCreateFence(_device, &info(), nullptr, &data()[index]) != VK_SUCCESS) {
			throw runtimeError("Failed to create fences.");
		}
	}
}
