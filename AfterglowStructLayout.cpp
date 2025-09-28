#include "AfterglowStructLayout.h"

#include <algorithm>
#include "DebugUtilities.h"


uint32_t AfterglowStructLayout::attributeByteSize(AttributeType type) {
	return _attributeTypeInfos.at(type).size;
}

uint32_t AfterglowStructLayout::numAttributeComponents(AttributeType type) {
	return _attributeTypeInfos.at(type).numComponents;
}

const std::string& AfterglowStructLayout::hlslTypeName(AttributeType type) {
	return _attributeTypeInfos.at(type).hlslTypeName;
}

uint32_t AfterglowStructLayout::numAttributes() const {
	return _attributes.size();
}

uint32_t AfterglowStructLayout::attributeByteSize(const std::string& name) const {
	for (const auto& attribute : _attributes) {
		if (attribute.name == name) {
			return _attributeTypeInfos.at(attribute.type).size;
		}
	}
	DEBUG_CLASS_WARNING("Attribute name is not exist.");
	return 0;
}

uint32_t AfterglowStructLayout::byteSize() const {
	uint32_t size = 0;
	uint32_t currentGroupSize = 0;
	for (const auto& attribute : _attributes) {
		uint32_t attributeSize = attributeByteSize(attribute.type);
		if (attributeSize + currentGroupSize > _structAlignment) {
			size = util::Align(size, _structAlignment) + attributeSize;
			currentGroupSize = util::Align(attributeSize, _structAlignment) - attributeSize;
		}
		else {
			size += attributeSize;
			currentGroupSize += attributeSize;
		}
	}
	return util::Align(size, _structAlignment);
}

uint32_t AfterglowStructLayout::offset(
	const std::string& name, 
	util::OptionalRef<std::function<AttributeMemberWithOffsetCallback>> callback) const {
	uint32_t size = 0;
	uint32_t currentGroupSize = 0;
	uint32_t offset = 0;
	for (const auto& attribute : _attributes) {
		if (callback != std::nullopt) {
			(*callback)(attribute, size);
		}
		if (attribute.name == name) {
			return offset;
		}

		uint32_t attributeSize = attributeByteSize(attribute.type);

		if (attributeSize + currentGroupSize > _structAlignment) {
			size = util::Align(size, _structAlignment) + attributeSize;
			currentGroupSize = util::Align(attributeSize, _structAlignment) - attributeSize;
		}
		else {
			size += attributeSize;
			currentGroupSize += attributeSize;
		}

		offset = size - attributeSize;
	}
	throw std::runtime_error("[AfterglowStructLayout] Attribute name not found, can not aquire offset value.");
}

void AfterglowStructLayout::addAttribute(AttributeType type, const std::string& name) {
	_attributes.emplace_back(type, name);
}

void AfterglowStructLayout::removeAttribute(const std::string& name) {
	std::erase_if(_attributes, [&name](const auto& attribute){ return attribute.name == name; });
}

bool AfterglowStructLayout::removeAttribute(uint32_t index) {
	if (index >= _attributes.size()) {
		return false;
	}
	_attributes.erase(_attributes.begin() + index);
	return true;
}

void AfterglowStructLayout::forEachAttributeMember(const std::function<AttributeMemberCallback>& callback) const {
	for (const auto& attribute : _attributes) {
		callback(attribute);
	}
}

void AfterglowStructLayout::forEachAttributeMemberWithOffset(const std::function<AttributeMemberWithOffsetCallback>& callback) const {
	// Oh, it's weird.
	offset(_attributes.back().name, callback);
}
