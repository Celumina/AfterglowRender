#include "AfterglowModelAsset.h"

#include <stdexcept>
#include <iostream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "AfterglowModelAssetCache.h"
#include "AfterglowUtilities.h"
#include "DebugUtilities.h"

struct AfterglowModelAsset::Impl {
	std::string path;
	unsigned int importSettings = 0;
	Assimp::Importer importer;
	const aiScene* scene = nullptr;

	// std::vector<...> for different material index.
	std::vector<std::shared_ptr<IndexArray>> indices;
	std::vector<std::shared_ptr<VertexArray>> vertices;

	uint32_t numMeshes;

	inline void initScene();
	inline void initData();
	inline void initDataFromCache(const AfterglowModelAssetCache& cache);
	inline void generateCache();

	inline void setVertex(uint32_t meshIndex, const aiMesh* mesh, uint32_t meshVertexIndex);

	template<typename FuncType>
	inline void forEachNode(FuncType&& func, aiNode* parent);
};

AfterglowModelAsset::AfterglowModelAsset(const std::string& modelPath) :
	_impl(std::make_unique<Impl>()){
	_impl->path = modelPath;
	initialize();
}

AfterglowModelAsset::AfterglowModelAsset(const model::AssetInfo& assetInfo) :
	_impl(std::make_unique<Impl>()) {
	_impl->path = assetInfo.path;

	auto importFlagBits = util::EnumValue(assetInfo.importFlags);

	if (importFlagBits & util::EnumValue(model::ImportFlag::GenerateTangent)) {
		_impl->importSettings |= aiProcess_CalcTangentSpace;
	}
	if (importFlagBits & util::EnumValue(model::ImportFlag::GenerateAABB)) {
		_impl->importSettings |= aiProcess_GenBoundingBoxes;
	}

	initialize();
}

AfterglowModelAsset::~AfterglowModelAsset() {
}

uint32_t AfterglowModelAsset::numMeshes() {
	return _impl->numMeshes;
}

std::weak_ptr<AfterglowModelAsset::IndexArray> AfterglowModelAsset::indices(uint32_t meshIndex) {
	return _impl->indices[meshIndex];
}

std::weak_ptr<AfterglowModelAsset::VertexArray> AfterglowModelAsset::vertices(uint32_t meshIndex) {
	return _impl->vertices[meshIndex];
}

void AfterglowModelAsset::printModelInfo() {
	_impl->forEachNode(
		[this](aiNode* node) {
			std::cout << "Node: " << node->mName.C_Str() << "\n";
			uint32_t numMeshes = node->mNumMeshes;
			if (numMeshes) {
				std::cout << "..NumMeshes: " << numMeshes << "\n";
				for (uint32_t index = 0; index < numMeshes; ++index) {
					uint32_t meshIndex = node->mMeshes[index];
					auto* mesh = _impl->scene->mMeshes[meshIndex];
					std::cout << "..Mesh: " << mesh->mName.C_Str() << "\n";
					std::cout << "....NumVertices: " << mesh->mNumVertices << "\n";
					std::cout << "....NumFaces: " << mesh->mNumFaces << "\n";
					std::cout << "....NumAnimMeshes: " << mesh->mNumAnimMeshes << "\n";
					std::cout << "....NumBones: " << mesh->mNumBones << "\n";
					std::cout << "....MaterialIndex: " << mesh->mMaterialIndex << "\n";
				}
			}
		}, 
		_impl->scene->mRootNode
	);
}

inline void AfterglowModelAsset::initialize() {
	_impl->importSettings |= aiProcess_Triangulate | aiProcess_SortByPType;
	// Try to load cache first.
	// TODO: Add a force reparse flag as param.
	std::string cachePath {_impl->path + AfterglowModelAssetCache::suffix()};
	if (std::filesystem::exists(cachePath)) {
		AfterglowModelAssetCache cache{ AfterglowModelAssetCache::Mode::Read, cachePath };
		if (cache.fileHead().sourceFileModifiedTime == std::filesystem::last_write_time(_impl->path)) {
			_impl->initDataFromCache(cache);
			return;
		}
	}

	// Otherwise parse model file (Very long time).
	DEBUG_CLASS_INFO("Model asset load begin: " + _impl->path);
	_impl->initScene();
	DEBUG_CLASS_INFO("Scene inialized.");
	_impl->initData();
	DEBUG_CLASS_INFO("Indices and vertices data were loaded.");
	_impl->generateCache();
	DEBUG_CLASS_INFO("Cache file was generated.");
	// printModelInfo();
}

inline void AfterglowModelAsset::Impl::initScene() {
	if (scene) {
		return;
	}
	scene = importer.ReadFile(path, importSettings);
	if (!scene) {
		DEBUG_CLASS_ERROR("Failed to import the model asset: " + path);
		throw std::runtime_error("[AfterglowModelAsset] Failed to import the model asset.");
	}
	numMeshes = scene->mNumMeshes;
}

