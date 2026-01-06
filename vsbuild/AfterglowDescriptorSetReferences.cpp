#include "AfterglowDescriptorSetReferences.h"


void AfterglowDescriptorSetReferences::reset(const AfterglowDescriptorSets& sourceSets) {
	_source = &sourceSets;
	_refData.resize(sourceSets.size());
	uint32_t byteSize = sourceSets.size() * sizeof(AfterglowDescriptorSets::Raw);
	memcpy_s(_refData.data(), byteSize, sourceSets.address(), byteSize);
}

const AfterglowDescriptorSets* AfterglowDescriptorSetReferences::source() const noexcept {
	return _source;
}

const AfterglowDescriptorSets::Raw* AfterglowDescriptorSetReferences::address() const noexcept {
	return _refData.data();
}

uint32_t AfterglowDescriptorSetReferences::size() const noexcept {
	return static_cast<uint32_t>(_refData.size());
}

void AfterglowDescriptorSetReferences::resize(uint32_t size) {
	_refData.resize(size);
}

AfterglowDescriptorSets::Raw& AfterglowDescriptorSetReferences::operator[](uint32_t index) {
	return _refData[index];
}