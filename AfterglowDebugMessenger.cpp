#include "AfterglowDebugMessenger.h"
#include <iostream>

AfterglowDebugMessenger::AfterglowDebugMessenger(VkInstance& instance) : 
	_instance(instance) {
}

AfterglowDebugMessenger::~AfterglowDebugMessenger() {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		_instance,
		"vkDestroyDebugUtilsMessengerEXT"
	);
	if (func != nullptr) {
		func(_instance, data(), nullptr);
	}
}

void AfterglowDebugMessenger::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info().messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info().messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	info().pfnUserCallback = debugCallback;
	info().pUserData = nullptr;
}

void AfterglowDebugMessenger::create() {
	// CreateDebugUtilsMessengerEXT
	// Proxy function, because vulkan can not create a extension object straigthforward.
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		_instance,
		"vkCreateDebugUtilsMessengerEXT"
	);

	VkResult result = VK_RESULT_MAX_ENUM;
	if (func != nullptr) {
		result = func(_instance, &info(), nullptr, &data());
	}
	else {
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if (result != VK_SUCCESS) {
		throw runtimeError("Failed to set up debug messenger");
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL AfterglowDebugMessenger::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "[AfterglowDebugMessenger] [Validation layer] " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}
