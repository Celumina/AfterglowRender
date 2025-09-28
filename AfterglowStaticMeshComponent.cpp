#include "AfterglowStaticMeshComponent.h"

const std::string& AfterglowStaticMeshComponent::materialName(SlotID id) const {
	if (_materialNames.find(id) == _materialNames.end()) {
		if (_materialNames.empty()) {
			return emptyMaterialName;
		}
		return _materialNames.at(0);
	}
	return _materialNames.at(id);
}

const AfterglowStaticMeshComponent::MaterialNames& AfterglowStaticMeshComponent::materialNames() const {
	return _materialNames;
}

const std::string& AfterglowStaticMeshComponent::modelPath() const {
	return _assetInfo.path;
}

model::ImportFlag AfterglowStaticMeshComponent::modelImportFlags() const {
	return _assetInfo.importFlags;
}

const model::AssetInfo& AfterglowStaticMeshComponent::modelAssetInfo() const {
	return _assetInfo;
}

void AfterglowStaticMeshComponent::setMaterial(const std::string& materialName, SlotID id) {
	_materialNames[id] = materialName;
}

void AfterglowStaticMeshComponent::setModel(const std::string& modelPath) {
	if (_assetInfo.path != modelPath) {
		_assetInfo.path = modelPath;
		_meshDated = true;
	}
}

bool AfterglowStaticMeshComponent::meshDated() const {
	return _meshDated;
}

void AfterglowStaticMeshComponent::setMeshDated(bool isDated) {
	_meshDated = isDated;
}

void AfterglowStaticMeshComponent::addImportFlags(model::ImportFlag flags) {
	_assetInfo.importFlags |= flags;
	_meshDated = true;
}

void AfterglowStaticMeshComponent::removeImportFlags(model::ImportFlag flags) {
	_assetInfo.importFlags &= ~flags;
	_meshDated = true;
}
