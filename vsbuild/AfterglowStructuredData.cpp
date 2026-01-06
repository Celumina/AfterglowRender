#include "AfterglowStructuredData.h"

AfterglowStructuredData::AfterglowStructuredData(const AfterglowStructLayout& layout, size_t numElements) :
	_strideSize(layout.byteSize()), _data(_strideSize * numElements) {
	layout.forEachAttributeMemberWithOffset([this](const AfterglowStructLayout::AttributeMember& member, uint32_t offset){
		_attributeInfos[member.name] = { AfterglowStructLayout::attributeByteSize(member.type), offset };
	});
}

AfterglowStructuredData::AfterglowStructuredData(size_t elementSize, size_t numElements) : 
	_strideSize(elementSize), _data(_strideSize * numElements) {
}

const char* AfterglowStructuredData::data() const {
	return _data.data();
}

uint64_t AfterglowStructuredData::byteSize() const {
	return _data.size();
}
