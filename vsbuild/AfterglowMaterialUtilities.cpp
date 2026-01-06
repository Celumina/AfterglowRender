#include "AfterglowMaterialUtilities.h"
#include "AfterglowMaterial.h"


const AfterglowMaterialAsset& mat::ErrorMaterialAsset() {
	static AfterglowMaterialAsset errorMaterialAsset(AfterglowMaterial::errorMaterial());
	return errorMaterialAsset;
}

const std::string& mat::ErrorMaterialName() {
	static std::string name("__ERROR__");
	return name;
}

const AfterglowMaterialAsset& mat::EmptyPostProcessMaterialAsset() {
	static AfterglowMaterialAsset emptyPostProcessMaterialAsset(AfterglowMaterial::emptyPostProcessMaterial());
	return emptyPostProcessMaterialAsset;
}

const std::string& mat::EmptyPostProcessMaterialName() {
	static std::string name("__EMPTY_POSTPROCESS__");
	return name;
}
