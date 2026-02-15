#include "AfterglowStaticMeshComponent.h"
//#include "AfterglowMeshResource.h"
//
//// Nothing to do, just handle the std::unique_ptr<AfterglowMeshResource>.
//AfterglowStaticMeshComponent::AfterglowStaticMeshComponent() = default; 
//AfterglowStaticMeshComponent::~AfterglowStaticMeshComponent() = default;
//
//AfterglowStaticMeshComponent::AfterglowStaticMeshComponent(AfterglowStaticMeshComponent&& rval) noexcept = default;
//AfterglowStaticMeshComponent& AfterglowStaticMeshComponent::operator=(AfterglowStaticMeshComponent&& rval) noexcept = default;
//
//AfterglowStaticMeshComponent::AfterglowStaticMeshComponent(const AfterglowStaticMeshComponent& other) : 
//	AfterglowComponent(other),
//	_assetInfo(other._assetInfo),
//	_materialNames(other._materialNames),
//	_meshResource(nullptr),
//	_meshDated(true) {
//}
//
//AfterglowStaticMeshComponent& AfterglowStaticMeshComponent::operator=(const AfterglowStaticMeshComponent& other) {
//	AfterglowComponent::operator=(other);
//	_assetInfo = other._assetInfo;
//	_materialNames = other._materialNames;
//	_meshResource = nullptr;
//	_meshDated = true;
//	return *this;
//}

//AfterglowStaticMeshComponent::AfterglowStaticMeshComponent(AfterglowStaticMeshComponent&& rval) noexcept : 
//	AfterglowComponent(rval), 
//	_assetInfo(std::move(rval._assetInfo)), 
//	_materialNames(std::move(rval._materialNames)), 
//	_meshResource(std::move(rval._meshResource)), 
//	_meshDated(std::move(rval._meshDated)) {
//}
//
//AfterglowStaticMeshComponent& AfterglowStaticMeshComponent::operator=(AfterglowStaticMeshComponent&& rval) noexcept {
//	AfterglowComponent::operator=(rval);
//	_assetInfo = std::move(rval._assetInfo);
//	_materialNames = std::move(rval._materialNames);
//	_meshResource = std::move(rval._meshResource);
//	_meshDated = std::move(rval._meshDated);
//	return *this;
//}

//const std::string& AfterglowStaticMeshComponent::materialName(SlotID id) const {
//	if (_materialNames.find(id) == _materialNames.end()) {
//		if (_materialNames.empty()) {
//			return emptyMaterialName;
//		}
//		return _materialNames.at(0);
//	}
//	return _materialNames.at(id);
//}
//
//const AfterglowStaticMeshComponent::MaterialNames& AfterglowStaticMeshComponent::materialNames() const noexcept {
//	return _materialNames;
//}

const std::string& AfterglowStaticMeshComponent::modelPath() const noexcept {
	return _assetInfo.path;
}

model::ImportFlag AfterglowStaticMeshComponent::modelImportFlags() const noexcept {
	return _assetInfo.importFlags;
}

const model::AssetInfo& AfterglowStaticMeshComponent::modelAssetInfo() const noexcept {
	return _assetInfo;
}

//void AfterglowStaticMeshComponent::setMaterial(const std::string& materialName, SlotID id) {
//	_materialNames[id] = materialName;
//}

void AfterglowStaticMeshComponent::setModel(const std::string& modelPath) {
	if (_assetInfo.path != modelPath) {
		_assetInfo.path = modelPath;
		_meshDated = true;
	}
}

bool AfterglowStaticMeshComponent::meshDated() const noexcept {
	return _meshDated;
}

void AfterglowStaticMeshComponent::setMeshDated(bool isDated) noexcept {
	_meshDated = isDated;
}

void AfterglowStaticMeshComponent::addImportFlags(model::ImportFlag flags)  noexcept {
	_assetInfo.importFlags |= flags;
	_meshDated = true;
}

void AfterglowStaticMeshComponent::removeImportFlags(model::ImportFlag flags)  noexcept {
	_assetInfo.importFlags &= ~flags;
	_meshDated = true;
}
