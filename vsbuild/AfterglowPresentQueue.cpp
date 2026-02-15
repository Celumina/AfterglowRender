#include "AfterglowPresentQueue.h"
#include "AfterglowFramebufferManager.h"
#include "AfterglowWindow.h"
#include "AfterglowPhysicalDevice.h"
#include "AfterglowSwapchain.h"
#include "AfterglowSynchronizer.h"
#include "ExceptionUtilities.h"

AfterglowPresentQueue::AfterglowPresentQueue(AfterglowDevice& device) : 
	AfterglowQueue(device, device.physicalDevice().presentFamilyIndex()) {
}

void AfterglowPresentQueue::submit(AfterglowWindow& window, AfterglowFramebufferManager& framebufferManager, AfterglowSynchronizer& synchronizer, uint32_t imageIndex) {
	VkSemaphore signalSemaphores[] = { synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::RenderFinished) };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { framebufferManager.swapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	// Finally we submit the request to present an image to the swap chain!
	// DEBUG_COST_BEGIN("PresentSubmit");
	VkResult result = vkQueuePresentKHR(_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.resized()) {
		window.waitIdle([&framebufferManager](){ framebufferManager.recreateSwapchain(); });
	}
	else if (result != VK_SUCCESS) {
		EXCEPT_CLASS_RUNTIME("Failed to present swapchain image.");
	}
	//else {
	//	window.setPresented(true);
	//}
	// DEBUG_COST_END;
}

