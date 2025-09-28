#include "AfterglowDevice.h"
#include <set>

#include "Configurations.h"

AfterglowDevice::AfterglowDevice(AfterglowPhysicalDevice& physicalDevice) : 
	_physicalDevice(physicalDevice), _currentFrameIndex(0) {
}

AfterglowDevice::~AfterglowDevice() {
	destroy(vkDestroyDevice, data(), nullptr);
}

AfterglowPhysicalDevice& AfterglowDevice::physicalDevice() {
	return _physicalDevice;
}

void AfterglowDevice::waitIdle() {
	vkDeviceWaitIdle(*this);
}

uint32_t AfterglowDevice::currentFrameIndex() const {
	return _currentFrameIndex;
}

uint32_t AfterglowDevice::lastFrameIndex() const {
	return (_currentFrameIndex + cfg::maxFrameInFlight - 1) % cfg::maxFrameInFlight;
}

void AfterglowDevice::updateCurrentFrameIndex() {
	_currentFrameIndex = (_currentFrameIndex + 1) % cfg::maxFrameInFlight;
}

void AfterglowDevice::initCreateInfo() {
	// (Optional) Info ptr will be init on initCreateInfoShell automatically.
	// AfterglowProxyObject::initCreateInfo();

	std::set<uint32_t> uniqueQueueFamilies = {
		_physicalDevice.graphicsFamilyIndex(),
		_physicalDevice.presentFamilyIndex()
	};
	_queueCreateInfos = std::make_unique<QueueCreateInfoArray>();

	// Because uniqueQueueFamilies is a set, 
	// if graphicFamily and presentFamily has same index, loop just create info structure once.
	// Otherwise create info structure twice for each family of queue.
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &_queuePriority;

		_queueCreateInfos->push_back(queueCreateInfo);
	}

	_deviceFeatures = std::make_unique<VkPhysicalDeviceFeatures>();
	_deviceFeatures->samplerAnisotropy = VK_TRUE;
	_deviceFeatures->sampleRateShading = VK_TRUE;
	_deviceFeatures->shaderResourceMinLod = VK_TRUE;

	info().sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info().queueCreateInfoCount = static_cast<uint32_t>(_queueCreateInfos->size());
	info().pQueueCreateInfos = _queueCreateInfos->data();
	info().pEnabledFeatures = _deviceFeatures.get();

	info().enabledExtensionCount = static_cast<uint32_t>(cfg::deviceExtensions.size());
	info().ppEnabledExtensionNames = cfg::deviceExtensions.data();

	if (cfg::enableValidationLayers) {
		info().enabledLayerCount = static_cast<uint32_t> (cfg::validationLayers.size());
		info().ppEnabledLayerNames = cfg::validationLayers.data();
	}
	else {
		info().enabledLayerCount = 0;
	}
}

void AfterglowDevice::create() {
	if (vkCreateDevice(_physicalDevice, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create logical device.");
	}

	// Release custom create info.
	_deviceFeatures.reset();
	_queueCreateInfos.reset();
}
