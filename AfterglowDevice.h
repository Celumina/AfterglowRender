#pragma once

#include "AfterglowProxyObject.h"

class AfterglowPhysicalDevice;

class AfterglowDevice : public AfterglowProxyObject<AfterglowDevice, VkDevice, VkDeviceCreateInfo> {
public:
	using QueueCreateInfoArray = std::vector<VkDeviceQueueCreateInfo>;
	AfterglowDevice(AfterglowPhysicalDevice& physicalDevice);
	~AfterglowDevice();

	AfterglowPhysicalDevice& physicalDevice();

	void waitIdle();
	uint32_t currentFrameIndex() const;
	uint32_t lastFrameIndex() const;
	void updateCurrentFrameIndex();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowPhysicalDevice& _physicalDevice;
	std::unique_ptr<VkPhysicalDeviceFeatures> _deviceFeatures;
	std::unique_ptr<QueueCreateInfoArray> _queueCreateInfos;
	// Only one priority is supported yet. queuePriority range from 0.0 to 1.0.
	float _queuePriority = 1.0f;

	uint32_t _currentFrameIndex;
};

