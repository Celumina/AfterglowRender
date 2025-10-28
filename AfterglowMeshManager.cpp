#include "AfterglowMeshManager.h"

#include "AfterglowComponentPool.h"

AfterglowMeshManager::AfterglowMeshManager(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) : 
	_meshPool(commandPool, graphicsQueue) {
}

AfterglowMeshManager::ShapeMesh& AfterglowMeshManager::shapeMesh(uint32_t index) {
	return _shapeMeshes[index];
}

AfterglowDevice& AfterglowMeshManager::device() noexcept {
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

bool AfterglowMeshManager::activateStaticMesh(AfterglowStaticMeshComponent& staticMesh) {
	if (!staticMesh.enabled() || staticMesh.modelPath().empty()) {
		return false;
	}
	AfterglowMeshResource* meshResource = nullptr;
	if (_meshResources.find(staticMesh.id()) == _meshResources.end()) {
		meshResource = &_meshResources.emplace(
			staticMesh.id(), AfterglowMeshResource{ AfterglowMeshResource::Mode::SharedPool }
		).first->second;
		staticMesh.setMeshDated(true);
	}
	else {
		meshResource = &_meshResources.at(staticMesh.id());
	}
	meshResource->bindStaticMesh(staticMesh);
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
	destMeshUnifrom.projection = camera.perspective();
	destMeshUnifrom.invView = camera.invView();
	destMeshUnifrom.invProjection = camera.invPerspective();;

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

	// Remove mesh if it's not longer exists.
	std::erase_if(_meshResources, [](const auto& item){ return !item.second.activated(); });

	for (auto& [id, meshResource] : _meshResources) {
		// Cache static mesh handle to avoid repeatly search.
		auto& staticMesh = meshResource.staticMesh();

		calculateMeshUniform(
			staticMesh.entity().get<AfterglowTransformComponent>(), camera, meshResource.meshUniform()
		);

		// Load asset if mesh resource is changed.
		if (staticMesh.meshDated()) {
			meshResource.setMeshReference(_meshPool.mesh(staticMesh.modelAssetInfo()));
			// loadModelAsset(*staticMesh, meshResource);
			staticMesh.setMeshDated(false);
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
		return !computeComponent || !computeComponent->enabled();
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
