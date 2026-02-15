#pragma once
#include <string>

class AfterglowMaterialAsset;

namespace mat {
	const AfterglowMaterialAsset& ErrorMaterialAsset();
	const std::string& ErrorMaterialName();

	const AfterglowMaterialAsset& EmptyPostProcessMaterialAsset();
	const std::string& EmptyPostProcessMaterialName();
}