#include "AfterglowDeviceMemory.h"

AfterglowDeviceMemory::AfterglowDeviceMemory(AfterglowDevice& device) : 
	_device(device) {
}

AfterglowDeviceMemory::~AfterglowDeviceMemory() {
	destroy(vkFreeMemory, _device, data(), nullptr);
}

void AfterglowDeviceMemory::initCreateInfo() {
	// Configurate in other buffer class which is have to allocate memory manually.
	info().sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
}

void AfterglowDeviceMemory::create() {
	if (vkAllocateMemory(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to allocate device memory.");
	}
}