void AfterglowModelAsset::Impl::initData() {
	indices.resize(scene->mNumMeshes);
	vertices.resize(scene->mNumMeshes);

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		auto* mesh = scene->mMeshes[meshIndex];
		vertices[meshIndex] = std::make_shared<VertexArray>(mesh->mNumVertices);
		// Make sure import settings get triangulate mesh.
		indices[meshIndex] = std::make_shared<IndexArray>(mesh->mNumFaces * 3);

		for (uint32_t meshVertexindex = 0; meshVertexindex < mesh->mNumVertices; ++meshVertexindex) {
			setVertex(meshIndex, mesh, meshVertexindex);
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			const aiFace& face = mesh->mFaces[faceIndex];
			// Number of faceVertIndex usually are 3. and face.mIndices[faceVertIndex] gets that vertex's global index.
			for (uint32_t faceVertIndex = 0; faceVertIndex < face.mNumIndices; ++faceVertIndex) {
				(*indices[meshIndex])[faceIndex * face.mNumIndices + faceVertIndex] = face.mIndices[faceVertIndex];
			}
		}
	}
}

inline void AfterglowModelAsset::Impl::initDataFromCache(const AfterglowModelAssetCache& cache) {
	numMeshes = cache.numMeshes();
	indices.resize(numMeshes);
	vertices.resize(numMeshes);
	for (uint32_t index = 0; index < numMeshes; ++index) {
		indices[index] = std::make_shared<IndexArray>();
		vertices[index] = std::make_shared<VertexArray>();
		cache.read(index, *indices[index], *vertices[index]);
	}
}

inline void AfterglowModelAsset::Impl::generateCache() {
	AfterglowModelAssetCache cache(AfterglowModelAssetCache::Mode::Write, path + AfterglowModelAssetCache::suffix());
	for (uint32_t index = 0; index < scene->mNumMeshes; ++index) {
		cache.recordWrite(*indices[index], *vertices[index]);
	}
	cache.write(std::filesystem::last_write_time(path));
}

inline void AfterglowModelAsset::Impl::setVertex(uint32_t meshIndex, const aiMesh* mesh, uint32_t meshVertexIndex) {
	auto& vertex = (*vertices[meshIndex])[meshVertexIndex];
	if constexpr (vert::StandardVertex::hasAttribute<vert::Position>()) {
		if (mesh->HasPositions()) {
			const aiVector3D& position = mesh->mVertices[meshVertexIndex];
			vertex.set<vert::Position>({position.x, position.y, position.z});
		}
	}

	if constexpr (vert::StandardVertex::hasAttribute<vert::Normal>()) {
		if (mesh->HasNormals()) {
			const aiVector3D& normal = mesh->mNormals[meshVertexIndex];
			vertex.set<vert::Normal>({ normal.x, normal.y, normal.z });
		}
	}

	if constexpr (vert::StandardVertex::hasAttribute<vert::Tangent>()) {
		if (mesh->HasTangentsAndBitangents()) {
			const aiVector3D& tangent = mesh->mTangents[meshVertexIndex];
			vertex.set<vert::Tangent>({ tangent.x, tangent.y, tangent.z });
		}
	}

	if constexpr (vert::StandardVertex::hasAttribute<vert::Bitangent>()) {
		if (mesh->HasTangentsAndBitangents()) {
			const aiVector3D& bitangent = mesh->mBitangents[meshVertexIndex];
			vertex.set<vert::Bitangent>({ bitangent.x, bitangent.y, bitangent.z });
		}
	}

	if constexpr (vert::StandardVertex::hasAttribute<vert::Color>()) {
		if (mesh->HasVertexColors(0)) {
			const aiColor4D& color = mesh->mColors[0][meshVertexIndex];
			vertex.set<vert::Color>({ color.r, color.g, color.b, color.a });
		}
	}
	// TODO: More color group here.

	// AfterglowVertex supports 4 groups Texture coordinates, theirs enough.
	// First TextureCoord Group: mTextureCoords[0]
	if constexpr (vert::StandardVertex::hasAttribute<vert::TexCoord0>()) {
		if (mesh->HasTextureCoords(0)) {
			const aiVector3D& uv = mesh->mTextureCoords[0][meshVertexIndex];
			vertex.set<vert::TexCoord0>({ uv.x, uv.y });
		}
	}
	if constexpr (vert::StandardVertex::hasAttribute<vert::TexCoord1>()) {
		if (mesh->HasTextureCoords(1)) {
			const aiVector3D& uv = mesh->mTextureCoords[1][meshVertexIndex];
			vertex.set<vert::TexCoord1>({ uv.x, uv.y });
		}
	}
	if constexpr (vert::StandardVertex::hasAttribute<vert::TexCoord2>()) {
		if (mesh->HasTextureCoords(2)) {
			const aiVector3D& uv = mesh->mTextureCoords[2][meshVertexIndex];
			vertex.set<vert::TexCoord2>({ uv.x, uv.y });
		}
	}
	if constexpr (vert::StandardVertex::hasAttribute<vert::TexCoord3>()) {
		if (mesh->HasTextureCoords(3)) {
			const aiVector3D& uv = mesh->mTextureCoords[3][meshVertexIndex];
			vertex.set<vert::TexCoord3>({ uv.x, uv.y });
		}
	}
}

template<typename FuncType>
inline void AfterglowModelAsset::Impl::forEachNode(FuncType&& func, aiNode* parent) {
	if (!parent) {
		return;
	}
	func(parent);
	uint32_t numChildren = parent->mNumChildren;
	for (uint32_t index = 0; index < numChildren; ++index) {
		forEachNode(func, parent->mChildren[index]);
	}
}