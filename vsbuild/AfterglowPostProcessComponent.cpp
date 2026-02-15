#include "AfterglowPostProcessComponent.h"
#include "AfterglowShapeMeshComponent.h"
#include "AfterglowComputeComponent.h"
#include "AfterglowPassManager.h"
#include "AfterglowBloomPassSet.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowMaterialUtilities.h"
#include "AfterglowUtilities.h"
#include "DebugUtilities.h"

void AfterglowPostProcessComponent::setBloomIntensity(uint32_t downSamplingIndex, glm::vec4 intensity) {
	auto* horizontalBlurMatInstName = _bloomPassSet->horizontalBlurMaterialName(downSamplingIndex);
	auto* verticalBlurCombinationMatInstName = _bloomPassSet->verticalBlurCombinationMaterialInstanceName(downSamplingIndex);

	if (!horizontalBlurMatInstName || !verticalBlurCombinationMatInstName) {
		DEBUG_CLASS_WARNING("Bloom downsampling level is out of range. ");
		return;
	}

	auto* horizontalBlurMatInst = sysUtils().materialInstance(*horizontalBlurMatInstName);
	auto* verticalBlurCombinationMatInst = sysUtils().materialInstance(*verticalBlurCombinationMatInstName);

	if (!horizontalBlurMatInst || !verticalBlurCombinationMatInst) {
		DEBUG_CLASS_WARNING("Bloom material instance not found.");
		return;
	}

	horizontalBlurMatInst->setVector(shader::Stage::Fragment, AfterglowBloomPassSet::ParamName::bloomIntensity, intensity);
	verticalBlurCombinationMatInst->setVector(shader::Stage::Fragment, AfterglowBloomPassSet::ParamName::bloomIntensity, intensity);
	sysUtils().submitMaterialInstance(*horizontalBlurMatInstName);
	sysUtils().submitMaterialInstance(*verticalBlurCombinationMatInstName);
}

void AfterglowPostProcessComponent::enableBloom() noexcept {
	if (_bloomPassSet) {
		_bloomPassSet->enable();
		setMaterialInstanceScalar("bloomEnabled", 1.0f);
	}
}

void AfterglowPostProcessComponent::disableBloom() noexcept {
	if (_bloomPassSet) {
		_bloomPassSet->disable();
		setMaterialInstanceScalar("bloomEnabled", 0.0f);
	}
}

void AfterglowPostProcessComponent::onEnable() {
	entity().get<AfterglowShapeMeshComponent>().setMaterial(_postProcessMaterialName);
}

void AfterglowPostProcessComponent::onDisable() {
	entity().get<AfterglowShapeMeshComponent>().setMaterial(mat::EmptyPostProcessMaterialName());
}

void AfterglowPostProcessComponent::onRenderBegin() {
	auto& shapeMesh = entity().get<AfterglowShapeMeshComponent>();
	_bloomPassSet = &sysUtils().passManager().installCustomPassSet<AfterglowBloomPassSet>(
		sysUtils(), 
		*shapeMesh.meshResource(), 
		render::Domain::Transparency, 
		render::Domain::PostProcess
	);
}

inline void AfterglowPostProcessComponent::setMaterialInstanceScalar(const std::string& name, float value) {
	auto* postProcessMat = sysUtils().materialInstance(_postProcessMaterialName);
	if (postProcessMat) {
		postProcessMat->setScalar(shader::Stage::Fragment, name, value);
		sysUtils().submitMaterialInstance(_postProcessMaterialName);
	}
}

void AfterglowPostProcessComponent::awake() {
	// Register internal materials
	_postProcessMaterialName = sysUtils().registerMaterialAsset("Assets/Shared/Materials/PostProcess.mat");

	// Create dependent components
	auto& shapeMesh = static_cast<AfterglowShapeMeshComponent&>(
		sysUtils().addComponent(entity(), util::TypeIndex<AfterglowShapeMeshComponent>())
	);
	shapeMesh.setMaterial(_postProcessMaterialName);
	auto& compute = static_cast<AfterglowComputeComponent&>(
		sysUtils().addComponent(entity(), util::TypeIndex<AfterglowComputeComponent>())
	);
	compute.setComputeMaterial(_postProcessMaterialName);
}

//AfterglowPassSetBase* AfterglowPostProcessComponent::bloomSubpassSet() {
//	auto* shapeMesh = entity().component<AfterglowShapeMeshComponent>();
//	if (!shapeMesh) {
//		DEBUG_CLASS_WARNING("Renderable component not found.");
//		return nullptr;
//	}
//	return shapeMesh->customPassSet().get();
//}
