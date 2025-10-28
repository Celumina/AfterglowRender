#pragma once
#include <optional>

#include "AfterglowProxyObject.h"

class AfterglowInstance;
class AfterglowSurface;

class AfterglowPhysicalDevice : public AfterglowProxyObject<AfterglowPhysicalDevice, VkPhysicalDevice> {
	AFTERGLOW_PROXY_STORAGE_ONLY
public:
	struct QueueFamilyIndices {
		std::optional<uint32_t>  graphicFamily;
		std::optional<uint32_t>  presentFamily;
		bool isComplete() const;
	};

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	AfterglowPhysicalDevice(AfterglowInstance& instance, AfterglowSurface& surface);
	~AfterglowPhysicalDevice();

	uint32_t graphicsFamilyIndex();
	uint32_t presentFamilyIndex();

	// Swapchain supprot.
	SwapchainSupportDetails querySwapchainSupport(AfterglowSurface& surface);

	// Physical device properties.
	const VkPhysicalDeviceProperties& properties() const noexcept;
	VkFormatProperties formatProperties(VkFormat format);

	// Texture supports.
	VkFormat findSupportedFormat(const std::vector<VkFormat>& cadidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	VkSampleCountFlagBits msaaSampleCount();

private:
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, AfterglowSurface& surface);
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, AfterglowSurface& surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device, AfterglowSurface& surface);
	int evaluateDeviceSuitablility(VkPhysicalDevice device, AfterglowSurface& surface);
	VkSampleCountFlagBits getMaxUsableSamleCount();

	QueueFamilyIndices _queueFamilyIndices;
	VkSampleCountFlagBits _msaaSampleCount;
	VkPhysicalDeviceProperties _properties;
};

