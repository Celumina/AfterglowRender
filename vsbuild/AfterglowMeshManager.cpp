#include "AfterglowMeshManager.h"

#include "AfterglowRenderableContext.h"
#include "AfterglowShapeMeshResource.h"

AfterglowMeshManager::AfterglowMeshManager(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) : 
	_meshPool(commandPool, graphicsQueue) {
}

AfterglowDevice& AfterglowMeshManager::device() noexcept {
	return _meshPool.commandPool().device();
}

void AfterglowMeshManager::fillMeshUniform(const AfterglowTransformComponent& transform, ubo::MeshUniform& destMeshUnifrom) {
	// Transform matrix calculated in System thread, This function is almost zero overhead.
	destMeshUnifrom.model = transform.globalTransformMatrix();
	// For normal calculation
	destMeshUnifrom.invTransModel = transform.globalInvTransTransformMatrix();
	destMeshUnifrom.objectID = static_cast<uint32_t>(transform.id());
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

	// Compute Components
	auto& computeComponents = componentPool.components<AfterglowComputeComponent>();
	for (auto& computeComponent : computeComponents) {
		if (computeComponent.enabled()) {
			const auto& transform = computeComponent.entity().get<AfterglowTransformComponent>();
			fillMeshUniform(transform, computeComponent.meshUniform());
		}
	}

	// Post Process
	auto* processProcess = renderableContext.postProcess;
	if (processProcess && processProcess->enabled()) {
		auto& shapeResource = processProcess->shapeResource();
		if (!shapeResource) {
			shapeResource = std::make_unique<AfterglowShapeMeshResource>();
			shapeResource->initializeShape<shape::NDCRectangle>(_meshPool.commandPool(), _meshPool.graphicsQueue());
		}
	}
}

void AfterglowMeshManager::updateResources(AfterglowRenderableContext& renderableContext) {
	auto& componentPool = renderableContext.componentPool;

	// Static Meshes
	auto& staticMeshes = componentPool.components<AfterglowStaticMeshComponent>();
	for (auto& staticMesh : staticMeshes) {
		if (staticMesh.meshResource() && staticMesh.meshDated()) {
			staticMesh.meshResource()->setMeshReference(_meshPool.mesh(staticMesh.modelAssetInfo()));
			// Update mesh uniform resource info
			updateMeshUniformResourceInfo(staticMesh, staticMesh.entity().component<AfterglowComputeComponent>());
			staticMesh.setMeshDated(false);
		}
	}
	// Update real reasource from the Mesh Pool.
	_meshPool.update();
}
