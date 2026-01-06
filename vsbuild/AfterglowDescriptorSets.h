#pragma once
#include "AfterglowProxyArray.h"
#include "AfterglowDescriptorSetLayout.h"
#include "AfterglowDescriptorPool.h"
#include "Configurations.h"
// Unn, a array based object again.
// TODO: Add a offset.
class AfterglowDescriptorSets : public AfterglowProxyArray<AfterglowDescriptorSets, VkDescriptorSet, VkDescriptorSetAllocateInfo> {
public:
	using RawLayoutArray = std::vector<AfterglowDescriptorSetLayout::Raw>;
	// TODO: Add offset param to constructor.
	AfterglowDescriptorSets(AfterglowDescriptorPool& pool, AfterglowDescriptorSetLayout& layout, uint32_t numSets, uint32_t numExternSets = 0);
	AfterglowDescriptorSets(AfterglowDescriptorPool& pool, const RawLayoutArray& layouts, uint32_t numExternSets = 0);
	AfterglowDescriptorSets(AfterglowDescriptorPool& pool, RawLayoutArray&& layouts, uint32_t numExternSets = 0);
	~AfterglowDescriptorSets();

	// @brief: begin of full array, not only allocated elements, remenber fill external sets manually.
	VkDescriptorSet* address();
	const VkDescriptorSet* address() const;

	uint32_t numExternSets() const;

	VkDescriptorSet* find(VkDescriptorSetLayout setLayout);

	/**
	* @brief: Replace element of set array by index, use to fill offset element (for the god damn global desciptor set).
	* @return: Success to replace descriptor set. 
	*/
	bool fillExternSet(uint32_t index, VkDescriptorSet set);

proxy_protected:
	void initCreateInfo();
	void create();

private:
	// @desc: internal sets address, returns data() + numExternSets;
	VkDescriptorSet* allocatedAddress();
	// @desc: internal set count, returns size() - numExternSets;
	uint32_t allocatedSize();

	AfterglowDescriptorPool& _descriptorPool;
	RawLayoutArray _rawLayouts;
	uint32_t _numExternSets;
};

