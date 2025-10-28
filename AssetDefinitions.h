#pragma once
#include <string>

namespace img {
	enum class Format {
		Undefined = 0x0,

		// File Image Format.
		Grey = 0x1,
		GreyAlpha = 0x2,
		RGB = 0x3,
		RGBA = 0x4,

		// Depth Format, place it here due to we just prepare one format variable for imageInfo.
		DepthOnly = 0x5,
		DepthStencil = 0x6
	};

	std::string HLSLPixelTypeName(Format format);

	enum class ColorSpace {
		Undefined = 0x0, 

		Linear = 0x1, 
		SRGB = 0x2
	};

	struct Info {
		Format format;
		uint64_t size;
		int32_t width;
		int32_t height;
		int32_t channels;
		ColorSpace colorSpace;
	};

	struct AssetInfo {
		Format format;
		ColorSpace colorSpace;
		std::string path;

		// For hash key comparation
		bool operator==(const AssetInfo& other) const;
	};
}

namespace model {
	enum class ImportFlag : uint32_t {
		None = 0, 
		GenerateTangent = 1 << 0, 
		GenerateAABB = 1 << 1
	};

	constexpr ImportFlag operator| (ImportFlag left, ImportFlag right);
	constexpr ImportFlag operator& (ImportFlag left, ImportFlag right);
	constexpr ImportFlag operator~ (ImportFlag flag);

	constexpr ImportFlag& operator|= (ImportFlag& left, ImportFlag right);
	constexpr ImportFlag& operator&= (ImportFlag& left, ImportFlag right);

	struct AssetInfo {
		ImportFlag importFlags;
		std::string path;
		// TODO: TypeIndex Flag.

		// For hash key compare
		bool operator==(const AssetInfo& other) const;
	};
}

constexpr model::ImportFlag model::operator|(ImportFlag left, ImportFlag right) {
	return static_cast<ImportFlag>(
		static_cast<std::underlying_type_t<ImportFlag>>(left) | static_cast<std::underlying_type_t<ImportFlag>>(right)
		);
}

constexpr model::ImportFlag model::operator&(ImportFlag left, ImportFlag right) {
	return static_cast<ImportFlag>(
		static_cast<std::underlying_type_t<ImportFlag>>(left) & static_cast<std::underlying_type_t<ImportFlag>>(right)
		);
}

constexpr model::ImportFlag model::operator~(ImportFlag flag) {
	return static_cast<ImportFlag>(~static_cast<std::underlying_type_t<ImportFlag>>(flag));
}

constexpr model::ImportFlag& model::operator|=(ImportFlag& left, ImportFlag right) {
	left = left | right;
	return left;
}

constexpr model::ImportFlag& model::operator&=(ImportFlag& left, ImportFlag right) {
	left = left & right;
	return left;
}

// Specialized hash function
namespace std {
	template <>
	struct hash<img::AssetInfo> {
		size_t operator()(const img::AssetInfo& key) const {
			size_t formatHash = hash<img::Format>{}(key.format);
			size_t colorSpaceHash = hash<img::ColorSpace>{}(key.colorSpace);
			size_t pathHash = hash<string>{}(key.path);
			return formatHash ^ (colorSpaceHash << 1) ^ (pathHash << 2);
		}
	};

	template <>
	struct hash<model::AssetInfo> {
		size_t operator()(const model::AssetInfo& key) const {
			size_t importFlagsHash = hash<model::ImportFlag>{}(key.importFlags);
			size_t pathHash = hash<string>{}(key.path);
			return importFlagsHash ^ (pathHash << 1);
		}
	};
}