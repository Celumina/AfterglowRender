#include "AfterglowDescriptorSetWriter.h"
#include "Configurations.h"

AfterglowDescriptorSetWriter::AfterglowDescriptorSetWriter(AfterglowDevice& device) :
	_device(device) {
}

AfterglowDevice& AfterglowDescriptorSetWriter::device() noexcept {
	return _device;
}

void AfterglowDescriptorSetWriter::registerImage(
	AfterglowSampler& sampler, 
	AfterglowImageView& imageView, 
	const AfterglowDescriptorSetLayout& setLayout, 
	const VkDescriptorSet& set, 
	uint32_t bindingIndex, 
	VkImageLayout imageLayout) {
	_writeDescriptorInfos.imageWriteContexts.emplace_back(
		VkDescriptorImageInfo{
			.sampler = sampler,
			.imageView = imageView,
			.imageLayout = imageLayout
		},
		setLayout,
		set,
		bindingIndex
	);
}

void AfterglowDescriptorSetWriter::write() {
	createDescriptorWrites();
	vkUpdateDescriptorSets(
		_device, 
		static_cast<uint32_t>(_writeDescriptorInfos.writes.size()), 
		_writeDescriptorInfos.writes.data(), 
		0, 
		nullptr
	);
	_writeDescriptorInfos.writes.clear();
	_writeDescriptorInfos.imageWriteContexts.clear();
	_writeDescriptorInfos.bufferWriteContexts.clear();
}

void AfterglowDescriptorSetWriter::createDescriptorWrites() {
	for (const auto& bufferWriteContext : _writeDescriptorInfos.bufferWriteContexts) {
		if (bufferWriteContext.bindingIndex >= bufferWriteContext.setLayout.bindings().size()) {
			//DEBUG_INFO("_____________________BUFIDX" + std::to_string(bufferWriteContext.bindingIndex));
			//DEBUG_INFO("_____________________BNDSZE" + std::to_string(bufferWriteContext.setLayout.bindings().size()));
			DEBUG_CLASS_WARNING("bufferWriteContext.bindingIndex out of range.");
			continue;
		}
		auto& binding = bufferWriteContext.setLayout.bindings()[bufferWriteContext.bindingIndex];
		_writeDescriptorInfos.writes.emplace_back(
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = bufferWriteContext.set,
				.dstBinding = bufferWriteContext.bindingIndex,
				.dstArrayElement = 0,
				.descriptorCount = binding.descriptorCount,
				.descriptorType = binding.descriptorType,
				.pBufferInfo = &bufferWriteContext.info
			}
		);
	}
	for (const auto& imageWriteContext : _writeDescriptorInfos.imageWriteContexts) {
		if (imageWriteContext.bindingIndex >= imageWriteContext.setLayout.bindings().size()) {
			DEBUG_CLASS_WARNING("imageWriteContext.bindingIndex out of range.");
			continue;
		}
		auto& binding = imageWriteContext.setLayout.bindings()[imageWriteContext.bindingIndex];
		_writeDescriptorInfos.writes.emplace_back(
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = imageWriteContext.set,
				.dstBinding = imageWriteContext.bindingIndex,
				.dstArrayElement = 0,
				.descriptorCount = binding.descriptorCount,
				.descriptorType = binding.descriptorType,
				.pImageInfo = &imageWriteContext.info,
				.pTexelBufferView = nullptr
			}
		);
	}
}
