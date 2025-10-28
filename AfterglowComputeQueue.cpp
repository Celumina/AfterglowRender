#include "AfterglowComputeQueue.h"
#include "AfterglowSynchronizer.h"
#include "AfterglowPhysicalDevice.h"

// Compute use same the queue with Graphics now.
AfterglowComputeQueue::AfterglowComputeQueue(AfterglowDevice& device) : 
	AfterglowQueue(device, device.physicalDevice().graphicsFamilyIndex()){
}


void AfterglowComputeQueue::cancelSemaphore(AfterglowSynchronizer& synchronizer) {
	synchronizer.wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	synchronizer.reset(AfterglowSynchronizer::FenceFlag::ComputeInFlight);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::ComputeFinished);
	submitInfo.pWaitDstStageMask = waitStages;

	if (vkQueueSubmit(_queue, 1, &submitInfo, synchronizer.fence(AfterglowSynchronizer::FenceFlag::ComputeInFlight)) != VK_SUCCESS) {
		throw std::runtime_error("[AfterglowComputeQueue] Failed to cancel compute semaphore.");
	}
}

void AfterglowComputeQueue::submit(VkCommandBuffer* commandBuffers, AfterglowSynchronizer& synchronizer) {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &synchronizer.semaphore(AfterglowSynchronizer::SemaphoreFlag::ComputeFinished);

	if (vkQueueSubmit(_queue, 1, &submitInfo, synchronizer.fence(AfterglowSynchronizer::FenceFlag::ComputeInFlight)) != VK_SUCCESS) {
		throw std::runtime_error("[AfterglowComputeQueue] Failed to submit compute command buffer.");
	}
}