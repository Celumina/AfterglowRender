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

INR_CLASS(AfterglowComputeComponent) {
	INR_BASE_CLASSES<AfterglowComponent<AfterglowComputeComponent>>;
	INR_FUNCS(
		INR_FUNC(setComputeMaterial), 
		INR_FUNC(computeMaterialName)
	);
};