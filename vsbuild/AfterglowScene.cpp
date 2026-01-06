#include "AfterglowScene.h"

AfterglowScene::AfterglowScene(const std::string& sceneName) : 
	_sceneName(sceneName) {
}

AfterglowEntity& AfterglowScene::createEntity(const std::string& name, util::OptionalRef<AfterglowEntity> parent) {
	EntityTree::NodeID parentID = EntityTree::invalidID();
	if (parent) {
		parentID = parent->get().id();
	}

	auto id = _entities.append(name, std::make_unique<AfterglowEntity>(), parentID);
	auto& entity = *_entities.get<AfterglowEntity>(id);

	// Tag synchronization.
	entity.initID(id);
	entity.rename(name);
	entity.setParent(*_entities.get<AfterglowEntity>(parentID));

	return entity;
}

bool AfterglowScene::destroyEntity(AfterglowEntity& entity) {
	return _entities.remove(entity.id());
}

std::vector<AfterglowEntity*> AfterglowScene::findEntities(const std::string name) {
	auto ids = _entities.find(name);
	std::vector<AfterglowEntity*> entities;
	entities.reserve(ids.size());
	for (auto id : ids) {
		auto* entity = _entities.get<AfterglowEntity>(id);
		if (entity) {
			entities.push_back(entity);
		}
	}
	return entities;
}

bool AfterglowScene::isExists(AfterglowEntity& entity) const {
	return _entities.isExists(entity.id());
}

bool AfterglowScene::hasChild(AfterglowEntity& entity) const {
	return _entities.numChildren(entity.id());
}

const std::string& AfterglowScene::sceneName() const noexcept {
	return _sceneName;
}

void AfterglowScene::setSceneName(const std::string& sceneName) noexcept {
	_sceneName = sceneName;
}
