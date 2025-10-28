#include "AfterglowCommandPool.h"
#include "AfterglowPhysicalDevice.h"

#include "Configurations.h"

AfterglowCommandPool::AfterglowCommandPool(AfterglowDevice& device) : 
_device(device) {
}

AfterglowCommandPool::~AfterglowCommandPool() {
	destroy(vkDestroyCommandPool, _device, data(), nullptr);
}

AfterglowDevice& AfterglowCommandPool::device() noexcept {
	return _device;
}

void AfterglowCommandPool::initCreateInfo() {
	// Each command pool
	// can only allocate command buffers that are submitted on a single type of queue.
	info().sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// Every frame we want to use this commandbuffer, so mark it as RESET
	info().flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info().queueFamilyIndex = _device.physicalDevice().graphicsFamilyIndex();
}

void AfterglowCommandPool::create() {
	if (vkCreateCommandPool(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create command pool.");
	}
}
