#pragma once

#include <memory>
#include "AfterglowMaterialInstance.h"

class AfterglowMaterialInstanceAsset {
public:
	AfterglowMaterialInstanceAsset(const std::string& path);
	~AfterglowMaterialInstanceAsset();

	// @desc: 
	// If failed to get material instance name, return empty str "".
	std::string materialnstanceName() const;
	// @desc: 
	// If failed to get parent material name, return empty str "".
	std::string parentMaterialName() const;

	// @brief: Fill parameters to destMaterialInstance
	void fill(AfterglowMaterialInstance& destMaterialInstance) const;

private:
	struct Context;
	std::unique_ptr<Context> _context;
};

