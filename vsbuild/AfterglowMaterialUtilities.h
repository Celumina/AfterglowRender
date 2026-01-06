#pragma once
#include <string>
#include "AfterglowMaterialAsset.h"

namespace mat {
	const AfterglowMaterialAsset& ErrorMaterialAsset();
	const std::string& ErrorMaterialName();

	const AfterglowMaterialAsset& EmptyPostProcessMaterialAsset();
	const std::string& EmptyPostProcessMaterialName();
}