#include "AfterglowMeshManager.h"

AfterglowMeshManager::AfterglowMeshManager(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) : 
	_meshPool(commandPool, graphicsQueue) {
}

AfterglowMeshManager::ShapeMesh& AfterglowMeshManager::shapeMesh(uint32_t index) {
	return _shapeMeshes[index];
}

AfterglowDevice& AfterglowMeshManager::device() {
	return _meshPool.commandPool().device();
}

AfterglowMeshManager::MeshResources& AfterglowMeshManager::meshResources() {
	return _meshResources;
}

AfterglowMeshManager::ShapeMeshes& AfterglowMeshManager::shapeMeshes() {
	return _shapeMeshes;
}

const AfterglowMeshManager::ComputeMeshInfos& AfterglowMeshManager::computeMeshInfos() const {
	return _computeMeshInfos;
}

bool AfterglowMeshManager::activateStaticMesh(const AfterglowStaticMeshComponent& staticMesh) {
	if (!staticMesh.enabled() || staticMesh.modelPath().empty()) {
		return false;
	}
	AfterglowMeshResource* meshResource = nullptr;
	if (_meshResources.find(staticMesh.id()) == _meshResources.end()) {
		meshResource = &_meshResources.emplace(
			staticMesh.id(), AfterglowMeshResource{ AfterglowMeshResource::Mode::SharedPool}
		).first->second;
	}
	else {
		meshResource = &_meshResources.at(staticMesh.id());
	}
	meshResource->setActivated(true);
	return true;
}

void AfterglowMeshManager::removeStaticMesh(AfterglowStaticMeshComponent::ID id) {
	_meshResources.erase(id);
}

void AfterglowMeshManager::calculateMeshUniform(
	const AfterglowTransformComponent& transform,
	const AfterglowCameraComponent& camera, 
	ubo::MeshUniform& destMeshUnifrom) {

	destMeshUnifrom.model = transform.globalTransformMatrix();
	
	// For normal calculation
	destMeshUnifrom.invTransModel = glm::transpose(glm::inverse(destMeshUnifrom.model));

	// View
	destMeshUnifrom.view = camera.view();

	// Projection
	destMeshUnifrom.projection = camera.perspective();

	// Inverse matrices
	destMeshUnifrom.invView = glm::inverse(destMeshUnifrom.view);
	destMeshUnifrom.invProjection = glm::inverse(destMeshUnifrom.projection);

	// ID
	destMeshUnifrom.objectID = static_cast<uint32_t>(transform.id());
}

void AfterglowMeshManager::update(AfterglowComponentPool& componentPool, const AfterglowCameraComponent& camera) {
	// Active Meshes
	// TODO: remove componentPool ref dependency.
	auto& staticMeshes = componentPool.components<AfterglowStaticMeshComponent>();
	
	for (auto& staticMesh : staticMeshes) {
		// Call every frame to keep a mesh exists in rendering;
		activateStaticMesh(staticMesh);
	}

	for (auto& [id, meshResource] : _meshResources) {
		auto* staticMesh = componentPool.component<AfterglowStaticMeshComponent>(id);
		// Remove mesh if it's not longer exists.
		if (!staticMesh || !meshResource.activated()) {
			removeStaticMesh(id);
			continue;
		}
		// Cache static mesh handle to avoid repeatly search.
		meshResource.bindStaticMesh(*staticMesh);

		calculateMeshUniform(
			staticMesh->entity().get<AfterglowTransformComponent>(), camera, meshResource.meshUniform()
		);

		// Load asset if mesh resource is changed.
		if (staticMesh->meshDated()) {
			meshResource.setMeshReference(_meshPool.mesh(staticMesh->modelAssetInfo()));
			// loadModelAsset(*staticMesh, meshResource);
			staticMesh->setMeshDated(false);
		}

		// Update status.
		meshResource.setActivated(false);
	}
}

void AfterglowMeshManager::updateComputeMesheInfos(AfterglowComponentPool& componentPool, const AfterglowCameraComponent& camera) {
	const auto& computeComponents = componentPool.components<AfterglowComputeComponent>();

	// Method 0
	// Clear no longer exist compute mesh uniforms. 
	std::erase_if(_computeMeshInfos, [&componentPool](const auto& item){
		auto* computeComponent = componentPool.component<AfterglowComputeComponent>(item.first);
		return !computeComponent;
	});

	// Method 1
	// Not support, due to material manager use meshuniform address to recreate uniform buffer.
	// Mesh uniform is not very large, just clear and rebuild it. 
	// _computeMeshInfos.clear();

	for (const auto& computeComponent : computeComponents) {
		if (computeComponent.enabled()) {
			const auto& transform = computeComponent.entity().get<AfterglowTransformComponent>();

			auto& computeMeshInfo = _computeMeshInfos[computeComponent.id()];
			computeMeshInfo.materialName = &computeComponent.computeMaterialName();
			calculateMeshUniform(transform, camera, computeMeshInfo.uniform);
		}
	}
}
