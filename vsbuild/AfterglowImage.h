#pragma once
#include "AfterglowDeviceMemory.h"
#include "AfterglowImageView.h"
#include "AfterglowSampler.h"
#include "AssetDefinitions.h"
#include "AfterglowPhysicalDevice.h"

template<typename DerivedType, bool useUniqueSampler = true>
class AfterglowImage : public AfterglowProxyObject<DerivedType, VkImage, VkImageCreateInfo> {
public:
	using Parent = AfterglowProxyObject<DerivedType, VkImage, VkImageCreateInfo>;
	static constexpr bool hasUniqueSampler = useUniqueSampler;

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
	AfterglowSampler::AsElement _sampler;
};

namespace img {
	struct ImageReference {
		AfterglowSampler& sampler;
		AfterglowImageView& imageView;
		VkImage image;
	};

	using ImageReferences = std::vector<ImageReference>;

	template<typename Type>
	concept ImageType = std::is_base_of_v<AfterglowImage<Type>, Type>;

	// @note: construct a ImageReference will initialize the source image.
	template<ImageType Type>
	ImageReference MakeImageReference(Type& image) {
		return ImageReference{ 
			.sampler = image.sampler(), 
			.imageView = image.imageView(), 
			.image = image
		};
	};
}

template<typename DerivedType, bool useUniqueSampler>
AfterglowImage<DerivedType, useUniqueSampler>::AfterglowImage(AfterglowDevice& device) :
	_device(device),
	_imageInfo(),
	_imageView(AfterglowImageView::makeElement(device)) {
	if constexpr (useUniqueSampler) {
		_sampler.recreate(device);
	}
}

template<typename DerivedType, bool useUniqueSampler>
AfterglowImage<DerivedType, useUniqueSampler>::~AfterglowImage() {
	Parent::destroy(vkDestroyImage, _device, Parent::data(), nullptr);
}

template<typename DerivedType, bool useUniqueSampler>
uint64_t AfterglowImage<DerivedType, useUniqueSampler>::size() {
	return _imageInfo.size;
}

template<typename DerivedType, bool useUniqueSampler>
AfterglowImageView& AfterglowImage<DerivedType, useUniqueSampler>::imageView() {
	return _imageView;
}

template<typename DerivedType, bool useUniqueSampler>
AfterglowSampler& AfterglowImage<DerivedType, useUniqueSampler>::sampler() {
	if constexpr (useUniqueSampler) {
		return _sampler;
	}
	else {
		static_assert(useUniqueSampler, "This instantiated class is not enable the useUniqueSampler flag.");
	}
}

template<typename DerivedType, bool useUniqueSampler>
inline bool AfterglowImage<DerivedType, useUniqueSampler>::wasInitialized() {
	return _memory;
}

template<typename DerivedType, bool useUniqueSampler>
void AfterglowImage<DerivedType, useUniqueSampler>::initCreateInfo() {
	// Write there instead of constructor due to the createInfo depend on ImageInfo.
	Parent::info().sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	Parent::info().imageType = VK_IMAGE_TYPE_2D;

	// Derived class to specify extent width and height.
	Parent::info().extent.depth = 1;

	// TODO: mipmap count as parameter, and array count as derived class
	Parent::info().mipLevels = 1;
	Parent::info().arrayLayers = 1;
	
	// Parent::info().format = vulkanFormat(_imageInfo);
	// Set in derived class manually
	Parent::info().format = VK_FORMAT_UNDEFINED;

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

template<typename DerivedType, bool useUniqueSampler>
void AfterglowImage<DerivedType, useUniqueSampler>::create() {
	if (vkCreateImage(_device, &Parent::info(), nullptr, &Parent::data()) != VK_SUCCESS) {
		throw Parent::runtimeError("Failed to create image.");
	}

	// Fill ImageView for this Image.
	_imageView->image = Parent::data();
	_imageView->format = Parent::info().format;
}

template<typename DerivedType, bool useUniqueSampler>
void AfterglowImage<DerivedType, useUniqueSampler>::initMemory(VkMemoryPropertyFlags properties) {
	VkMemoryRequirements memoryRequirements;
	// Remind that Parent::data() could not create automatically, so we use *this.
	vkGetImageMemoryRequirements(_device, *this, &memoryRequirements);
	_memory = AfterglowDeviceMemory::makeElement(_device);
	_memory->allocationSize = memoryRequirements.size;
	_memory->memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);
	// Bind image with memory.
	vkBindImageMemory(_device, Parent::data(), _memory, 0);
}

template<typename DerivedType, bool useUniqueSampler>
uint32_t AfterglowImage<DerivedType, useUniqueSampler>::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

