#include "AfterglowDescriptorSetLayout.h"

AfterglowDescriptorSetLayout::AfterglowDescriptorSetLayout(AfterglowDevice& device) : 
	_device(device) {
}

AfterglowDescriptorSetLayout::~AfterglowDescriptorSetLayout() {
	destroy(vkDestroyDescriptorSetLayout, _device, data(), nullptr);
}

AfterglowDevice& AfterglowDescriptorSetLayout::device() noexcept {
	return _device;
}

void AfterglowDescriptorSetLayout::appendBinding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t descriptorCount) {
	if (isDataExists()) {
		throw runtimeError("Can not append a binding due to the setLayout had been created.");
	}

	VkDescriptorSetLayoutBinding binding{};
	binding.binding = _bindings.size();
	binding.descriptorType = type;
	binding.stageFlags = stage;
	binding.descriptorCount = descriptorCount;
	_bindings.push_back(binding);

	info().bindingCount = _bindings.size();
	info().pBindings = _bindings.data();
}

AfterglowDescriptorSetLayout::BindingArray& AfterglowDescriptorSetLayout::bindings() {
	return _bindings;
}

const AfterglowDescriptorSetLayout::BindingArray& AfterglowDescriptorSetLayout::bindings() const {
	return _bindings;
}

void AfterglowDescriptorSetLayout::initCreateInfo() {	
	info().sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
}

void AfterglowDescriptorSetLayout::create() {
	if (vkCreateDescriptorSetLayout(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create descriptor set layout.");
	}
}
