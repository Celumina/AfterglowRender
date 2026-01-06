#pragma once
#include "AfterglowActionComponent.h"
#include "UniformBufferObjects.h"
#include "ComputeDefinitions.h"

// For GPU Generic Computation.
class AfterglowComputeComponent : public AfterglowActionComponent<AfterglowComputeComponent> {
public:
	inline void setComputeMaterial(const std::string materialName) { _materialName = materialName; }
	inline const std::string& computeMaterialName() const noexcept { return _materialName; }
	
	// @note: Modify by MeshManager.
	inline ubo::MeshUniform& meshUniform() noexcept { return _meshUniform; }
	inline const ubo::MeshUniform& meshUniform() const noexcept { return _meshUniform; }

	// @warning: Make sure the compute material is ready before invoke it.
	void applyDispatchFrequency(compute::DispatchFrequency frequency) noexcept;

private: 
	ubo::MeshUniform _meshUniform;
	std::string _materialName;
};

INR_CLASS(AfterglowComputeComponent) {
	INR_BASE_CLASSES<AfterglowActionComponent<AfterglowComputeComponent>>;
	INR_FUNCS(
		INR_FUNC(setComputeMaterial), 
		INR_FUNC(computeMaterialName), 
		INR_OVERLOADED_FUNC(ubo::MeshUniform& (AfterglowComputeComponent::*)(), meshUniform),
		INR_OVERLOADED_FUNC(const ubo::MeshUniform& (AfterglowComputeComponent::*)() const, meshUniform), 
		INR_FUNC(applyDispatchFrequency)
	);
};