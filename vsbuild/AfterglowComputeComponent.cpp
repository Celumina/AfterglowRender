#include "AfterglowComputeComponent.h"
#include "AfterglowMaterial.h"
#include "AfterglowComputeTask.h"
#include "DebugUtilities.h"

void AfterglowComputeComponent::applyDispatchFrequency(compute::DispatchFrequency frequency) noexcept {
	if (_materialName.empty()) {
		DEBUG_CLASS_ERROR("The compute material name is empty. ");
		return;
	}
	// TODO: Find instance first...
	auto* material = sysUtils().findMaterialByInstanceName(_materialName);
	if (!material || !material->hasComputeTask()) {
		DEBUG_CLASS_ERROR("The compute material is not found.");
		return;
	}
	material->computeTask().setDispatchFrequency(frequency);
}
