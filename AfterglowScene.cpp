#include "AfterglowScene.h"

AfterglowScene::AfterglowScene() {
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

	// entity.initSceneContext(_sceneContext);
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

bool AfterglowScene::isExists(AfterglowEntity& entity) {
	return _entities.isExists(entity.id());
}
