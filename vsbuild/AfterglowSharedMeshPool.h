#pragma once

#include "AfterglowSharedResourcePool.h"

#include "AfterglowIndexBuffer.h"
#include "AfterglowVertexBuffer.h"
#include "AfterglowModelAsset.h"

struct AfterglowMeshPoolResource {
	AfterglowIndexBuffer::Array indexBuffers;
	// AfterglowVertexBuffer::Array vertexBuffers;
	// Polynomial unique_ptr for Different VertexBuffer Types
	std::vector<std::unique_ptr<AfterglowObject>> vertexBuffers;
	
	// VertexBufferHandle is type-independent, for different vertex buffer type support.
	std::vector<AfterglowVertexBufferHandle> vertexBufferHandles;
	AfterglowReferenceCount count;
};


class AfterglowMeshReference : public AfterglowResourceReference<model::AssetInfo, AfterglowMeshPoolResource> {
public:
	AfterglowMeshReference(
		const model::AssetInfo& assetInfo, AfterglowResourceReference::Resources& meshes, AfterglowReferenceCount& count
	);
	AfterglowMeshReference(const AfterglowMeshReference& other);

	// const model::AssetInfo& assetInfo() const;
	AfterglowIndexBuffer::Array& indexBuffers() noexcept;
	//AfterglowVertexBuffer::Array& vertexBuffers() noexcept;
	std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles() noexcept;
};


class AfterglowSharedMeshPool : public AfterglowSharedResourcePool<AfterglowMeshReference> {
public: 
	AfterglowSharedMeshPool(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	// @brief: Get ref of mesh resource, if resource not exists, it will create mesh from file automatically.
	AfterglowMeshReference mesh(const model::AssetInfo& assetInfo);

private:
	template<vert::VertexType Type>
	Resource* createMesh(const model::AssetInfo& assetInfo);
};

template<vert::VertexType Type>
AfterglowSharedMeshPool::Resource* AfterglowSharedMeshPool::createMesh(const model::AssetInfo& assetInfo) {
	using VertexBuffer = AfterglowVertexBufferTemplate<Type>;
	auto meshIterator = _resources.emplace(assetInfo, Resource{}).first;
	auto& mesh = meshIterator->second;
	mesh.count.setDecreaseCallback(
		[this, meshIterator](AfterglowReferenceCount::Count count) {
			if (count <= 0) {
				DEBUG_CLASS_INFO("Mesh was destroyed: " + meshIterator->first.path);
				// _resources.erase(meshIterator);
				removeResource(&meshIterator->first);
			}
		});

	AfterglowModelAsset modelAsset(assetInfo);
	auto& device = commandPool().device();
	for (uint32_t index = 0; index < modelAsset.numMeshes(); ++index) {
		auto& indexBuffer = mesh.indexBuffers.emplace_back();
		mesh.vertexBuffers.emplace_back(std::make_unique<VertexBuffer>(device));

		indexBuffer.recreate(device);
		VertexBuffer& vertexBuffer = *reinterpret_cast<VertexBuffer*>(mesh.vertexBuffers.back().get());
		(*indexBuffer).bind(modelAsset.indices(index));
		vertexBuffer.bind(modelAsset.vertexData(index));
		(*indexBuffer).submit(commandPool(), graphicsQueue());
		vertexBuffer.submit(commandPool(), graphicsQueue());

		mesh.vertexBufferHandles.emplace_back(vertexBuffer.handle());
	}
	return &mesh;
}
