#pragma once
#include "AfterglowProxyObject.h"
class AfterglowDebugMessenger : public AfterglowProxyObject<AfterglowDebugMessenger, VkDebugUtilsMessengerEXT, VkDebugUtilsMessengerCreateInfoEXT> {
public:
	AfterglowDebugMessenger(VkInstance& instance);
	~AfterglowDebugMessenger();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkInstance& _instance;
};

