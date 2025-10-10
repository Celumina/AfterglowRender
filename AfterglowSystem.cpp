#include "AfterglowSystem.h"
#include "LocalClock.h"

AfterglowSystem::AfterglowSystem(AfterglowWindow& window, AfterglowMaterialManager& materialManager) :
	_systemStoped(false), 
	_renderableContext({ _componentPool, nullptr}),
	_utilities(this), 
	_window(window), 
	_materialManager(materialManager) {
}

AfterglowSystem::~AfterglowSystem() {
	_systemStoped = false;
}

AfterglowScene& AfterglowSystem::scene() {
	return _scene;
}

AfterglowComponentPool& AfterglowSystem::componentPool() {
	return _componentPool;
}

AfterglowCameraComponent* AfterglowSystem::mainCamera() {
	return _renderableContext.camera;
}

void AfterglowSystem::startSystemThread() {
	if (!_systemThread) {
		_systemThread = std::make_unique<std::jthread>([&]() {systemLoop();  });
	}
}

void AfterglowSystem::stopSystemThread() {
	_systemStoped = true;
	_systemThread.reset();
}

bool AfterglowSystem::destroyEntity(AfterglowEntity& entity) {
	if (!_scene.isExists(entity)) {
		return false;
	}
	removeComponents<reg::RegisteredComponentTypes>(entity);
	_scene.destroyEntity(entity);
	return true;
}

AfterglowComponentBase* AfterglowSystem::addComponent(AfterglowEntity& destEntity, std::type_index typeIndex) {
	return _componentPool.create(destEntity, typeIndex, [this](auto& component){ specializedAddBehaviour(component); });
}

void AfterglowSystem::setMainCamera(AfterglowCameraComponent& camera) {
	_renderableContext.camera = &camera;
}

AfterglowRenderableContext& AfterglowSystem::renderableContext() {
	return _renderableContext;
}

void AfterglowSystem::systemLoop() {
	// TODO: Stop system automatically?
	while (!_systemStoped) {
		_clock.update();
		_window.input().update();
		updateComponents<reg::RegisteredComponentTypes>();
	}
}

AfterglowWindow& AfterglowSystem::window() {
	return _window;
}

const AfterglowInput& AfterglowSystem::input() const {
	return _window.input();
}

const LocalClock& AfterglowSystem::clock() const {
	return _clock;
}

AfterglowMaterialManager& AfterglowSystem::materialManager() {
	return _materialManager;
}
