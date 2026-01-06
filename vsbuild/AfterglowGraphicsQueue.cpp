#include "AfterglowGraphicsQueue.h"
#include "AfterglowSynchronizer.h"
#include "AfterglowPhysicalDevice.h"

AfterglowGraphicsQueue::AfterglowGraphicsQueue(AfterglowDevice& device) : 
	AfterglowQueue(device, device.physicalDevice().graphicsFamilyIndex()) {
}

void AfterglowGraphicsQueue::submit(VkCommandBuffer* commandBuffers, AfterglowSynchronizer& synchronizer) {
	// Which semaphores ans which stages want to wait.
	VkSemaphore waitSemaphores[] = {
		synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::ComputeFinished),
		synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::ImageAvaliable)
	};

	// waitStages means that
	// theoretically the implementation can already start executing our vertex shader
	// and such while the image is not yet available.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	// Specify which semaphores to signal once the command buffer(s) have finished execution.
	VkSemaphore signalSemaphores[] = { synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::RenderFinished) };

	// Submit optimization.
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(VkSemaphore);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	// commandBuffers to actually submit
	submitInfo.commandBufferCount = 1;
	// Address from first element.
	// AfterglowCommandBuffer grarentees same memory layout with VkCommandBuffer.
	submitInfo.pCommandBuffers = commandBuffers;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// DEBUG_COST_BEGIN("GraphicsSubmit");
	if (vkQueueSubmit(_queue, 1, &submitInfo, synchronizer.fence(AfterglowSynchronizer::FenceFlag::RenderInFlight)) != VK_SUCCESS) {
		throw std::runtime_error("[AfterglowGraphicsQueue] Failed to submit draw command buffer.");
	}
	// DEBUG_COST_END;
} 
