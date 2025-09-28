#pragma once
#include "AfterglowDevice.h"
#include "AfterglowDescriptorSetLayout.h"

class AfterglowDescriptorPool : public AfterglowProxyObject<AfterglowDescriptorPool, VkDescriptorPool, VkDescriptorPoolCreateInfo> {
public:
	using PoolSizeArray = std::vector<VkDescriptorPoolSize>;

	AfterglowDescriptorPool(AfterglowDevice& device);
	~AfterglowDescriptorPool();

	AfterglowDevice& device();

	bool isCreated();

	// Make sure pool is not created before extent the pool size.
	// @deprecated
	void extendPoolSize(AfterglowDescriptorSetLayout& layout);

	void extendUniformPoolSize(uint32_t descriptorCount);
	void extendImageSamplerPoolSize(uint32_t descriptorCount);

	void setMaxDescritporSets(uint32_t maxSets);

	// @descr: If failed to allocate descriptor sets, throw a runtime error.
	void allocateDescriptorSets(VkDescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets);
	void freeDescriptorSets(VkDescriptorSet* pDescriptorSets, uint32_t descriptorSetCount);

	uint32_t remainingUnallocatedSetCount() const;

	// void reset();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	// TODO: _poolSize Array.
	// Form DescriptorSetLayout.
	PoolSizeArray _poolSizes;
	AfterglowDevice& _device;

	uint32_t _remainingSetCount;
};

