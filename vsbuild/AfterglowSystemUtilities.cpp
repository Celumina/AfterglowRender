#include "AfterglowSystemUtilities.h"
#include "AfterglowSystem.h"
#include "AfterglowMaterial.h"
#include "AfterglowWindow.h"
#include "AfterglowMaterialManager.h"
#include "AfterglowCameraComponent.h"
#include "AfterglowTicker.h"
#include "UniformBufferObjects.h"
#include "LocalClock.h"

struct AfterglowSystemUtilities::Impl {
	AfterglowSystem* system = nullptr;
	AfterglowMaterialManager* materialManager = nullptr;
	AfterglowTicker* ticker = nullptr;
};

AfterglowSystemUtilities::AfterglowSystemUtilities(AfterglowSystem* system) :
	_impl(std::make_unique<AfterglowSystemUtilities::Impl>()){
	_impl->system = system;
	_impl->materialManager = &system->materialManager();
	_impl->ticker = &system->ticker();
}

AfterglowSystemUtilities::~AfterglowSystemUtilities() {
}

std::weak_ptr<AfterglowScene> AfterglowSystemUtilities::scene() const noexcept {
	return _impl->system->scene();
}

AfterglowEntity& AfterglowSystemUtilities::createEntity(const std::string& name, std::type_index componentTypeIndex, util::OptionalRef<AfterglowEntity> parent) const {
	auto& entity = _impl->system->createEntity<>(name, parent);
	_impl->system->addComponent(entity, componentTypeIndex);
	return entity;
}

AfterglowEntity& AfterglowSystemUtilities::createEntity(const std::string& name, const TypeIndexArray& componentTypeIndices, util::OptionalRef<AfterglowEntity> parent) const {
	auto& entity = _impl->system->createEntity<>(name, parent);
	for (const auto& componentTypeIndex : componentTypeIndices) {
		_impl->system->addComponent(entity, componentTypeIndex);
	}
	return entity;
}

bool AfterglowSystemUtilities::destroyEntity(AfterglowEntity& entity) const {
	return _impl->system->destroyEntity(entity);
}

AfterglowEntity* AfterglowSystemUtilities::findEntity(const std::string& name) const {
	auto entites = _impl->system->scene().lock()->findEntities(name);
	if (!entites.empty()) {
		return entites[0];
	}
	return nullptr;
}

std::vector<AfterglowEntity*> AfterglowSystemUtilities::findEntities(const std::string& name) const {
	return _impl->system->scene().lock()->findEntities(name);
}

AfterglowEntity& AfterglowSystemUtilities::createStaticMeshEntity(const std::string& modelPath, const std::string& materialName) const {
	auto& entity = _impl->system->createEntity<AfterglowStaticMeshComponent>(modelPath);
	auto& staicMesh = entity.get<AfterglowStaticMeshComponent>();
	staicMesh.setModel(modelPath);
	staicMesh.setMaterial(materialName);
	return entity;
}

AfterglowComponentBase& AfterglowSystemUtilities::addComponent(AfterglowEntity& destEntity, std::type_index componentTypeIndex) const {
	AfterglowComponentBase* component = nullptr;
	reg::AsType(componentTypeIndex, [&]<typename ComponentType>() {
		component = &_impl->system->addComponent<ComponentType>(destEntity);
	});
	return *component;
}

AfterglowCameraComponent* AfterglowSystemUtilities::mainCamera() const {
	return _impl->system->mainCamera();
}

const AfterglowInput& AfterglowSystemUtilities::input() const {
	return _impl->system->input();
}

void AfterglowSystemUtilities::lockCursor() const {
	_impl->system->window().lockCursor();
}

void AfterglowSystemUtilities::unlockCursor() const {
	_impl->system->window().unlockCursor();
}

std::string AfterglowSystemUtilities::registerMaterialAsset(const std::string& materialPath) const {
	return _impl->materialManager->registerMaterialAsset(materialPath);
}

std::string AfterglowSystemUtilities::registerMaterialInstanceAsset(const std::string& materialInstancePath) const {
	return _impl->materialManager->registerMaterialInstanceAsset(materialInstancePath);
}

void AfterglowSystemUtilities::unregisterMaterialAsset(const std::string& materialPath) const {
	return _impl->materialManager->unregisterMaterialAsset(materialPath);
}

void AfterglowSystemUtilities::unregisterMaterialInstanceAsset(const std::string& materialInstancePath) const {
	return _impl->materialManager->unregisterMaterialInstanceAsset(materialInstancePath);
}

AfterglowMaterial& AfterglowSystemUtilities::createMaterial(const std::string& name, util::OptionalRef<AfterglowMaterial> sourceMaterial) const {
	return _impl->materialManager->createMaterial(name, sourceMaterial);
}

AfterglowMaterialInstance& AfterglowSystemUtilities::createMaterialInstance(const std::string& name, const std::string& parentMaterialName) const {
	return _impl->materialManager->createMaterialInstance(name, parentMaterialName);
}

AfterglowMaterial* AfterglowSystemUtilities::material(const std::string& name) const {
	return _impl->materialManager->material(name);
}

AfterglowMaterial* AfterglowSystemUtilities::findMaterialByInstanceName(const std::string& name) const {
	return _impl->materialManager->findMaterialByInstanceName(name);
}

AfterglowMaterialInstance* AfterglowSystemUtilities::materialInstance(const std::string& name) const {
	return _impl->materialManager->materialInstance(name);
}

const ubo::GlobalUniform& AfterglowSystemUtilities::globalUniform() const noexcept {
	return _impl->materialManager->globalUniform();
}

double AfterglowSystemUtilities::fps() const noexcept {
	return _impl->ticker->clock().fps();
}

double AfterglowSystemUtilities::timeSec() const noexcept {
	return _impl->ticker->clock().timeSec();
}

double AfterglowSystemUtilities::deltaTimeSec() const noexcept {
	return _impl->ticker->clock().deltaTimeSec();
}

float AfterglowSystemUtilities::maximumFPS() const noexcept {
	return _impl->ticker->maximumFPS();
}

void AfterglowSystemUtilities::setMaximumFPS(float fps) noexcept {
	return _impl->ticker->setMaximumFPS(fps);
}
