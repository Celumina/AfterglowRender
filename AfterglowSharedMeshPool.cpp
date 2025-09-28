#include "AfterglowSharedMeshPool.h"

#include "AfterglowModelAsset.h"


AfterglowMeshReference::AfterglowMeshReference(
	const model::AssetInfo& assetInfo, AfterglowResourceReference::Resources& meshes, AfterglowReferenceCount& count
) : AfterglowResourceReference(assetInfo, meshes, count) {
}

AfterglowMeshReference::AfterglowMeshReference(const AfterglowMeshReference& other) :
	AfterglowResourceReference(other) {
}

AfterglowIndexBuffer::Array& AfterglowMeshReference::indexBuffers() {
	verifyValue();
	return _value->indexBuffers;
}

AfterglowVertexBuffer::Array& AfterglowMeshReference::vertexBuffers() {
	verifyValue();
	return _value->vertexBuffers;
}

std::vector<AfterglowVertexBufferHandle>& AfterglowMeshReference::vertexBufferHandles() {
	verifyValue();
	return _value->vertexBufferHandles;
}

const model::AssetInfo& AfterglowMeshReference::assetInfo() const {
	return _key;
}


AfterglowSharedMeshPool::AfterglowSharedMeshPool(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) : 
	AfterglowSharedResourcePool(commandPool, graphicsQueue) {
}

AfterglowMeshReference AfterglowSharedMeshPool::mesh(const model::AssetInfo& assetInfo) {
	auto meshIterator = _resources.find(assetInfo);
	Resource* mesh = nullptr;
	if (meshIterator == _resources.end()) {
		mesh = createMesh(assetInfo);
	}
	else {
		mesh = &(meshIterator->second);
	}
	return AfterglowMeshReference{assetInfo, _resources, mesh->count};
}

AfterglowSharedMeshPool::Resource* AfterglowSharedMeshPool::createMesh(const model::AssetInfo& assetInfo) {
	auto meshIterator = _resources.emplace(assetInfo, Resource{}).first;
	auto& mesh = meshIterator->second;
	mesh.count.setDecreaseCallback(
		[this, meshIterator](AfterglowReferenceCount::Count count) {
			if (count <= 0) { 
				DEBUG_CLASS_INFO("Mesh was destroyed: " + meshIterator->first.path);
				_resources.erase(meshIterator);
			}
	});

	AfterglowModelAsset modelAsset(assetInfo);
	for (uint32_t index = 0; index < modelAsset.numMeshes(); ++index) {
		auto& indexBuffer = mesh.indexBuffers.emplace_back();
		auto& vertexBuffer = mesh.vertexBuffers.emplace_back();
		auto& device = commandPool().device();

		indexBuffer.recreate(device);
		vertexBuffer.recreate(device);
		(*indexBuffer).bind(modelAsset.indices(index));
		(*vertexBuffer).bind(modelAsset.vertices(index));
		(*indexBuffer).submit(commandPool(), graphicsQueue());
		(*vertexBuffer).submit(commandPool(), graphicsQueue());

		mesh.vertexBufferHandles.emplace_back((*vertexBuffer).handle());
	}
	return &mesh;
}
