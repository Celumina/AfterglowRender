#include "AfterglowSystemUtilities.h"
#include "AfterglowSystem.h"


struct AfterglowSystemUtilities::Context {
	AfterglowSystem* system = nullptr;
};

AfterglowSystemUtilities::AfterglowSystemUtilities(void* system) : 
	_context(std::make_unique<AfterglowSystemUtilities::Context>()){
	_context->system = reinterpret_cast<AfterglowSystem*>(system);
}

AfterglowSystemUtilities::~AfterglowSystemUtilities() {
}

AfterglowEntity& AfterglowSystemUtilities::createEntity(const std::string& name, std::type_index componentTypeIndex, util::OptionalRef<AfterglowEntity> parent) const {
	auto& entity = _context->system->createEntity<>(name, parent);
	_context->system->addComponent(entity, componentTypeIndex);
	return entity;
}

AfterglowEntity& AfterglowSystemUtilities::createEntity(const std::string& name, const TypeIndexArray& componentTypeIndices, util::OptionalRef<AfterglowEntity> parent) const {
	auto& entity = _context->system->createEntity<>(name, parent);
	for (const auto& componentTypeIndex : componentTypeIndices) {
		_context->system->addComponent(entity, componentTypeIndex);
	}
	return entity;
}

bool AfterglowSystemUtilities::destroyEntity(AfterglowEntity& entity) const {
	return _context->system->destroyEntity(entity);
}

const AfterglowInput& AfterglowSystemUtilities::input() const {
	return _context->system->input();
}

void AfterglowSystemUtilities::lockCursor() const {
	_context->system->window().lockCursor();
}

void AfterglowSystemUtilities::unlockCursor() const {
	_context->system->window().unlockCursor();
}

std::string AfterglowSystemUtilities::registerMaterialAsset(const std::string& materialPath) const {
	return _context->system->materialManager().registerMaterialAsset(materialPath);
}

std::string AfterglowSystemUtilities::registerMaterialInstanceAsset(const std::string& materialInstancePath) const {
	return _context->system->materialManager().registerMaterialInstanceAsset(materialInstancePath);
}

void AfterglowSystemUtilities::unregisterMaterialAsset(const std::string& materialPath) const {
	return _context->system->materialManager().unregisterMaterialAsset(materialPath);
}

void AfterglowSystemUtilities::unregisterMaterialInstanceAsset(const std::string& materialInstancePath) const {
	return _context->system->materialManager().unregisterMaterialInstanceAsset(materialInstancePath);
}

AfterglowMaterial& AfterglowSystemUtilities::createMaterial(const std::string& name, const AfterglowMaterial& sourceMaterial) const {
	return _context->system->materialManager().createMaterial(name, sourceMaterial);
}

AfterglowMaterialInstance& AfterglowSystemUtilities::createMaterialInstance(const std::string& name, const std::string& parentMaterialName) const {
	return _context->system->materialManager().createMaterialInstance(name, parentMaterialName);
}

AfterglowMaterial* AfterglowSystemUtilities::material(const std::string& name) const {
	return _context->system->materialManager().material(name);
}

AfterglowMaterialInstance* AfterglowSystemUtilities::materialInstance(const std::string& name) const {
	return _context->system->materialManager().materialInstance(name);
}

const ubo::GlobalUniform& AfterglowSystemUtilities::globalUniform() const {
	return _context->system->materialManager().globalUniform();
}

const LocalClock& AfterglowSystemUtilities::clock() const {
	return _context->system->clock();
}
