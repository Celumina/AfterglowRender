#include "AfterglowPresentQueue.h"
#include <stdexcept>

#include "AfterglowWindow.h"

AfterglowPresentQueue::AfterglowPresentQueue(AfterglowDevice& device) : 
	AfterglowQueue(device, device.physicalDevice().presentFamilyIndex()) {
}

void AfterglowPresentQueue::submit(AfterglowWindow& window, AfterglowFramebufferManager& framebufferManager, AfterglowSynchronizer& synchronizer, uint32_t imageIndex) {
	VkSemaphore signalSemaphores[] = { synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::RenderFinished) };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { framebufferManager.swapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	// Finally we submit the request to present an image to the swap chain!
	VkResult result = vkQueuePresentKHR(_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.resized()) {
		window.waitIdle([&framebufferManager](){ framebufferManager.recreate(); });
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("[AfterglowPresentQueue] Failed to present swap chain image.");
	}
}

