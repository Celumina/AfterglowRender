#pragma once

#include "AfterglowSharedResourcePool.h"

#include "AfterglowIndexBuffer.h"
#include "AfterglowVertexBuffer.h"
#include "AssetDefinitions.h"

struct AfterglowMeshPoolResource {
	AfterglowIndexBuffer::Array indexBuffers;
	AfterglowVertexBuffer::Array vertexBuffers;
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

	const model::AssetInfo& assetInfo() const;
	AfterglowIndexBuffer::Array& indexBuffers();
	AfterglowVertexBuffer::Array& vertexBuffers();
	std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles();
};


class AfterglowSharedMeshPool : public AfterglowSharedResourcePool<AfterglowMeshReference> {
public: 
	AfterglowSharedMeshPool(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	// @brief: Get ref of mesh resource, if resource not exists, it will create mesh from file automatically.
	AfterglowMeshReference mesh(const model::AssetInfo& assetInfo);

private:
	Resource* createMesh(const model::AssetInfo& assetInfo);
};
