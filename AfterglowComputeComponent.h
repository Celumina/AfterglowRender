#pragma once
#include "AfterglowComponent.h"

// For GPU Generic Computation.
class AfterglowComputeComponent : public AfterglowComponent<AfterglowComputeComponent> {
public:
	void setComputeMaterial(const std::string materialName);
	const std::string& computeMaterialName() const;

private: 
	std::string _materialName;
};

