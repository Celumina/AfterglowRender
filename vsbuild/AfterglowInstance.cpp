#include "AfterglowInstance.h"

#include <iostream>
#include <GLFW/glfw3.h>

#include "Configurations.h"

AfterglowInstance::AfterglowInstance() {
	_extensions = getRequiredExtensions();
}

AfterglowInstance::~AfterglowInstance() {
	// Manully Created form vkCreate...(). 
	destroy(vkDestroyInstance, data(), nullptr);
}

void AfterglowInstance::initCreateInfo() {
	if constexpr(cfg::enableValidationLayers) {
		if (!checkValidationLayerSupport()) {
			throw runtimeError("Validation layers requested, but not available.");
		}
	}

	if (!_appInfo) {
		_appInfo = std::make_unique<VkApplicationInfo>();
	}
	_appInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	_appInfo->pApplicationName = cfg::applicationName;
	_appInfo->applicationVersion = cfg::applicationVersion;
	_appInfo->pEngineName = cfg::engineName;
	_appInfo->engineVersion = cfg::engineVersion;
	_appInfo->apiVersion = cfg::apiVersion;

	info().sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info().pApplicationInfo = _appInfo.get();

	info().enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
	info().ppEnabledExtensionNames = _extensions.data();

	info().enabledLayerCount = 0;


	if  constexpr (cfg::enableValidationLayers) {
		if (!_debugInfo) {
			_debugInfo = std::make_unique<VkDebugUtilsMessengerCreateInfoEXT>();
		}
		info().enabledLayerCount = static_cast<uint32_t>(cfg::validationLayers.size());
		info().ppEnabledLayerNames = cfg::validationLayers.data();

		populateDebugMessengerCreateInfo(*_debugInfo);
		info().pNext = (VkDebugUtilsMessengerCreateInfoEXT*)_debugInfo.get();
	}
	else {
		info().enabledLayerCount = 0;
		info().pNext = nullptr;
	}

}

void AfterglowInstance::create() {
	if (vkCreateInstance(&info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create vulkan instance.");
	}

	_appInfo.reset();
	_debugInfo.reset();
}

AfterglowInstance::ExtensionArray AfterglowInstance::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = nullptr;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Range constructor
	// glfwExtensions: begin of array
	// glfwExtensions + glfwExtensionCount: end of array, use pointer operation.
	ExtensionArray extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if constexpr (cfg::enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void AfterglowInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

bool AfterglowInstance::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : cfg::validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL AfterglowInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "[AfterglowInstance] [Validation Layer] " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}
