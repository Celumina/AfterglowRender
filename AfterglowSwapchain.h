#pragma once 
#include "AfterglowImageView.h"
#include "AfterglowSynchronizer.h"

class AfterglowSwapchain : public AfterglowProxyObject<AfterglowSwapchain, VkSwapchainKHR, VkSwapchainCreateInfoKHR> {
public:
	using ImageArray = std::vector<VkImage>;

	AfterglowSwapchain(AfterglowDevice& device, AfterglowWindow& window, AfterglowSurface& surface);
	~AfterglowSwapchain();

	const VkFormat& imageFormat();
	const VkExtent2D& extent();
	float aspectRatio();

	const ImageArray& images();
	const AfterglowImageView::Array& imageViews();
	VkImage& image(uint32_t index);
	AfterglowImageView& imageView(uint32_t index);
	inline AfterglowDevice& device();

	void recreate();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	void initImageViews();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	AfterglowDevice& _device;
	AfterglowWindow& _window;
	AfterglowSurface& _surface;

	ImageArray _images;
	VkFormat _imageFormat;
	VkExtent2D _extent;

	AfterglowImageView::Array _imageViews;
};

