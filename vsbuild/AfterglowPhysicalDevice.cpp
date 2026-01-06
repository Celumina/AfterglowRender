#include "AfterglowPhysicalDevice.h"
#include <map>
#include <set>

#include "AfterglowInstance.h"
#include "AfterglowSurface.h"
#include "Configurations.h"
#include "RenderConfigurations.h"


AfterglowPhysicalDevice::AfterglowPhysicalDevice(AfterglowInstance& instance, AfterglowSurface& surface) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw runtimeError("Failed to find GPUs with Vulkan support.");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : devices) {
		candidates.insert(std::make_pair(evaluateDeviceSuitablility(device, surface), device));
	}

	if (candidates.rbegin()->first > 0) {
		linkData(candidates.rbegin()->second);
		_queueFamilyIndices = findQueueFamilies(data(), surface);
	}
	else {
		throw runtimeError("Failed to find a suitable GPU.");
	}

	// After physical device linked, get its properties and features.
	vkGetPhysicalDeviceProperties(*this, &_properties);
	vkGetPhysicalDeviceFeatures(*this, &_features);
	_msaaSampleCount = cfg::enableMSAA ? getMaxUsableSamleCount() : VK_SAMPLE_COUNT_1_BIT;
}

AfterglowPhysicalDevice::~AfterglowPhysicalDevice() {
	// Reset dataptr only.
	destroy();
}

uint32_t AfterglowPhysicalDevice::graphicsFamilyIndex() {
	return _queueFamilyIndices.graphicFamily.value();
}

uint32_t AfterglowPhysicalDevice::presentFamilyIndex() {
	return _queueFamilyIndices.presentFamily.value();
}

AfterglowPhysicalDevice::SwapchainSupportDetails AfterglowPhysicalDevice::querySwapchainSupport(AfterglowSurface& surface) {
	return querySwapchainSupport(data(), surface);
}

const VkPhysicalDeviceProperties& AfterglowPhysicalDevice::properties() const noexcept {
	return _properties;
}

const VkPhysicalDeviceFeatures& AfterglowPhysicalDevice::features() const noexcept {
	return _features;
}

VkFormatProperties AfterglowPhysicalDevice::formatProperties(VkFormat format) {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(*this, format, &formatProperties);
	return formatProperties;
}

VkFormat AfterglowPhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& cadidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : cadidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(*this, format, &properties);
		// properties.linearTilingFeatures & features: if contains, gets features flag.
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw runtimeError("Failed to find supproted format.");
}

VkFormat AfterglowPhysicalDevice::findDepthFormat() {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkSampleCountFlagBits AfterglowPhysicalDevice::msaaSampleCount() noexcept {
	return _msaaSampleCount;
}

AfterglowPhysicalDevice::QueueFamilyIndices AfterglowPhysicalDevice::findQueueFamilies(VkPhysicalDevice device, AfterglowSurface& surface) {
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int index = 0;
	for (const auto& queueFamily : queueFamilies) {
		// Both support compute queue and graphic queue.
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			&& (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) { 
			indices.graphicFamily = index;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = index;
		}

		if (indices.isValid()) {
			break;
		}
		++index;
	}
	return indices;
}

AfterglowPhysicalDevice::SwapchainSupportDetails AfterglowPhysicalDevice::querySwapchainSupport(VkPhysicalDevice device, AfterglowSurface& surface) {
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool AfterglowPhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(cfg::deviceExtensions.begin(), cfg::deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	// If requiredExtensions is empty, meaning that all required extensions is supported.
	return requiredExtensions.empty();
}

bool AfterglowPhysicalDevice::isDeviceSuitable(VkPhysicalDevice device, AfterglowSurface& surface) {
	// No filter now, we just choose the first suitable one.
	QueueFamilyIndices indices = findQueueFamilies(device, surface);
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapchainSupportDetails swapChainSupport = querySwapchainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isValid() 
		&& extensionsSupported 
		&& swapChainAdequate 
		&& supportedFeatures.samplerAnisotropy;
}

int AfterglowPhysicalDevice::evaluateDeviceSuitablility(VkPhysicalDevice device, AfterglowSurface& surface) {
	// Just a simple example of GPU Suitablility Evaluation.
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Geometry shaders support
	if (deviceFeatures.geometryShader) {
		score += 1000;
	}

	if (!isDeviceSuitable(device, surface)) {
		return 0;
	}

	return score;
}

VkSampleCountFlagBits AfterglowPhysicalDevice::getMaxUsableSamleCount() {
	auto counts = properties().limits.framebufferColorSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) {
		return VK_SAMPLE_COUNT_64_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_32_BIT) {
		return VK_SAMPLE_COUNT_32_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_16_BIT) {
		return VK_SAMPLE_COUNT_16_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_8_BIT) {
		return VK_SAMPLE_COUNT_8_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_4_BIT) {
		return VK_SAMPLE_COUNT_4_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_2_BIT) {
		return VK_SAMPLE_COUNT_2_BIT;
	}
	return VK_SAMPLE_COUNT_1_BIT;
}

bool AfterglowPhysicalDevice::QueueFamilyIndices::isValid() const noexcept {
	return graphicFamily.has_value() && presentFamily.has_value();
}
