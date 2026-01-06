#pragma once
#include "AfterglowDevice.h"

// TODO: Fill class functions.
// TODO: Use in UniformBuffer and set Info.
class AfterglowDescriptorSetLayout : public AfterglowProxyObject<AfterglowDescriptorSetLayout, VkDescriptorSetLayout, VkDescriptorSetLayoutCreateInfo> {
public:
	using BindingArray = std::vector<VkDescriptorSetLayoutBinding>;

	AfterglowDescriptorSetLayout(AfterglowDevice& device);
	~AfterglowDescriptorSetLayout();

	AfterglowDevice& device() noexcept;

	// TODO: Replace as append uniform, append sampler...
	void appendBinding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t descriptorCount = 1);
	BindingArray& bindings();

	const BindingArray& bindings() const;

proxy_protected:
	void initCreateInfo();
	void create();

private:
	// TODO: many of  bindings.
	// std::unique_ptr<AfterglowDescriptorSetLayoutBindings> _bindings;
	BindingArray _bindings;

	AfterglowDevice& _device;
};

