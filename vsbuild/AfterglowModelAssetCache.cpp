#include "AfterglowModelAssetCache.h"
#include <fstream>
#include "ExceptionUtilities.h"

struct AfterglowModelAssetCache::Impl {
	// Generic
	Mode mode;
	FileHead fileHead;
	std::string filePath;

	// Read
	std::unique_ptr<std::ifstream> inFile;
	std::unique_ptr<IndexedTable> indexedTable;

	// Write
	std::vector<std::pair<const vert::IndexArray&, const vert::VertexData&>> meshRefs;

};

AfterglowModelAssetCache::AfterglowModelAssetCache(Mode mode, const std::string& path) : 
	_impl(std::make_unique<Impl>()) {
	_impl->mode = mode;
	_impl->filePath = path;
	if (mode == Mode::Read) {
		_impl->inFile = std::make_unique<std::ifstream>(path, std::ios::binary);
		auto& inFile = *_impl->inFile;
		auto& fileHead = _impl->fileHead;
		if (!inFile) {
			EXCEPT_CLASS_RUNTIME("Failed to open cache file: " + path);
		}
		inFile.read(reinterpret_cast<char*>(&fileHead), sizeof(FileHead));
		if (std::string(fileHead.flag) != _fileHeadFlag) {
			EXCEPT_CLASS_RUNTIME("Invaild file head: " + path);
		}
		_impl->indexedTable = std::make_unique<IndexedTable>(fileHead.indexedTableByteSize / sizeof(IndexedTableElement));
		auto& indexedTable = *_impl->indexedTable;
		inFile.read(reinterpret_cast<char*>(indexedTable.data()), fileHead.indexedTableByteSize);
	}
	else if (mode == Mode::Write) {
		// Nothing yet.
	}
}

AfterglowModelAssetCache::~AfterglowModelAssetCache() {
}

AfterglowModelAssetCache::FileHead& AfterglowModelAssetCache::fileHead() {
	return _impl->fileHead;
}

uint32_t AfterglowModelAssetCache::numMeshes() const {
	return _impl->indexedTable->size();
}

uint32_t AfterglowModelAssetCache::numIndices(uint32_t meshIndex) const {
	return _impl->indexedTable->at(meshIndex).indexDataSize / sizeof(vert::IndexArray::value_type);
}

uint32_t AfterglowModelAssetCache::vertexDataSize(uint32_t meshIndex) const {
	return _impl->indexedTable->at(meshIndex).vertexDataSize;
}

bool AfterglowModelAssetCache::outdated(const model::AssetInfo& info, TimeStamp modifiedTime) {
	if (_impl->fileHead.version != _currentVersion
		|| info.importFlags != _impl->fileHead.importFlags
		|| modifiedTime != _impl->fileHead.sourceFileModifiedTime
		) {
		return true;
	}
	return false;
}

void AfterglowModelAssetCache::read(uint32_t meshIndex, vert::IndexArray & destIndexArray, vert::VertexData& destVertexData) const {
	destIndexArray.resize(numIndices(meshIndex));
	destVertexData.resize(vertexDataSize(meshIndex));
	auto& inFile = *_impl->inFile;
	auto& tableElement = _impl->indexedTable->at(meshIndex);
	inFile.seekg(tableElement.indexDataOffset, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(destIndexArray.data()), tableElement.indexDataSize);
	inFile.seekg(tableElement.vertexDataOffset, std::ios::beg);
	inFile.read(destVertexData.data(), tableElement.vertexDataSize);
}

const model::AABB& AfterglowModelAssetCache::aabb() const {
	return _impl->fileHead.aabb;
}

void AfterglowModelAssetCache::recordWrite(const vert::IndexArray& indexArray, const vert::VertexData& vertexData) {
	if (_impl->mode != Mode::Write) {
		EXCEPT_CLASS_RUNTIME("Mode is not Matched, recordWrite for Write only.");
	}
	_impl->meshRefs.push_back({indexArray, vertexData});
}

void AfterglowModelAssetCache::write(const model::AssetInfo& info, TimeStamp sourceFileModifiedTime, const model::AABB& aabb) {
	size_t numMeshes = _impl->meshRefs.size();

	auto& fileHead = _impl->fileHead;
	fileHead.version = _currentVersion;
	fileHead.importFlags = info.importFlags;
	fileHead.indexedTableByteSize = numMeshes * sizeof(IndexedTableElement);
	fileHead.sourceFileModifiedTime = sourceFileModifiedTime;
	fileHead.aabb = aabb;

	IndexedTable indexedTable(numMeshes);

	uint64_t currentOffset = sizeof(FileHead) + fileHead.indexedTableByteSize;

	for (size_t index = 0; index < numMeshes; ++index) {
		auto& indexArray = _impl->meshRefs[index].first;
		auto& vertexData = _impl->meshRefs[index].second;
		indexedTable[index].indexDataOffset = currentOffset;
		indexedTable[index].indexDataSize = indexArray.size() * sizeof(vert::IndexArray::value_type);
		currentOffset += indexedTable[index].indexDataSize;
		indexedTable[index].vertexDataOffset = currentOffset;
		indexedTable[index].vertexDataSize = vertexData.size();
		currentOffset += indexedTable[index].vertexDataSize;
	}

	std::ofstream outFile(_impl->filePath, std::ios::binary);
	if (!outFile) {
		EXCEPT_CLASS_RUNTIME("Failed to write file, invalid file path: " + _impl->filePath);
	}
	outFile.write(reinterpret_cast<const char*>(&fileHead), sizeof(FileHead));
	outFile.write(reinterpret_cast<const char*>(indexedTable.data()), fileHead.indexedTableByteSize);

	for (size_t index = 0; index < numMeshes; ++index) {
		outFile.write(reinterpret_cast<const char*>(_impl->meshRefs[index].first.data()), indexedTable[index].indexDataSize);
		outFile.write(reinterpret_cast<const char*>(_impl->meshRefs[index].second.data()), indexedTable[index].vertexDataSize);
	}
}

const std::string& AfterglowModelAssetCache::suffix() {
	return _suffix;
}