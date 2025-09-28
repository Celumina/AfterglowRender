#include "AssetDefinitions.h"
#include <stdexcept>

std::string img::HLSLPixelTypeName(Format format) {
	switch (format) {
	case Format::Grey:
		return "float";
	case Format::GreyAlpha:
		return "float2";
	case Format::RGB:
		return "float3";
	case Format::RGBA:
		return "float4";
	case Format::DepthOnly:
		return "float";
	case Format::DepthStencil:
		return "float2";
	}
	throw std::runtime_error("Fomat type name is undefined.");
}

bool img::AssetInfo::operator==(const AssetInfo& other) const {
	return format == other.format && colorSpace == other.colorSpace && path == other.path;
}

bool model::AssetInfo::operator==(const AssetInfo& other) const {
	return importFlags == other.importFlags && path == other.path;
}
