#include "AfterglowStructLayout.h"

#include <algorithm>
#include <stdexcept>
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

uint32_t AfterglowStructLayout::numAttributes() const noexcept {
	return static_cast<uint32_t>(_attributes.size());
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
		 updateAttributeAlignmentStates(attributeSize, size, currentGroupSize);
	}
	return util::Align(size, _structAlignment);
}

uint32_t AfterglowStructLayout::byteSizeWithoutPadding() const {
	uint32_t size = 0;
	for (const auto& attribute : _attributes) {
		size += attributeByteSize(attribute.type);
	}
	return size;
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
		updateAttributeAlignmentStates(attributeSize, size, currentGroupSize);

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
	if (_attributes.empty()) {
		DEBUG_CLASS_ERROR("StructLayout is empty.");
		return;
	}
	// Oops, it's weird.
	offset(_attributes.back().name, callback);
}

std::vector<AfterglowStructLayout::AttributeMember> AfterglowStructLayout::generateHLSLStructMembers() const {
	std::vector<AttributeMember> members;
	members.reserve(_attributes.size());

	uint32_t size = 0;
	uint32_t currentGroupSize = 0;
	uint32_t paddingIndex = 0;

	for (uint32_t index = 0; index < _attributes.size(); ++index) {
		members.push_back(_attributes[index]);
		uint32_t attributeSize = attributeByteSize(_attributes[index].type);
		updateAttributeAlignmentStates(attributeSize, size, currentGroupSize);

		if (index >= _attributes.size() - 1 
			|| attributeByteSize(_attributes[index + 1].type) + currentGroupSize > _structAlignment) {
			appendPaddingAttributes(members, util::Align(size, _structAlignment) - size);
		}
	}
	return members;
}

inline void AfterglowStructLayout::appendPaddingAttributes(std::vector<AttributeMember>& dest, int32_t paddingSize) const {
	while (paddingSize >= attributeByteSize(AttributeType::Float)) {
		dest.emplace_back(AttributeType::Float, std::format("__padding{}", dest.size()));
		paddingSize -= attributeByteSize(AttributeType::Float);
	}
}

inline void AfterglowStructLayout::updateAttributeAlignmentStates(
	const uint32_t attributeSize, uint32_t& size, uint32_t& currentGroupSize) const noexcept {
	if (attributeSize + currentGroupSize > _structAlignment) {
		size = util::Align(size, _structAlignment) + attributeSize;
		currentGroupSize = attributeSize % _structAlignment; 
	}
	else {
		size += attributeSize;
		currentGroupSize = (currentGroupSize + attributeSize) % _structAlignment;
	}
}
