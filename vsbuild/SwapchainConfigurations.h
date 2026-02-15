#pragma once
#include "vulkan/vulkan_core.h"

namespace swapchain{
	// TODO: HDR Surface format support
	// @note: Due to the GUI problem, Gamma correct (EOTF) here instead of LUT.
	constexpr VkFormat presentFormat = /*VK_FORMAT_B8G8R8A8_UNORM*/ VK_FORMAT_B8G8R8A8_SRGB;
	constexpr VkColorSpaceKHR presentColorSpace = /*VK_COLOR_SPACE_PASS_THROUGH_EXT*/ VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}