#include "AfterglowSwapchain.h"

#include <algorithm>
#include "AfterglowFramebufferManager.h"
#include "AfterglowWindow.h"
#include "AfterglowSurface.h"
#include "AfterglowPhysicalDevice.h"


AfterglowSwapchain::AfterglowSwapchain(AfterglowDevice& device, AfterglowWindow& window, AfterglowSurface& surface) :
	_device(device), _window(window), _surface(surface), _imageFormat(VkFormat()), _extent(VkExtent2D()) {
	initialize();
	initImageViews();
}

AfterglowSwapchain::~AfterglowSwapchain() {
	destroy(vkDestroySwapchainKHR, _device, data(), nullptr);
}

const VkFormat& AfterglowSwapchain::imageFormat() {
	return _imageFormat;
}

const VkExtent2D& AfterglowSwapchain::extent() {
	return _extent;
}

float AfterglowSwapchain::aspectRatio() {
	if (_extent.width == 0 || _extent.height == 0) {
		return 1.0;
	}
	return static_cast<float>(_extent.width) / _extent.height;
}

const AfterglowSwapchain::ImageArray& AfterglowSwapchain::images() {
	return _images;
}

const AfterglowImageView::Array& AfterglowSwapchain::imageViews() {
	return _imageViews;
}

VkImage& AfterglowSwapchain::image(uint32_t index) {
	return _images[index];
}

AfterglowImageView& AfterglowSwapchain::imageView(uint32_t index) {
	return _imageViews[index];
}

inline AfterglowDevice& AfterglowSwapchain::device() noexcept {
	return _device;
}

void AfterglowSwapchain::recreate() {
	destroy(vkDestroySwapchainKHR, _device, *this, nullptr);
	initialize();
	initImageViews();
}

void AfterglowSwapchain::initCreateInfo() {
	auto swapChainSupport = _device.physicalDevice().querySwapchainSupport(_surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	// Simply sticking to this minimum means that we may sometimes have
	// to wait on the driver to complete internal operations before we can acquire
	//	another image to render to.Therefore it is recommended to request at least one
	//	more image than the minimum.
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	// Make sure minImageCount + 1 not exceed the maxImageCount, 
	// and consider if maxImageCout is no-limit (value == 0).
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	info().sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info().surface = _surface;
	info().minImageCount = imageCount;
	info().imageFormat = surfaceFormat.format;
	info().imageColorSpace = surfaceFormat.colorSpace;
	info().imageExtent = extent;
	info().imageArrayLayers = 1;
	info().imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {
		_device.physicalDevice().graphicsFamilyIndex(),
		_device.physicalDevice().presentFamilyIndex()
	};

	if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
		info().imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info().queueFamilyIndexCount = 2;
		info().pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		info().imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info().queueFamilyIndexCount = 0;
		info().pQueueFamilyIndices = nullptr;
	}

	info().preTransform = swapChainSupport.capabilities.currentTransform;
	info().compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	info().presentMode = presentMode;
	info().clipped = VK_TRUE;

	info().oldSwapchain = VK_NULL_HANDLE;

	_imageFormat = surfaceFormat.format;
	_extent = extent;
}

void AfterglowSwapchain::create() {
	if (vkCreateSwapchainKHR(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create swap chain.");
	}

	// Retrieve swapchain image form swap chain
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(_device, data(), &imageCount, nullptr);
	_images.resize(imageCount);
	vkGetSwapchainImagesKHR(_device, data(), &imageCount, _images.data());
}

void AfterglowSwapchain::initImageViews() {
	// ImageViews
	_imageViews.clear();
	for (size_t index = 0; index < images().size(); ++index) {
		_imageViews.push_back(AfterglowImageView::makeElement(device()));
		auto& imageView = _imageViews.back();

		imageView->image = images()[index];
		imageView->format = imageFormat();

		imageView->components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageView->components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageView->components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageView->components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	}
}

VkSurfaceFormatKHR AfterglowSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		// TODO: HDR Surface format support
		// @note: Due to the GUI problem, Gamma correct (EOTF) here instead of LUT.
		if (availableFormat.format == /*VK_FORMAT_B8G8R8A8_UNORM*/ VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == /*VK_COLOR_SPACE_PASS_THROUGH_EXT*/ VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	// if not found, return the first.
	return availableFormats[0];
}

VkPresentModeKHR AfterglowSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		// VK_PRESENT_MODE_MAILBOX_KHR: Triple Buffering
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	// The first choose is Triple Buffering, if GPU not support, then choose Vertical Blank.
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D AfterglowSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = _window.framebufferSize();

		actualExtent.width = std::clamp(
			actualExtent.width,
			capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width
		);
		actualExtent.height = std::clamp(
			actualExtent.height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height
		);
		return actualExtent;
	}
}
