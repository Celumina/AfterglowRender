#include "AfterglowSynchronizer.h"
#include <stdexcept>

#include "AfterglowUtilities.h"

AfterglowSynchronizer::AfterglowSynchronizer(AfterglowDevice& device) : 
	_device(device) {
	for (int frameIndex = 0; frameIndex < cfg::maxFrameInFlight; ++frameIndex) {
		auto& semaphores = _inFlightSemaphores[frameIndex];
		auto& fences = _inFlightFences[frameIndex];
		semaphores.recreate(device, util::EnumValue(SemaphoreFlag::EnumCount));
		fences.recreate(device, util::EnumValue(FenceFlag::EnumCount));
	}
}

void AfterglowSynchronizer::wait(FenceFlag fenceFlag) {
	vkWaitForFences(_device, 1, &fence(fenceFlag), VK_TRUE,  UINT64_MAX);
}

VkResult AfterglowSynchronizer::fenceStatus(FenceFlag fenceFlag) {
	return vkGetFenceStatus(_device, fence(fenceFlag));
}

void AfterglowSynchronizer::reset(FenceFlag fenceFlag) {
	vkResetFences(_device, 1, &fence(fenceFlag));
}

VkSemaphore& AfterglowSynchronizer::semaphore(SemaphoreFlag semaphoreFlag) {
	return _inFlightSemaphores[_device.currentFrameIndex()][util::EnumValue(semaphoreFlag)];
}

VkFence& AfterglowSynchronizer::fence(FenceFlag fenceFlag) {
	return _inFlightFences[_device.currentFrameIndex()][util::EnumValue(fenceFlag)];
}

AfterglowDevice& AfterglowSynchronizer::device() noexcept {
	return _device;
}
