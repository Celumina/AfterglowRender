#pragma once
#include "AfterglowDescriptorSets.h"

class AfterglowDescriptorSetReferences {
public:
	void reset(const AfterglowDescriptorSets& sourceSets);

	const AfterglowDescriptorSets* source() const noexcept;

	const AfterglowDescriptorSets::Raw* address() const noexcept;
	uint32_t size() const noexcept;

	void resize(uint32_t size);

	AfterglowDescriptorSets::Raw& operator[](uint32_t index);

private:
	const AfterglowDescriptorSets* _source = nullptr;
	std::vector<AfterglowDescriptorSets::Raw> _refData;

};

