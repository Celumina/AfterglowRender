#pragma once
#include "AfterglowDeviceMemory.h"
#include "AfterglowImageView.h"
#include "AfterglowSampler.h"
#include "AssetDefinitions.h"
#include "AfterglowPhysicalDevice.h"

namespace img {
	struct WriteInfo {
		AfterglowSampler& sampler;
		AfterglowImageView& imageView;
	};

	using WriteInfoArray = std::vector<WriteInfo>;

	template<typename Type>
	concept ImageType = 
		requires { typename Type::Derived; } 
		&& std::is_base_of_v<AfterglowProxyObject<typename Type::Derived, VkImage, VkImageCreateInfo>, Type>;
}

template<typename DerivedType>
class AfterglowImage : public AfterglowProxyObject<DerivedType, VkImage, VkImageCreateInfo> {
public:
	using Parent = AfterglowProxyObject<DerivedType, VkImage, VkImageCreateInfo>;

	AfterglowImage(AfterglowDevice& device);
	~AfterglowImage();

	uint64_t size();
	AfterglowImageView& imageView();
	AfterglowSampler& sampler();

	// If InitMemory() was called, return true.
	bool wasInitialized();

proxy_base_protected(Parent):
	// Because we appointment a new BufferType, so implement a custom create function is necessary.
	void initCreateInfo();
	void create();

protected:
	void initMemory(VkMemoryPropertyFlags properties);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkImageMemoryBarrier makeBarrier(VkImageLayout oldLayout, VkImageLayout newLayout);
	inline VkFormat vulkanFormat(const img::Info& info);

	AfterglowDevice& _device;
	AfterglowDeviceMemory::AsElement _memory;
	AfterglowImageView::AsElement _imageView;
	// TODO: release this after created?
	img::Info _imageInfo;
	// TODO: Shared sampler.
	AfterglowSampler::AsElement _sampler;
};

template<typename DerivedType>
AfterglowImage<DerivedType>::AfterglowImage(AfterglowDevice& device) :
	_device(device),
	_imageInfo(),
	_imageView(AfterglowImageView::makeElement(device)),
	_sampler(AfterglowSampler::makeElement(device)) {
}

template<typename DerivedType>
AfterglowImage<DerivedType>::~AfterglowImage() {
	Parent::destroy(vkDestroyImage, _device, Parent::data(), nullptr);
}

template<typename DerivedType>
uint64_t AfterglowImage<DerivedType>::size() {
	return _imageInfo.size;
}

template<typename DerivedType>
AfterglowImageView& AfterglowImage<DerivedType>::imageView() {
	return _imageView;
}

template<typename DerivedType>
AfterglowSampler& AfterglowImage<DerivedType>::sampler() {
	return _sampler;
}

template<typename DerivedType>
inline bool AfterglowImage<DerivedType>::wasInitialized() {
	return _memory;
}

template<typename DerivedType>
void AfterglowImage<DerivedType>::initCreateInfo() {
	// Write there instead of constructor due to the createInfo depend on ImageInfo.
	Parent::info().sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	Parent::info().imageType = VK_IMAGE_TYPE_2D;

	// Derived class to specify extent width and height.
	Parent::info().extent.depth = 1;

	// TODO: mipmap count as parameter, and array count as derived class
	Parent::info().mipLevels = 1;
	Parent::info().arrayLayers = 1;
	// TODO: SRGB Linear Supprot
	Parent::info().format = vulkanFormat(_imageInfo);

	//  VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
	//  VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access
	// using VK_IMAGE_TILING_OPTIMAL for efficient access from the shader.
	Parent::info().tiling = VK_IMAGE_TILING_OPTIMAL;

	// Set a specified layout for GPU performance optimization. Inside the class we assign a default to it. Modifiy this attribute out of class if needed.
	Parent::info().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	Parent::info().usage = 
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	Parent::info().sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	Parent::info().samples = VK_SAMPLE_COUNT_1_BIT;
	Parent::info().flags = 0;
}

template<typename DerivedType>
void AfterglowImage<DerivedType>::create() {
	if (vkCreateImage(_device, &Parent::info(), nullptr, &Parent::data()) != VK_SUCCESS) {
		throw Parent::runtimeError("Failed to create image.");
	}

	// Fill ImageView for this Image.
	_imageView->image = Parent::data();
	_imageView->format = Parent::info().format;
}

template<typename DerivedType>
void AfterglowImage<DerivedType>::initMemory(VkMemoryPropertyFlags properties) {
	VkMemoryRequirements memoryRequirements;
	// Remind that Parent::data() could not create automatically, so we use *this.
	vkGetImageMemoryRequirements(_device, *this, &memoryRequirements);
	_memory = AfterglowDeviceMemory::makeElement(_device);
	_memory->allocationSize = memoryRequirements.size;
	_memory->memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);
	// Bind image with memory.
	vkBindImageMemory(_device, Parent::data(), _memory, 0);
}

template<typename DerivedType>
uint32_t AfterglowImage<DerivedType>::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(_device.physicalDevice(), &memoryProperties);
	for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
		if ((typeFilter & (1 << index)) && (memoryProperties.memoryTypes[index].propertyFlags & properties)) {
			return index;
		}
	}
	throw Parent::runtimeError("Failed to find suitable memory type.");
	return -1;
}

template<typename DerivedType>
VkImageMemoryBarrier AfterglowImage<DerivedType>::makeBarrier(VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkImageMemoryBarrier barrier{};

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = *this;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	return barrier;
}

template<typename DerivedType>
inline VkFormat AfterglowImage<DerivedType>::vulkanFormat(const img::Info& info) {
	using Format = img::Format;
	if (info.colorSpace == img::ColorSpace::Linear) {
		switch (info.format) {
		case Format::Grey:
			return VK_FORMAT_R8_UNORM;
		case Format::GreyAlpha:
			return VK_FORMAT_R8G8_UNORM;
		case Format::RGB:
			return VK_FORMAT_R8G8B8_UNORM;
		case Format::RGBA:
			return VK_FORMAT_R8G8B8A8_UNORM;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}
	else if (info.colorSpace == img::ColorSpace::SRGB) {
		switch (info.format) {
		case Format::Grey:
			return VK_FORMAT_R8_SRGB;
		case Format::GreyAlpha:
			return VK_FORMAT_R8G8_SRGB;
		case Format::RGB:
			return VK_FORMAT_R8G8B8_SRGB;
		case Format::RGBA:
			return VK_FORMAT_R8G8B8A8_SRGB;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}
	return VK_FORMAT_UNDEFINED;
}
