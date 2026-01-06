#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include "VertexStructs.h"
#include "AssetDefinitions.h"

// Fbx parse is too slow, so store them as cache file.
class AfterglowModelAssetCache {
public:
	using TimeStamp = std::chrono::time_point<std::chrono::file_clock>;

	struct alignas(8) FileHead {
		// Afterglow model cache
		const char flag[4] = "amc";
		uint16_t version;
		model::ImportFlag importFlags;
		uint32_t indexedTableByteSize;
		TimeStamp sourceFileModifiedTime;
		model::AABB aabb;
	};

	struct  alignas(8) IndexedTableElement {
		uint64_t indexDataOffset;
		uint64_t indexDataSize;
		uint64_t vertexDataOffset;
		uint64_t vertexDataSize;
	};

	using IndexedTable = std::vector<IndexedTableElement>;

	enum class Mode {
		Read, 
		Write
	};

	AfterglowModelAssetCache(Mode mode, const std::string& path);
	~AfterglowModelAssetCache();

	// Generic Functions
	FileHead& fileHead();
	uint32_t numMeshes() const;
	uint32_t numIndices(uint32_t meshIndex) const;
	uint32_t vertexDataSize(uint32_t meshIndex) const;

	// @brief: Check if the cache is outdated with input params.
	bool outdated(const model::AssetInfo& info, TimeStamp modifiedTime);

	// Read Functions
	void read(uint32_t meshIndex, vert::IndexArray& destIndexArray, vert::VertexData& destVertexData) const;

	// Write Functions
	void recordWrite(const vert::IndexArray& indexArray, const vert::VertexData& vertexData);
	void write(const model::AssetInfo& info, TimeStamp sourceFileModifiedTime, const model::AABB& aabb);

	static const std::string& suffix();

private:
	static inline const char* _fileHeadFlag = "amc";
	static inline std::string _suffix = ".cache";
	static inline uint16_t _currentVersion = 2;

	struct Impl;
	std::unique_ptr<Impl> _impl;
};
