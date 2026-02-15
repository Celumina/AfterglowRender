#include "AfterglowModelAsset.h"

#include <iostream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "AfterglowModelAssetCache.h"
#include "AfterglowUtilities.h"
#include "ExceptionUtilities.h"

struct AfterglowModelAsset::Impl {
	model::AssetInfo info = {};
	Assimp::Importer importer;
	const aiScene* scene = nullptr;

	// Static AABB of the whole scene where were combined from meshes.
	model::AABB aabb = {};

	unsigned int importSettings = 0;
	uint32_t numMeshes = 0;

	// std::vector<...> for different material index.
	std::vector<std::shared_ptr<vert::IndexArray>> indices;
	std::vector<std::shared_ptr<vert::VertexData>> vertices;

	inline void initScene();

	template<vert::VertexType Type>
	inline void initData();

	inline void initDataFromCache(const AfterglowModelAssetCache& cache);
	inline void generateCache();

	template<vert::VertexType Type>
	inline void setVertex(uint32_t meshIndex, const aiMesh* mesh, uint32_t meshVertexIndex);

	template<typename FuncType>
	inline void forEachNode(FuncType&& func, aiNode* parent);
};

AfterglowModelAsset::AfterglowModelAsset(const std::string& modelPath) :
	_impl(std::make_unique<Impl>()){
	_impl->info.importFlags = model::ImportFlag::None;
	_impl->info.path = modelPath;
	initialize();
}

AfterglowModelAsset::AfterglowModelAsset(const model::AssetInfo& assetInfo) :
	_impl(std::make_unique<Impl>()) {
	_impl->info = assetInfo;
	initialize();
}

AfterglowModelAsset::~AfterglowModelAsset() {
}

uint32_t AfterglowModelAsset::numMeshes() {
	return _impl->numMeshes;
}

std::weak_ptr<vert::IndexArray> AfterglowModelAsset::indices(uint32_t meshIndex) {
	return _impl->indices[meshIndex];
}

