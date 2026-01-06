#pragma once

#include <concepts>
#include <stdexcept>
#include "AfterglowStructLayout.h"

#include "TypeConstraints.h"
#include "DebugUtilities.h"

class AfterglowStructuredData {
public:

	AfterglowStructuredData(const AfterglowStructLayout& layout, size_t numElements);
	AfterglowStructuredData(size_t elementSize, size_t numElements);

	template<util::Trivial Type>
	void setElement(size_t index, const Type& value);

	template<util::Trivial Type>
	void setElementAttribute(size_t elementIndex, const std::string& attributeName, const Type& value);

	template<util::Trivial Type>
	Type& element(size_t index);

	template<util::Trivial Type>
	const Type& element(size_t index) const;

	template<util::Trivial Type>
	Type& elementAttribute(size_t elementIndex, const std::string& attributeName);

	template<util::Trivial Type>
	const Type& elementAttribute(size_t elementIndex, const std::string& attributeName) const;

	const char* data() const;
	uint64_t byteSize() const;

	template<constraint::ArrayType Type>
	void fill(const Type& srcArray);

	template<typename ElementType, typename CallbackType, typename ...ParamTypes>
	void forEachElement(CallbackType callback, ParamTypes&& ...params);

private:
	// Fixed info for fast access
	struct AttributeInfo {
		uint32_t byteSize;
		uint32_t offset;
	};

	template<util::Trivial Type>
	inline void verifyElement(size_t index) const;

	template<util::Trivial Type>
	inline AttributeInfo verifyAttribute(const std::string& attributeName) const;

	std::unordered_map<std::string, AttributeInfo> _attributeInfos;
	uint32_t _strideSize;
	std::vector<char> _data;
};

template<util::Trivial Type>
inline void AfterglowStructuredData::setElement(size_t index, const Type& value) {
	element<Type>(index) = value;
}

template<util::Trivial Type>
inline void AfterglowStructuredData::setElementAttribute(size_t elementIndex, const std::string& attributeName, const Type& value) {
	elementAttribute<Type>(elementIndex, attributeName) = value;
}

template<util::Trivial Type>
inline Type& AfterglowStructuredData::element(size_t index) {
	verifyElement<Type>(index);
	return *(reinterpret_cast<Type*>(_data.data()) + index);
}

template<util::Trivial Type>
inline const Type& AfterglowStructuredData::element(size_t index) const {
	return const_cast<AfterglowStructuredData*>(this)->element();
}

template<util::Trivial Type>
inline Type& AfterglowStructuredData::elementAttribute(size_t elementIndex, const std::string& attributeName) {
	AttributeInfo attrInfo = verifyAttribute<Type>(attributeName);
	return *(reinterpret_cast<Type*>(_data.data()) + elementIndex * _strideSize + attrInfo.offset);
}

template<util::Trivial Type>
inline const Type& AfterglowStructuredData::elementAttribute(size_t elementIndex, const std::string& attributeName) const {
	return const_cast<AfterglowStructuredData*>(this)->elementAttribute(elementIndex, attributeName);
}

template<constraint::ArrayType Type>
inline void AfterglowStructuredData::fill(const Type& srcArray) {
	const size_t srcArrayByteSize = sizeof(Type::value_type) * srcArray.size();
	if (srcArrayByteSize != _data.size()) {
		DEBUG_CLASS_WARNING(
			"The byte size of the sturctured data does not matched the source array, data would be truncated or some field would not be populated."
		);
	}
	memcpy_s(_data.data(), _data.size(), srcArray.data(), std::min(srcArrayByteSize, _data.size()));
}

template<typename ElementType, typename CallbackType, typename ...ParamTypes>
inline void AfterglowStructuredData::forEachElement(CallbackType callback, ParamTypes && ...params) {
	if (sizeof(ElementType) != _strideSize) {
		throw std::runtime_error("[AfterglowStructuredData] Different type size with layout size.");
	}
	size_t numElements = _data.size() / _strideSize;
	for (uint32_t index = 0; index < numElements; ++index) {
		callback(*(reinterpret_cast<ElementType*>(_data.data()) + index), params...);
	}
}

template<util::Trivial Type>
inline void AfterglowStructuredData::verifyElement(size_t index) const {
	if (sizeof(Type) != _strideSize) {
		throw std::runtime_error("[AfterglowStructuredData] Different type size with layout size.");
	}
	if (index < 0 || index >= /*numElements*/(_data.size() / _strideSize)) {
		throw std::runtime_error("[AfterglowStructuredData] Index out of range.");
	}
}

template<util::Trivial Type>
inline AfterglowStructuredData::AttributeInfo AfterglowStructuredData::verifyAttribute(const std::string& attributeName) const {
	auto iterator = _attributeInfos.find(attributeName);
	if (iterator == _attributeInfos.end()) {
		throw std::runtime_error("[AfterglowStructuredData] This name is not an element attribute name.");
	}
	if (iterator->second.byteSize != sizeof(Type)) {
		throw std::runtime_error("[AfterglowStructuredData] Different type size with attribute size.");
	}
	return iterator->second;
}
