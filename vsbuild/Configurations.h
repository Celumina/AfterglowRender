#pragma once
#include <vector>

#include "AfterglowUtilities.h"

namespace cfg {
	using Text = const char*;

	// Window Configurations
	constexpr static uint32_t windowWidth = 1280;
	constexpr static uint32_t windowHeight = 720;
	constexpr static Text windowTitle = "Afterglow";
	constexpr static bool windowResizable = true;


	// Vulkan Configurations
	// Environ settings
	constexpr static Text layerPath = "Libraries/VulkanSDK-1.3.290.0/bin";
	
	// Instance settings
	constexpr static Text applicationName = "AfterglowRender";
	constexpr static uint32_t applicationVersion = util::MakeVersion(1, 0, 0);
	constexpr static Text engineName = "AfterglowEngine";
	constexpr static uint32_t engineVersion = util::MakeVersion(1, 0, 0);
	constexpr static uint32_t apiVersion = util::MakeApiVersion(0, 1, 0, 0);

	// Validation settings
	static const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	// In realese mode, console will not output anything, so just turn it off.
	// TODO: Gathering these debug code options.
	#ifdef _DEBUG
	constexpr static bool enableValidationLayers = true;
	#else
	constexpr static bool enableValidationLayers = false;
	#endif

	// Device settings
	static const  std::vector<const char*> deviceExtensions = {
		// VK_KHR_SWAPCHAIN_EXTENSION_NAME
		"VK_KHR_swapchain", 
		// "VK_EXT_shader_stencil_export", 
	};

	// Semaphore settings
	constexpr static uint32_t maxFrameInFlight = 2;

	// This extent size remain for dynamic material.
	constexpr static uint32_t uniformDescriptorSize = 1024;
	constexpr static uint32_t samplerDescriptorSize = 512;
	constexpr static uint32_t descriptorSetSize = 1024;

	constexpr static Text shaderEntryName = "main";
	constexpr static Text shaderRootDirectory = "Shaders/";
}
