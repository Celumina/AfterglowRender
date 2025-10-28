#pragma once
#include "AfterglowDevice.h"
#include "AfterglowGraphicsQueue.h"
class AfterglowCommandPool : public AfterglowProxyObject<AfterglowCommandPool, VkCommandPool, VkCommandPoolCreateInfo> {
public:
	AfterglowCommandPool(AfterglowDevice& device);
	~AfterglowCommandPool();

	AfterglowDevice& device() noexcept;

	// Input callback func format: func(VkCommandBuffer)
	template<typename FuncType>
	void allocateSingleCommand(AfterglowGraphicsQueue& graphicQueue, FuncType&& func);

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
};


template<typename FuncType>
void AfterglowCommandPool::allocateSingleCommand(AfterglowGraphicsQueue& graphicsQueue, FuncType&& func) {
	// Temp command buffer use for transfer buffer.
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = *this;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device(), &allocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	func(commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device(), *this, 1, &commandBuffer);
}
