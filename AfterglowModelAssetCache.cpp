#include "AfterglowModelAssetCache.h"
#include <fstream>
#include <stdexcept>
#include "DebugUtilities.h"

struct AfterglowModelAssetCache::Context {
	// Generic
	Mode mode;
	FileHead fileHead;
	std::string filePath;

	// Read
	std::unique_ptr<std::ifstream> inFile;
	std::unique_ptr<IndexedTable> indexedTable;

	// Write
	std::vector<std::pair<const IndexArray&, const VertexArray&>> meshRefs;

};

AfterglowModelAssetCache::AfterglowModelAssetCache(Mode mode, const std::string& path) : 
	_context(std::make_unique<Context>()) {
	_context->mode = mode;
	_context->filePath = path;
	if (mode == Mode::Read) {
		_context->inFile = std::make_unique<std::ifstream>(path, std::ios::binary);
		auto& inFile = *_context->inFile;
		auto& fileHead = _context->fileHead;
		if (!inFile) {
			DEBUG_CLASS_ERROR("Failed to open cache file: " + path);
			throw std::runtime_error("Failed to open cache file.");
		}
		inFile.read(reinterpret_cast<char*>(&fileHead), sizeof(FileHead));
		if (std::string(fileHead.flag) != _fileHeadFlag) {
			DEBUG_CLASS_ERROR("Invaild file head: " + path);
			throw std::runtime_error("Invaild file head.");
		}
		_context->indexedTable = std::make_unique<IndexedTable>(fileHead.indexedTableByteSize / sizeof(IndexedTableElement));
		auto& indexedTable = *_context->indexedTable;
		inFile.read(reinterpret_cast<char*>(indexedTable.data()), fileHead.indexedTableByteSize);
	}
	else if (mode == Mode::Write) {
		// Nothing yet.
	}
}

AfterglowModelAssetCache::~AfterglowModelAssetCache() {
}

AfterglowModelAssetCache::FileHead& AfterglowModelAssetCache::fileHead() {
	return _context->fileHead;
}

uint32_t AfterglowModelAssetCache::numMeshes() const {
	return _context->indexedTable->size();
}

uint32_t AfterglowModelAssetCache::numIndices(uint32_t meshIndex) const {
	return _context->indexedTable->at(meshIndex).indexDataSize / sizeof(IndexArray::value_type);
}

uint32_t AfterglowModelAssetCache::numVertices(uint32_t meshIndex) const {
	return _context->indexedTable->at(meshIndex).vertexDataSize / sizeof(VertexArray::value_type);
}

void AfterglowModelAssetCache::read(uint32_t meshIndex, IndexArray& destIndexArray, VertexArray& destVertexArray) const {
	destIndexArray.resize(numIndices(meshIndex));
	destVertexArray.resize(numVertices(meshIndex));
	auto& inFile = *_context->inFile;
	auto& tableElement = _context->indexedTable->at(meshIndex);
	inFile.seekg(tableElement.indexDataOffset, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(destIndexArray.data()), tableElement.indexDataSize);
	inFile.seekg(tableElement.vertexDataOffset, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(destVertexArray.data()), tableElement.vertexDataSize);
}

void AfterglowModelAssetCache::recordWrite(const IndexArray& indexArray, const VertexArray& vertexArray) {
	if (_context->mode != Mode::Write) {
		throw std::runtime_error("Mode is not Matched, recordWrite for Write only.");
	}
	_context->meshRefs.push_back({indexArray, vertexArray});
}

void AfterglowModelAssetCache::write(TimeStamp sourceFileModifiedTime) {
	size_t numMeshes = _context->meshRefs.size();

	auto& fileHead = _context->fileHead;
	fileHead.indexedTableByteSize = numMeshes * sizeof(IndexedTableElement);
	fileHead.sourceFileModifiedTime = sourceFileModifiedTime;

	IndexedTable indexedTable(numMeshes);

	uint64_t currentOffset = sizeof(FileHead) + fileHead.indexedTableByteSize;

	for (size_t index = 0; index < numMeshes; ++index) {
		auto& indexArray = _context->meshRefs[index].first;
		auto& vertexArray = _context->meshRefs[index].second;
		indexedTable[index].indexDataOffset = currentOffset;
		indexedTable[index].indexDataSize = indexArray.size() * sizeof(IndexArray::value_type);
		currentOffset += indexedTable[index].indexDataSize;
		indexedTable[index].vertexDataOffset = currentOffset;
		indexedTable[index].vertexDataSize = vertexArray.size() * sizeof(VertexArray::value_type);
		currentOffset += indexedTable[index].vertexDataSize;
	}

	std::ofstream outFile(_context->filePath, std::ios::binary);
	if (!outFile) {
		DEBUG_CLASS_ERROR("Failed to write file, invalid file path: " + _context->filePath);
		throw std::runtime_error("Failed to write file.");
	}
	outFile.write(reinterpret_cast<const char*>(&fileHead), sizeof(FileHead));
	outFile.write(reinterpret_cast<const char*>(indexedTable.data()), fileHead.indexedTableByteSize);

	for (size_t index = 0; index < numMeshes; ++index) {
		outFile.write(reinterpret_cast<const char*>(_context->meshRefs[index].first.data()), indexedTable[index].indexDataSize);
		outFile.write(reinterpret_cast<const char*>(_context->meshRefs[index].second.data()), indexedTable[index].vertexDataSize);
	}
}

const char* AfterglowModelAssetCache::suffix() {
	return _suffix;
}