template<typename DerivedType, bool useUniqueSampler>
VkImageMemoryBarrier AfterglowImage<DerivedType, useUniqueSampler>::makeBarrier(VkImageLayout oldLayout, VkImageLayout newLayout) {
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

template<typename DerivedType, bool useUniqueSampler>
inline VkFormat AfterglowImage<DerivedType, useUniqueSampler>::vulkanFormat(const img::Info& info) {
	if (info.colorSpace == img::ColorSpace::Linear) {
		// TODO: For interpolation, all format should be floating point.
		// TODO: Now cast in 8bits data only, due to the intention of higher precision int is unknown.
		switch (info.channels) {
		case img::Channel::R:
			switch (info.format) {
			case img::Format::UnsignedInt8: return VK_FORMAT_R8_UNORM;
			case img::Format::Int8: return VK_FORMAT_R8_SNORM;
			case img::Format::UnsignedInt16: return VK_FORMAT_R16_UNORM;
			case img::Format::Int16: return VK_FORMAT_R16_SNORM;
			case img::Format::Half: return VK_FORMAT_R16_SFLOAT;
			case img::Format::UnsignedInt32: return VK_FORMAT_R32_UINT;
			case img::Format::Int32: return VK_FORMAT_R32_SINT;
			case img::Format::Float: return VK_FORMAT_R32_SFLOAT;
			case img::Format::UnsignedInt64: return VK_FORMAT_R64_UINT;
			case img::Format::Int64: return VK_FORMAT_R64_SINT;
			case img::Format::Double: return VK_FORMAT_R64_SFLOAT;
			default:
				DEBUG_CLASS_ERROR("Unknown image format.");
				return VK_FORMAT_UNDEFINED;
			}
		case img::Channel::RG:
			switch (info.format) {
			case img::Format::UnsignedInt8: return VK_FORMAT_R8G8_UNORM;
			case img::Format::Int8: return VK_FORMAT_R8G8_SNORM;
			case img::Format::UnsignedInt16: return VK_FORMAT_R16G16_UNORM;
			case img::Format::Int16: return VK_FORMAT_R16G16_SNORM;
			case img::Format::Half: return VK_FORMAT_R16G16_SFLOAT;
			case img::Format::UnsignedInt32: return VK_FORMAT_R32G32_UINT;
			case img::Format::Int32: return VK_FORMAT_R32G32_SINT;
			case img::Format::Float: return VK_FORMAT_R32G32_SFLOAT;
			case img::Format::UnsignedInt64: return VK_FORMAT_R64G64_UINT;
			case img::Format::Int64: return VK_FORMAT_R64G64_SINT;
			case img::Format::Double: return VK_FORMAT_R64G64_SFLOAT;
			default:
				DEBUG_CLASS_ERROR("Unknown image format.");
				return VK_FORMAT_UNDEFINED;
			}
		case img::Channel::RGB:
			switch (info.format) {
			case img::Format::UnsignedInt8: return VK_FORMAT_R8G8B8_UNORM;
			case img::Format::Int8: return VK_FORMAT_R8G8B8_SNORM;
			case img::Format::UnsignedInt16: return VK_FORMAT_R16G16B16_UNORM;
			case img::Format::Int16: return VK_FORMAT_R16G16B16_SNORM;
			case img::Format::Half: return VK_FORMAT_R16G16B16_SFLOAT;
			case img::Format::UnsignedInt32: return VK_FORMAT_R32G32B32_UINT;
			case img::Format::Int32: return VK_FORMAT_R32G32B32_SINT;
			case img::Format::Float: return VK_FORMAT_R32G32B32_SFLOAT;
			case img::Format::UnsignedInt64: return VK_FORMAT_R64G64B64_UINT;
			case img::Format::Int64: return VK_FORMAT_R64G64B64_SINT;
			case img::Format::Double: return VK_FORMAT_R64G64B64_SFLOAT;
			default:
				DEBUG_CLASS_ERROR("Unknown image format.");
				return VK_FORMAT_UNDEFINED;
			}
		case img::Channel::RGBA:
			switch (info.format) {
			case img::Format::UnsignedInt8: return VK_FORMAT_R8G8B8A8_UNORM;
			case img::Format::Int8: return VK_FORMAT_R8G8B8A8_SNORM;
			case img::Format::UnsignedInt16: return VK_FORMAT_R16G16B16A16_UNORM;
			case img::Format::Int16: return VK_FORMAT_R16G16B16A16_SNORM;
			case img::Format::Half: return VK_FORMAT_R16G16B16A16_SFLOAT;
			case img::Format::UnsignedInt32: return VK_FORMAT_R32G32B32A32_UINT;
			case img::Format::Int32: return VK_FORMAT_R32G32B32A32_SINT;
			case img::Format::Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case img::Format::UnsignedInt64: return VK_FORMAT_R64G64B64A64_UINT;
			case img::Format::Int64: return VK_FORMAT_R64G64B64A64_SINT;
			case img::Format::Double: return VK_FORMAT_R64G64B64A64_SFLOAT;
			default:
				DEBUG_CLASS_ERROR("Unknown image format.");
				return VK_FORMAT_UNDEFINED;
			}
		default:
			DEBUG_CLASS_ERROR("Invalid numChannels.");
			return VK_FORMAT_UNDEFINED;
		}
	}
	else if (info.colorSpace == img::ColorSpace::SRGB) {
		switch (info.format) {
		case img::Format::UnsignedInt8:
			switch (info.channels) {
			case img::Channel::R:	return VK_FORMAT_R8_SRGB;
			case img::Channel::RG:	return VK_FORMAT_R8G8_SRGB;
			case img::Channel::RGB:	return VK_FORMAT_R8G8B8_SRGB;
			case img::Channel::RGBA:	return VK_FORMAT_R8G8B8A8_SRGB;
			default:
				DEBUG_CLASS_ERROR("Invalid numChannels.");
				return VK_FORMAT_UNDEFINED;

			}
		default:
			DEBUG_CLASS_ERROR("Unsupported image format for srgb color space.");
			return VK_FORMAT_UNDEFINED;
		}
	
	}
	DEBUG_CLASS_ERROR("Unknown image color space.");
	return VK_FORMAT_UNDEFINED;
}
