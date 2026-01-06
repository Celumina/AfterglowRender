#include "AfterglowDescriptorPool.h"

#include "Configurations.h"

AfterglowDescriptorPool::AfterglowDescriptorPool(AfterglowDevice& device) :
	_device(device) {
}

AfterglowDescriptorPool::~AfterglowDescriptorPool() {
	destroy(vkDestroyDescriptorPool, _device, data(), nullptr);
}

AfterglowDevice& AfterglowDescriptorPool::device() noexcept {
	return _device;
}

bool AfterglowDescriptorPool::isCreated() {
	return isDataExists();
}

void AfterglowDescriptorPool::extendPoolSize(AfterglowDescriptorSetLayout& layout) {
	if (isDataExists()) {
		throw runtimeError("Can not extend pool size because pool has been created.");
	}
	uint64_t oldPoolSizeCount = _poolSizes.size();
	_poolSizes.resize(oldPoolSizeCount + layout.bindings().size());
	for (uint32_t index = 0; index < layout.bindings().size(); ++index) {
		auto& binding = layout.bindings()[index];
		_poolSizes[oldPoolSizeCount + index].type = binding.descriptorType;
		_poolSizes[oldPoolSizeCount + index].descriptorCount = cfg::maxFrameInFlight * binding.descriptorCount;
	}
}

void AfterglowDescriptorPool::extendUniformPoolSize(uint32_t descriptorCount) {
	if (isDataExists()) {
		throw runtimeError("Can not extend pool size because pool has been created.");
	}
	_poolSizes.emplace_back(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount
	);
}

void AfterglowDescriptorPool::extendImageSamplerPoolSize(uint32_t descriptorCount) {
	if (isDataExists()) {
		throw runtimeError("Can not extend pool size because pool has been created.");
	}
	_poolSizes.emplace_back(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount
	);
}

void AfterglowDescriptorPool::setMaxDescritporSets(uint32_t maxSets) {
	if (isDataExists()) {
		throw runtimeError("Can not set descriptor count due to pool has been created.");
	}
	info().maxSets = maxSets;
}

void AfterglowDescriptorPool::allocateDescriptorSets(VkDescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets) {
	if (vkAllocateDescriptorSets(_device, &allocateInfo, pDescriptorSets) != VK_SUCCESS) {
		throw runtimeError("Failed to allocate descriptor sets.");
	}
	_remainingSetCount -= allocateInfo.descriptorSetCount;
	DEBUG_CLASS_INFO("Descriptor sets were allocated, remaining set count: " + std::to_string(_remainingSetCount));
}

void AfterglowDescriptorPool::freeDescriptorSets(VkDescriptorSet* pDescriptorSets, uint32_t descriptorSetCount) {
	if (vkFreeDescriptorSets(_device, *this, descriptorSetCount, pDescriptorSets) != VK_SUCCESS) {
		// Using for destructor, so never throw an exception.
		DEBUG_CLASS_FATAL("Failed to free descriptor sets.");
		return;
	}
	_remainingSetCount += descriptorSetCount;
	DEBUG_CLASS_INFO("Descriptor sets were freed, remaining set count: " + std::to_string(_remainingSetCount));
}

uint32_t AfterglowDescriptorPool::remainingUnallocatedSetCount() const {
	return _remainingSetCount;
}

//void AfterglowDescriptorPool::reset() {
//VkDescriptorPoolResetFlags
//	vkResetDescriptorPool(_device, *this, )
//}

void AfterglowDescriptorPool::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info().poolSizeCount = _poolSizes.size();
	info().pPoolSizes = _poolSizes.data();
	// Default size
	info().maxSets = cfg::descriptorSetSize;
	info().flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
}

void AfterglowDescriptorPool::create() {
	_remainingSetCount = info().maxSets;
	if (vkCreateDescriptorPool(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create descriptor pool.");
	}
}
