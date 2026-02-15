#pragma once
#include <string>
#include <vector>

namespace img {
	using DataArray = std::vector<char>;

	enum class Channel {
		None = 0, 
		R = 1, 
		RG = 2, 
		RGB = 3, 
		RGBA = 4
	};

	enum class Format {
		Undefined = 0,

		// 8 bits
		UnsignedInt8, 
		Int8, 

		// 16 bits
		UnsignedInt16, 
		Int16, 
		Half, 

		// 32 bits
		UnsignedInt32,
		Int32,
		Float, 

		// 64 bits
		UnsignedInt64,
		Int64,
		Double, 

		// depth format (32bits)
		DepthOnly, 
		DepthStencil, 

		EnunCount
	};

	uint32_t FormatByteSize(Format format);

	// @deprecated: Deduce pixel type implcit in HLSL.
	// std::string HLSLPixelTypeName(uint32_t numChannels, Format format);


	enum class ColorSpace {
		Undefined = 0, 

		Linear = 1, 
		SRGB = 2, 

		EnunCount
	};

	struct Info {
		uint64_t size = 0;
		Format format = Format::Undefined;
		int32_t width = 1;
		int32_t height = 1;
		int32_t depth = 1; // for 3D image. [width, height, depth]
		Channel channels = Channel::None;
		ColorSpace colorSpace = ColorSpace::Linear;
	};

	struct AssetInfo {
		// Format format;
		ColorSpace colorSpace;
		std::string path;

		// For hash key comparation
		bool operator==(const AssetInfo& other) const noexcept;
	};

}

namespace model {
	struct AABB {
		float min[3] = { 0.0f };
		float max[3] = { 0.0f };
	};

	AABB CombineAABB(const AABB& lhs, const AABB& rhs) noexcept;

	enum class ImportFlag : uint32_t {
		None = 0, 
		GenerateTangent = 1 << 0, 

		IgnoreVertexNormal = 1 << 1,
		VertexPTCT0 = IgnoreVertexNormal,
	
		IgnoreVertexTangent = 1 << 2,
		IgnoreNormalMap = IgnoreVertexTangent,
		VertexPNCT0 = IgnoreVertexTangent,
		IgnoreLighting = IgnoreVertexTangent | IgnoreVertexNormal,
		VertexPCT0 = IgnoreLighting,

		IgnoreVertexColor = 1 << 3,
		VertexPNTT0 = IgnoreVertexColor,
		VertexPTT0 = IgnoreVertexColor | IgnoreVertexNormal,
		VertexPNT0 = IgnoreVertexColor | IgnoreVertexTangent,
		VertexPT0 = IgnoreVertexColor | IgnoreVertexNormal | IgnoreVertexTangent,
		
		IgnoreVertexTexCoords = 1 << 4, 
		IgnoreTextureMapping = IgnoreVertexTexCoords,
		VertexPNTC = IgnoreVertexTexCoords,
		VertexPTC = IgnoreVertexTexCoords | IgnoreVertexNormal,
		VertexPNC = IgnoreVertexTexCoords | IgnoreVertexTangent,
		VertexPNT = IgnoreVertexTexCoords | IgnoreVertexColor,
		VertexPC = IgnoreVertexTexCoords | IgnoreVertexNormal | IgnoreVertexTangent,
		VertexPT = IgnoreVertexTexCoords | IgnoreVertexNormal | IgnoreVertexColor,
		VertexPN = IgnoreVertexTexCoords | IgnoreVertexTangent | IgnoreVertexColor,

		PositionOnly = IgnoreVertexNormal | IgnoreVertexTangent | IgnoreVertexColor | IgnoreVertexTexCoords,
		VertexP = PositionOnly,
		VertexBitsMask = PositionOnly,

		FlipUVs = 1 << 5, 
		RecomputeNormal = 1 << 6
	};

	bool IsFlagVertexType(ImportFlag flags, ImportFlag vertexFlag) noexcept;

	constexpr ImportFlag operator| (ImportFlag left, ImportFlag right);
	constexpr ImportFlag operator& (ImportFlag left, ImportFlag right);
	constexpr ImportFlag operator~ (ImportFlag flag);

	constexpr ImportFlag& operator|= (ImportFlag& left, ImportFlag right);
	constexpr ImportFlag& operator&= (ImportFlag& left, ImportFlag right);

	struct AssetInfo {
		ImportFlag importFlags = ImportFlag::None;
		std::string path;

		// For hash key compare
		bool operator==(const AssetInfo& other) const noexcept;
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


namespace util {
	// From Boosts.
	template<typename Type> void HashCombine(size_t& seed, Type const& v) {
		seed ^= std::hash<Type>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
}

// Specialized hash function
namespace std {
	template <>
	struct hash<img::AssetInfo> {
		size_t operator()(const img::AssetInfo& key) const {
			size_t seed = 0; 
			util::HashCombine(seed, key.colorSpace);
			util::HashCombine(seed, key.path);
			return seed;
		}
	};

	template <>
	struct hash<model::AssetInfo> {
		size_t operator()(const model::AssetInfo& key) const {
			size_t seed = 0; 
			util::HashCombine(seed, key.importFlags);
			util::HashCombine(seed, key.path);
			return seed;
		}
	};
}