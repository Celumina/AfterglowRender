#pragma once
#include "AfterglowDescriptorSetLayout.h"
#include "AfterglowDescriptorSets.h"
#include "AfterglowImage.h"
#include "AfterglowUniformBuffer.h"

class AfterglowDescriptorSetWriter : public AfterglowObject {
public: 
	template<typename InfoType>
	struct WriteContext {
		InfoType info;
		const AfterglowDescriptorSetLayout& setLayout;
		const VkDescriptorSet& set;
		uint32_t bindingIndex;
	};

	using BufferWriteContexts = std::vector<WriteContext<VkDescriptorBufferInfo>>;
	using ImageWriteContexts = std::vector<WriteContext<VkDescriptorImageInfo>>;
	using WriteDescriptorSets = std::vector<VkWriteDescriptorSet>;

	struct WriteDescriptorInfo {
		BufferWriteContexts bufferWriteContexts;
		ImageWriteContexts imageWriteContexts;
		WriteDescriptorSets writes;
	};

	AfterglowDescriptorSetWriter(AfterglowDevice& device);

	AfterglowDevice& device() noexcept;

	template<buffer::BufferType Type>
	void registerBuffer(
		Type& buffer,
		const AfterglowDescriptorSetLayout& setLayout, 
		const VkDescriptorSet& set, 
		uint32_t bindingIndex
	);

	template<img::ImageType Type>
	void registerImage(
		Type& image,
		const AfterglowDescriptorSetLayout& setLayout,
		const VkDescriptorSet& set,
		uint32_t bindingIndex, 
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	void registerImage(
		AfterglowSampler& sampler, 
		AfterglowImageView& imageView, 
		const AfterglowDescriptorSetLayout& setLayout,
		const VkDescriptorSet& set,
		uint32_t bindingIndex, 
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	void write();

private:
	void createDescriptorWrites();

	AfterglowDevice& _device;
	// Write every tick.
	WriteDescriptorInfo _writeDescriptorInfos;
};

template<buffer::BufferType Type>
void AfterglowDescriptorSetWriter::registerBuffer(
	Type& buffer,
	const AfterglowDescriptorSetLayout& setLayout,
	const VkDescriptorSet& set,
	uint32_t bindingIndex) {
	// C++ 20 Features.
	_writeDescriptorInfos.bufferWriteContexts.emplace_back(
		VkDescriptorBufferInfo{
			.buffer = buffer,
			.offset = 0,
			.range = buffer.byteSize()
		},
		setLayout,
		set,
		bindingIndex
	);
}

template<img::ImageType Type>
void AfterglowDescriptorSetWriter::registerImage(
	Type& image,
	const AfterglowDescriptorSetLayout& setLayout,
	const VkDescriptorSet& set,
	uint32_t bindingIndex, 
	VkImageLayout imageLayout) {
	
	VkSampler sampler = nullptr;
	if constexpr (Type::hasUniqueSampler) {
		sampler = image.sampler();
	}

	_writeDescriptorInfos.imageWriteContexts.emplace_back(
		VkDescriptorImageInfo{
			.sampler = sampler,
			.imageView = image.imageView(),
			.imageLayout = imageLayout
		},
		setLayout,
		set,
		bindingIndex
	);
}