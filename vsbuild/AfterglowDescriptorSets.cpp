#include "AfterglowDescriptorSets.h"

AfterglowDescriptorSets::AfterglowDescriptorSets(AfterglowDescriptorPool& pool, AfterglowDescriptorSetLayout& layout, uint32_t numSets, uint32_t numExternSets) :
	AfterglowProxyArray(numSets + numExternSets),
	_descriptorPool(pool), 
	_rawLayouts(numSets, layout), 
	_numExternSets(numExternSets) {
	initialize();
}

AfterglowDescriptorSets::AfterglowDescriptorSets(AfterglowDescriptorPool& pool, const RawLayoutArray& layouts, uint32_t numExternSets) :
	AfterglowProxyArray(layouts.size() + numExternSets),
	_descriptorPool(pool),
	_rawLayouts(layouts), 
	_numExternSets(numExternSets) {
	initialize();
	DEBUG_CLASS_INFO("Created from const RawLayoutArray&.");
}

AfterglowDescriptorSets::AfterglowDescriptorSets(AfterglowDescriptorPool& pool, RawLayoutArray&& layouts, uint32_t numExternSets) :
	AfterglowProxyArray(layouts.size() + numExternSets),
	_descriptorPool(pool),
	_rawLayouts(layouts), 
	_numExternSets(numExternSets) {
	initialize();
	DEBUG_CLASS_INFO("Created from RawLayoutArray&&.");
}

AfterglowDescriptorSets::~AfterglowDescriptorSets() {
	// Here free function is declared in pool for existing sets record.
	_descriptorPool.freeDescriptorSets(allocatedAddress(), allocatedSize());
	destroy();
}

VkDescriptorSet* AfterglowDescriptorSets::address() {
	return data();
}

const VkDescriptorSet* AfterglowDescriptorSets::address() const {
	return data();
};

uint32_t AfterglowDescriptorSets::numExternSets() const {
	return _numExternSets;
}

VkDescriptorSet* AfterglowDescriptorSets::find(VkDescriptorSetLayout setLayout) {
	for (uint32_t index = 0; index < _rawLayouts.size(); ++index) {
		auto rawSetLayout = _rawLayouts[index];
		if (rawSetLayout == setLayout) {
			return &operator[](index + _numExternSets);
		}
	}
	return nullptr;
}

bool AfterglowDescriptorSets::fillExternSet(uint32_t index, VkDescriptorSet set) {
	if (index >= _numExternSets) {
		DEBUG_CLASS_WARNING("Index out of offset elements range.");
		return false;
	}
	data()[index] = set;
	return true;
}

void AfterglowDescriptorSets::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info().descriptorPool = _descriptorPool;
	// Should be same with class proxy info.
	info().descriptorSetCount = allocatedSize();
	info().pSetLayouts = _rawLayouts.data();
}

void AfterglowDescriptorSets::create() {
	// Because of a custom array offset is employed, we manully assign a non-null value to array[0] to bypass the ProxyArray validation check.
	data()[0] = reinterpret_cast<Raw>(0x1);

	if (_descriptorPool.remainingUnallocatedSetCount() < info().descriptorSetCount) {
		throw runtimeError("Descriptor pool have not enough unallocated sets, try to reset the pool.");
	}
	_descriptorPool.allocateDescriptorSets(info(), allocatedAddress());
}

VkDescriptorSet* AfterglowDescriptorSets::allocatedAddress() {
	return data() + _numExternSets;
}

uint32_t AfterglowDescriptorSets::allocatedSize() {
	return size() - _numExternSets;
}
