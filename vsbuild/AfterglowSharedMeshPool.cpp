#include "AfterglowSharedMeshPool.h"



AfterglowMeshReference::AfterglowMeshReference(
	const model::AssetInfo& assetInfo, AfterglowResourceReference::Resources& meshes, AfterglowReferenceCount& count
) : AfterglowResourceReference(assetInfo, meshes, count) {
}

AfterglowMeshReference::AfterglowMeshReference(const AfterglowMeshReference& other) :
	AfterglowResourceReference(other) {
}

AfterglowIndexBuffer::Array& AfterglowMeshReference::indexBuffers() noexcept {
	// verifyValue();
	return _value->indexBuffers;
}

//AfterglowVertexBuffer::Array& AfterglowMeshReference::vertexBuffers() noexcept {
//	// verifyValue();
//	return _value->vertexBuffers;
//}

std::vector<AfterglowVertexBufferHandle>& AfterglowMeshReference::vertexBufferHandles() noexcept {
	// verifyValue();
	return _value->vertexBufferHandles;
}

const model::AABB& AfterglowMeshReference::aabb() const noexcept {
	return _value->aabb;
}

// @deprecated: std::unordered_map rehash will not change the element address.
//const model::AssetInfo& AfterglowMeshReference::assetInfo() const {
//	return _key;
//}

AfterglowSharedMeshPool::AfterglowSharedMeshPool(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowSynchronizer& synchronizer) :
	AfterglowSharedResourcePool(commandPool, graphicsQueue, synchronizer) {
}

AfterglowMeshReference AfterglowSharedMeshPool::mesh(const model::AssetInfo& assetInfo) {
	auto meshIterator = _resources.find(assetInfo);
	Resource* mesh = nullptr;
	if (meshIterator == _resources.end()) {
		AfterglowModelAsset::AsType(assetInfo.importFlags, [this, &mesh, &assetInfo]<typename VertexType>(){
			mesh = createMesh<VertexType>(assetInfo);
		});
	}
	else {
		mesh = &(meshIterator->second);
	}
	return AfterglowMeshReference{assetInfo, _resources, mesh->count};
}
