#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include "VertexStructs.h"

// TODO: FileHead: VersionInfo, AABB.

// Fbx parse is too slow, so store them as cache file.
class AfterglowModelAssetCache {
public:
	using IndexArray = std::vector<vert::StandardIndex>;
	using VertexArray = std::vector<vert::StandardVertex>;
	using TimeStamp = std::chrono::time_point<std::chrono::file_clock>;

	struct FileHead {
		// Afterglow model cache
		const char flag[4] = "amc";
		uint32_t indexedTableByteSize;
		TimeStamp sourceFileModifiedTime;
	};

	struct IndexedTableElement {
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
	uint32_t numVertices(uint32_t meshIndex) const;

	// Read Functions
	void read(uint32_t meshIndex, IndexArray& destIndexArray, VertexArray& destVertexArray) const;

	// Write Functions
	void recordWrite(const IndexArray& indexArray, const VertexArray& vertexArray);
	void write(TimeStamp sourceFileModifiedTime);

	static const char* suffix();

private:
	static inline const char* _fileHeadFlag = "amc";
	static inline const char* _suffix = ".cache";

	struct Context;
	std::unique_ptr<Context> _context;
};

