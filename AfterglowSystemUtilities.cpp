#include "AfterglowSystemUtilities.h"
#include "AfterglowSystem.h"
#include "AfterglowMaterial.h"
#include "AfterglowWindow.h"
#include "AfterglowMaterialManager.h"
#include "AfterglowTicker.h"
#include "LocalClock.h"

struct AfterglowSystemUtilities::Impl {
	AfterglowSystem* system = nullptr;
};

AfterglowSystemUtilities::AfterglowSystemUtilities(AfterglowSystem* system) :
	_impl(std::make_unique<AfterglowSystemUtilities::Impl>()){
	_impl->system = system;
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
	return _impl->system->materialManager().registerMaterialAsset(materialPath);
}

std::string AfterglowSystemUtilities::registerMaterialInstanceAsset(const std::string& materialInstancePath) const {
	return _impl->system->materialManager().registerMaterialInstanceAsset(materialInstancePath);
}

void AfterglowSystemUtilities::unregisterMaterialAsset(const std::string& materialPath) const {
	return _impl->system->materialManager().unregisterMaterialAsset(materialPath);
}

void AfterglowSystemUtilities::unregisterMaterialInstanceAsset(const std::string& materialInstancePath) const {
	return _impl->system->materialManager().unregisterMaterialInstanceAsset(materialInstancePath);
}

AfterglowMaterial& AfterglowSystemUtilities::createMaterial(const std::string& name, util::OptionalRef<AfterglowMaterial> sourceMaterial) const {
	return _impl->system->materialManager().createMaterial(name, sourceMaterial);
}

AfterglowMaterialInstance& AfterglowSystemUtilities::createMaterialInstance(const std::string& name, const std::string& parentMaterialName) const {
	return _impl->system->materialManager().createMaterialInstance(name, parentMaterialName);
}

AfterglowMaterial* AfterglowSystemUtilities::material(const std::string& name) const {
	return _impl->system->materialManager().material(name);
}

AfterglowMaterialInstance* AfterglowSystemUtilities::materialInstance(const std::string& name) const {
	return _impl->system->materialManager().materialInstance(name);
}

const ubo::GlobalUniform& AfterglowSystemUtilities::globalUniform() const noexcept {
	return _impl->system->materialManager().globalUniform();
}

double AfterglowSystemUtilities::fps() const noexcept {
	return _impl->system->ticker().clock().fps();
}

double AfterglowSystemUtilities::timeSec() const noexcept {
	return _impl->system->ticker().clock().timeSec();
}

double AfterglowSystemUtilities::deltaTimeSec() const noexcept {
	return _impl->system->ticker().clock().deltaTimeSec();
}

float AfterglowSystemUtilities::maximumFPS() const noexcept {
	return _impl->system->ticker().maximumFPS();
}

void AfterglowSystemUtilities::setMaximumFPS(float fps) noexcept {
	return _impl->system->ticker().setMaximumFPS(fps);
}
