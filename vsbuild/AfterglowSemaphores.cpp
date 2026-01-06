#include "AfterglowSemaphores.h"

AfterglowSemaphores::AfterglowSemaphores(AfterglowDevice& device, uint32_t numSemaphores) :
	AfterglowProxyArray(numSemaphores), _device(device) {
}

AfterglowSemaphores::~AfterglowSemaphores() {
	// Destroy array manually.
	for (uint32_t index = 0; index < size(); ++index) {
		vkDestroySemaphore(_device, data()[index], nullptr);
	}
	destroy();
}

void AfterglowSemaphores::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}

void AfterglowSemaphores::create() {
	for (uint32_t index = 0; index < size(); ++index) {
		if (vkCreateSemaphore(_device, &info(), nullptr, &(data()[index])) != VK_SUCCESS) {
			throw runtimeError("Failed to create semaphore.");
		}
	}
}