std::weak_ptr<vert::VertexData> AfterglowModelAsset::vertexData(uint32_t meshIndex) {
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

const model::AABB& AfterglowModelAsset::aabb() const noexcept {
	return _impl->aabb;
}

inline void AfterglowModelAsset::initialize() {
	// Generating AABB by default, due to camera culling and simple collision would use it.
	// Also the default aiProcess_FlipUVs is most general situation for texture mapping.
	_impl->importSettings |= 
		aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenBoundingBoxes | aiProcess_FlipUVs;

	auto importFlagBits = _impl->info.importFlags;
	if (static_cast<bool>(importFlagBits & model::ImportFlag::GenerateTangent)) {
		_impl->importSettings |= aiProcess_CalcTangentSpace;
	}
	if (static_cast<bool>(importFlagBits & model::ImportFlag::FlipUVs)) {
		_impl->importSettings &= ~aiProcess_FlipUVs;
	}
	if (static_cast<bool>(importFlagBits & model::ImportFlag::RecomputeNormal)) {
		_impl->importSettings |= aiProcess_ForceGenNormals;
	}

	// Try to load cache first.
	// TODO: Add a force reparse flag as param.
	std::string cachePath {_impl->info.path + AfterglowModelAssetCache::suffix()};
	if (std::filesystem::exists(cachePath)) {
		AfterglowModelAssetCache cache{ AfterglowModelAssetCache::Mode::Read, cachePath };
		if (!cache.outdated(_impl->info, std::filesystem::last_write_time(_impl->info.path))) {
			_impl->initDataFromCache(cache);
			return;
		}
	}

	// Otherwise parse model file (Very long time).
	DEBUG_CLASS_INFO("Model asset load begin: " + _impl->info.path);
	_impl->initScene();
	DEBUG_CLASS_INFO("Scene inialized.");
	AsType(_impl->info.importFlags, [this]<typename VertexType>(){
		_impl->initData<VertexType>();
	});
	DEBUG_CLASS_INFO("Indices and vertices data were loaded.");
	_impl->generateCache();
	DEBUG_CLASS_INFO("Cache file was generated.");
	// printModelInfo();
}

inline void AfterglowModelAsset::Impl::initScene() {
	if (scene) {
		return;
	}
	scene = importer.ReadFile(info.path, importSettings);
	if (!scene) {
		EXCEPT_CLASS_RUNTIME("Failed to import the model asset: " + info.path);
	}
	numMeshes = scene->mNumMeshes;
}

template<vert::VertexType Type>
void AfterglowModelAsset::Impl::initData() {
	indices.resize(scene->mNumMeshes);
	vertices.resize(scene->mNumMeshes);

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		auto* mesh = scene->mMeshes[meshIndex];

		// Combine AABBs from each meshes.
		model::AABB meshAABB = model::AABB{
			.min = { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z, },
			.max = { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z, }
		};
		aabb = model::CombineAABB(aabb, meshAABB);

		vertices[meshIndex] = std::make_shared<vert::VertexData>(mesh->mNumVertices * sizeof(Type));

		// @note: Make sure import settings yield triangulate meshes.
		indices[meshIndex] = std::make_shared<vert::IndexArray>(mesh->mNumFaces * 3);

		for (uint32_t meshVertexindex = 0; meshVertexindex < mesh->mNumVertices; ++meshVertexindex) {
			setVertex<Type>(meshIndex, mesh, meshVertexindex);
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
		indices[index] = std::make_shared<vert::IndexArray>();
		vertices[index] = std::make_shared<vert::VertexData>();
		cache.read(index, *indices[index], *vertices[index]);
	}
	aabb = cache.aabb();
}

inline void AfterglowModelAsset::Impl::generateCache() {
	AfterglowModelAssetCache cache(AfterglowModelAssetCache::Mode::Write, info.path + AfterglowModelAssetCache::suffix());
	for (uint32_t index = 0; index < scene->mNumMeshes; ++index) {
		cache.recordWrite(*indices[index], *vertices[index]);
	}
	cache.write(info, std::filesystem::last_write_time(info.path), aabb);
}

template<vert::VertexType Type>
inline void AfterglowModelAsset::Impl::setVertex(uint32_t meshIndex, const aiMesh* mesh, uint32_t meshVertexIndex) {
	auto& vertex = *reinterpret_cast<Type*>(&(*vertices[meshIndex])[meshVertexIndex * sizeof(Type)]);
	if constexpr (Type::template hasAttribute<vert::Position>()) {
		if (mesh->HasPositions()) {
			const aiVector3D& position = mesh->mVertices[meshVertexIndex];
			vertex.set<vert::Position>({position.x, position.y, position.z});
		}
	}

	if constexpr (Type::template hasAttribute<vert::Normal>()) {
		if (mesh->HasNormals()) {
			const aiVector3D& normal = mesh->mNormals[meshVertexIndex];
			vertex.set<vert::Normal>({ normal.x, normal.y, normal.z });
		}
	}

	if constexpr (Type::template hasAttribute<vert::Tangent>()) {
		if (mesh->HasTangentsAndBitangents()) {
			const aiVector3D& tangent = mesh->mTangents[meshVertexIndex];
			vertex.set<vert::Tangent>({ tangent.x, tangent.y, tangent.z });
		}
	}

	if constexpr (Type::template hasAttribute<vert::Bitangent>()) {
		if (mesh->HasTangentsAndBitangents()) {
			const aiVector3D& bitangent = mesh->mBitangents[meshVertexIndex];
			vertex.set<vert::Bitangent>({ bitangent.x, bitangent.y, bitangent.z });
		}
	}

	if constexpr (Type::template hasAttribute<vert::Color>()) {
		if (mesh->HasVertexColors(0)) {
			const aiColor4D& color = mesh->mColors[0][meshVertexIndex];
			vertex.set<vert::Color>({ color.r, color.g, color.b, color.a });
		}
	}
	// TODO: Many color groups here.

	// AfterglowVertex supports 4 groups Texture coordinates, theirs enough.
	// First TextureCoord Group: mTextureCoords[0]
	if constexpr (Type::template hasAttribute<vert::TexCoord0>()) {
		if (mesh->HasTextureCoords(0)) {
			const aiVector3D& uv = mesh->mTextureCoords[0][meshVertexIndex];
			vertex.set<vert::TexCoord0>({ uv.x, uv.y });
		}
	}
	if constexpr (Type::template hasAttribute<vert::TexCoord1>()) {
		if (mesh->HasTextureCoords(1)) {
			const aiVector3D& uv = mesh->mTextureCoords[1][meshVertexIndex];
			vertex.set<vert::TexCoord1>({ uv.x, uv.y });
		}
	}
	if constexpr (Type::template hasAttribute<vert::TexCoord2>()) {
		if (mesh->HasTextureCoords(2)) {
			const aiVector3D& uv = mesh->mTextureCoords[2][meshVertexIndex];
			vertex.set<vert::TexCoord2>({ uv.x, uv.y });
		}
	}
	if constexpr (Type::template hasAttribute<vert::TexCoord3>()) {
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