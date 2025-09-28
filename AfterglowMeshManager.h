#pragma once
#include "AfterglowObject.h"

#include "AfterglowComponentPool.h"
#include "AfterglowSharedMeshPool.h"
#include "AfterglowMeshResource.h"
#include "AfterglowShape.h"

class AfterglowMeshManager : public AfterglowObject {
public:
	// TODO: SharedModelAsset {path, {refCount, IndexVertexBuffer}}

	struct ShapeMesh {
		const std::string* materialName;
		std::unique_ptr<AfterglowObject> shape;
		AfterglowMeshResource resource;
	};

	struct ComputeMeshInfo {
		const std::string* materialName;
		ubo::MeshUniform uniform;
	};

	using MeshResources = std::unordered_map<AfterglowStaticMeshComponent::ID, AfterglowMeshResource>;
	// Use for draw mesh from code directly, instead of load from asset.
	using ShapeMeshes = std::vector<ShapeMesh>;
	using ComputeMeshInfos = std::unordered_map<AfterglowComputeComponent::ID, ComputeMeshInfo>;

	AfterglowMeshManager(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	// @return: Shape mesh index.
	template<shape::ShapeType Type>
	uint32_t addShapeMesh();

	ShapeMesh& shapeMesh(uint32_t index);

	AfterglowDevice& device();
	MeshResources& meshResources();
	ShapeMeshes& shapeMeshes();

	const ComputeMeshInfos& computeMeshInfos() const;

	void update(AfterglowComponentPool& componentPool, const AfterglowCameraComponent& camera);
	void updateComputeMesheInfos(AfterglowComponentPool& componentPool, const AfterglowCameraComponent& camera);

private:
	// TODO: Optimize this ugly method.
	// @brief: register a mesh buffer, if buffer exists, return it.
	// @desc: call this function every frame is the static mesh is exists.
	// @return:  activate successfully.
	bool activateStaticMesh(const AfterglowStaticMeshComponent& staticMesh);

	// @brief: unregister a mesh buffer from manager.
	void removeStaticMesh(AfterglowStaticMeshComponent::ID id);

	void calculateMeshUniform(
		const AfterglowTransformComponent& transform,
		const AfterglowCameraComponent& camera, 
		ubo::MeshUniform& destMeshUnifrom
	);

	AfterglowSharedMeshPool _meshPool;
	MeshResources _meshResources;
	ShapeMeshes _shapeMeshes;
	ComputeMeshInfos _computeMeshInfos;
};

template<shape::ShapeType Type>
inline uint32_t AfterglowMeshManager::addShapeMesh() {

	auto& shapeMesh = _shapeMeshes.emplace_back(
		nullptr, 
		std::make_unique<Type>( _meshPool.commandPool(), _meshPool.graphicsQueue() ), 
		AfterglowMeshResource{ AfterglowMeshResource::Mode::Custom}
	);

	Type& shape = *reinterpret_cast<Type*>(shapeMesh.shape.get());
	shapeMesh.resource.bindIndexBuffers(shape.indexBuffers());
	shapeMesh.resource.bindVertexBufferHandles(shape.vertexBufferHandles());

	return static_cast<uint32_t>(_shapeMeshes.size() - 1);
}
