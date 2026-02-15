#include "AfterglowMeshManager.h"

#include "AfterglowRenderableContext.h"
#include "AfterglowShapeMeshResource.h"

AfterglowMeshManager::AfterglowMeshManager(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowSynchronizer& synchronizer) :
	_meshPool(commandPool, graphicsQueue, synchronizer) {
}

AfterglowDevice& AfterglowMeshManager::device() noexcept {
	return _meshPool.commandPool().device();
}

inline void AfterglowMeshManager::fillMeshUniform(const AfterglowTransformComponent& transform, ubo::MeshUniform& destMeshUnifrom) {
	// Transform matrix calculated in System thread, This function is almost zero overhead.
	destMeshUnifrom.model = transform.globalTransformMatrix();
	// For normal calculation
	destMeshUnifrom.invTransModel = transform.globalInvTransTransformMatrix();
	destMeshUnifrom.objectID = static_cast<uint32_t>(transform.id());
}

inline void AfterglowMeshManager::fillMeshUniformAABB(AfterglowMeshResource& resource) {
	auto* aabb = resource.aabb();
	if (aabb) {
		resource.meshUniform().maxAABB = { aabb->max[0], aabb->max[1], aabb->max[2] };
		resource.meshUniform().minAABB = { aabb->min[0], aabb->min[1], aabb->min[2] };
	}
}

void AfterglowMeshManager::updateMeshUniformResourceInfo(AfterglowStaticMeshComponent& staticMesh, AfterglowComputeComponent* compute) {
	uint32_t indexCount = (*staticMesh.meshResource()->indexBuffers()[0]).indexCount();
	staticMesh.meshResource()->meshUniform().indexCount = indexCount;
	if (compute) {
		compute->meshUniform().indexCount = indexCount;
	}
}

void AfterglowMeshManager::updateUniforms(AfterglowRenderableContext& renderableContext) {
	auto& componentPool = renderableContext.componentPool;
	
	// Static Meshes
	auto& staticMeshes = componentPool.components<AfterglowStaticMeshComponent>();
	for (auto& staticMesh : staticMeshes) {
		auto& meshResource = staticMesh.meshResource();
		if (!meshResource) {
			meshResource = std::make_unique<AfterglowMeshResource>(AfterglowMeshResource::Mode::SharedPool);
		}
		if (staticMesh.enabled()) {
			fillMeshUniform(staticMesh.entity().get<AfterglowTransformComponent>(), meshResource->meshUniform());
		}
	}

	// Shape meshes
	auto& shapeMeshes = componentPool.components<AfterglowShapeMeshComponent>();
	for (auto& shapeMesh : shapeMeshes) {
		auto& meshResource = shapeMesh.meshResource();
		if (!meshResource) {
			meshResource = std::make_unique<AfterglowShapeMeshResource>();
			// shape mesh could not change the resource, so just initialize it as soon as the mesh resource initialized. 
			if (shapeMesh.shape() == AfterglowShapeMeshComponent::Shape::NDCRetangle) {
				reinterpret_cast<AfterglowShapeMeshResource*>(meshResource.get())->initializeShape<shape::NDCRectangle>(
					_meshPool.commandPool(), _meshPool.graphicsQueue()
				);
			}
		}
		if (shapeMesh.enabled()) {
			fillMeshUniform(shapeMesh.entity().get<AfterglowTransformComponent>(), meshResource->meshUniform());
		}
	}

	// Compute Components
	auto& computeComponents = componentPool.components<AfterglowComputeComponent>();
	for (auto& computeComponent : computeComponents) {
		if (computeComponent.enabled()) {
			const auto& transform = computeComponent.entity().get<AfterglowTransformComponent>();
			fillMeshUniform(transform, computeComponent.meshUniform());
		}
	}

	//// Post Process
	//auto* processProcess = renderableContext.postProcess;
	//if (processProcess && processProcess->enabled()) {
	//	auto& shapeResource = processProcess->shapeResource();
	//	if (!shapeResource) {
	//		shapeResource = std::make_unique<AfterglowShapeMeshResource>();
	//		shapeResource->initializeShape<shape::NDCRectangle>(_meshPool.commandPool(), _meshPool.graphicsQueue());
	//	}
	//}
}

void AfterglowMeshManager::updateResources(AfterglowRenderableContext& renderableContext) {
	auto& componentPool = renderableContext.componentPool;

	// Static Meshes
	auto& staticMeshes = componentPool.components<AfterglowStaticMeshComponent>();
	for (auto& staticMesh : staticMeshes) {
		if (staticMesh.meshResource() && staticMesh.meshDated()) {
			staticMesh.meshResource()->setMeshReference(_meshPool.mesh(staticMesh.modelAssetInfo()));
			// AABB is updated only if mesh dated.
			fillMeshUniformAABB(*staticMesh.meshResource());
			// Update mesh uniform resource info
			updateMeshUniformResourceInfo(staticMesh, staticMesh.entity().component<AfterglowComputeComponent>());
			staticMesh.setMeshDated(false);
		}
	}

	// Update real reasource from the Mesh Pool.
	_meshPool.update();
}
