#include "AssetDefinitions.h"
#include <stdexcept>
#include <algorithm>

uint32_t img::FormatByteSize(Format format) {
	switch (format) {		
	case img::Format::UnsignedInt8: return 1;
	case img::Format::Int8: return 1;
	case img::Format::UnsignedInt16: return 2;
	case img::Format::Int16: return 2;
	case img::Format::Half: return 2;
	case img::Format::UnsignedInt32: return 4;
	case img::Format::Int32: return 4; 
	case img::Format::Float: return 4; 
	case img::Format::UnsignedInt64: return 8;
	case img::Format::Int64: return 8;
	case img::Format::Double: return 8;
	default:
		throw std:: runtime_error("Image format is undefined.");
	}
}

bool img::AssetInfo::operator==(const AssetInfo& other) const noexcept {
	return colorSpace == other.colorSpace && path == other.path;
}

bool model::AssetInfo::operator==(const AssetInfo& other) const noexcept {
	return importFlags == other.importFlags && path == other.path;
}

model::AABB model::CombineAABB(const AABB& lhs, const AABB& rhs) noexcept {
	constexpr uint32_t dimension = sizeof(AABB::min) / sizeof(AABB::min[0]);
	AABB result{};
	for (uint32_t index = 0; index < dimension; ++index) {
		result.min[index] = std::min(lhs.min[index], rhs.min[index]);
		result.max[index] = std::max(lhs.max[index], rhs.max[index]);
	}
	return result;
}

bool model::IsFlagVertexType(ImportFlag flags, ImportFlag vertexFlag) noexcept {
	return (flags & ImportFlag::VertexBitsMask) == vertexFlag;
}