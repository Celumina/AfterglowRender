#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <algorithm>

#include "AfterglowObject.h"

class AfterglowContext {
public:
	using AttributeType = std::pair<std::type_index, std::unique_ptr<AfterglowObject>>;

	template<typename Type, typename ...ArgTypes>
	void initialize(ArgTypes&& ...args);

	template<typename Type>
	Type& get();

	const std::vector<AttributeType>& data() const;

private:
	template<typename Type>
	static std::type_index typeIndex();

	template<typename Type>
	AttributeType* at();

	template <typename Type>
	void registerAttribute();

	// We must ensure that the destruction order is reverse from the construction order.
	std::vector<AttributeType> _attributes;
	std::unordered_map<std::type_index, uint32_t> _attributeIndices;
};


template<typename Type, typename ...ArgTypes>
inline void AfterglowContext::initialize(ArgTypes&& ...args) {
	registerAttribute<Type>();
	at<Type>()->second = std::make_unique<Type>(args...);
}

template<typename Type>
inline Type& AfterglowContext::get() {
	registerAttribute<Type>();
	// return *reinterpret_cast<Type*>(rfind<Type>()->second.get());
	// New method is faster.
	return *reinterpret_cast<Type*>(at<Type>()->second.get());
}

template<typename Type>
inline std::type_index AfterglowContext::typeIndex() {
	return std::type_index(typeid(Type));
}

template<typename Type>
inline AfterglowContext::AttributeType* AfterglowContext::at() {
	auto indexIterator = _attributeIndices.find(typeIndex<Type>());
	if (indexIterator == _attributeIndices.end()) {
		return nullptr;
	}
	return &_attributes[indexIterator->second];
}

template<typename Type>
inline void AfterglowContext::registerAttribute() {
	if (!at<Type>()) {
		_attributes.push_back(std::make_pair(typeIndex<Type>(), nullptr));
		_attributeIndices[typeIndex<Type>()] = static_cast<uint32_t>(_attributes.size()) - 1;
	}
}
