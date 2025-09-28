#pragma once
#include <vector>

#include "AfterglowProxyObject.h"

class AfterglowInstance : public AfterglowProxyObject<AfterglowInstance, VkInstance, VkInstanceCreateInfo> {
public: 
	// Here const char* is safe because extension string is always constance.
	using ExtensionArray = std::vector<const char*>;

	AfterglowInstance();
	~AfterglowInstance();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	ExtensionArray getRequiredExtensions();
	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static bool checkValidationLayerSupport();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageTypes, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData
	);

	ExtensionArray _extensions;
	std::unique_ptr<VkApplicationInfo> _appInfo = nullptr;
	std::unique_ptr<VkDebugUtilsMessengerCreateInfoEXT> _debugInfo = nullptr;
};

