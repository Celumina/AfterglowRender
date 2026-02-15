#include "AfterglowSystem.h"
#include "AfterglowInput.h"
#include "AfterglowTicker.h"
#include "AfterglowMaterialManager.h"
#include "AfterglowWindow.h"
#include "AfterglowGUI.h"

struct AfterglowSystem::Impl {
	Impl(AfterglowWindow& windowRef, AfterglowMaterialManager& materialManagerRef);

	AfterglowWindow& window;
	AfterglowMaterialManager& materialManager;
	AfterglowTicker ticker;

	// To prevent destoryEntity when components updating.
	//std::vector<AfterglowEntity*> destroyEntityCache;

	std::unique_ptr<std::jthread> systemThread;
};

AfterglowSystem::Impl::Impl(AfterglowWindow& windowRef, AfterglowMaterialManager& materialManagerRef) : 
	window(windowRef), materialManager(materialManagerRef) {
}

AfterglowSystem::AfterglowSystem(
	AfterglowWindow& window, 
	AfterglowMaterialManager& materialManager, 
	AfterglowGUI& ui) :
	_scene(std::make_shared<AfterglowScene>()), 
	_renderableContext({ _componentPool, nullptr }),
	_utilities(this), 
	_impl(std::make_unique<Impl>(window, materialManager)) {
	ui.bindSystemUtilities(_utilities);
}

AfterglowSystem::~AfterglowSystem() {
}

std::weak_ptr<AfterglowScene> AfterglowSystem::scene() noexcept {
	return _scene;
}

AfterglowComponentPool& AfterglowSystem::componentPool() noexcept {
	return _componentPool;
}

AfterglowCameraComponent* AfterglowSystem::mainCamera() noexcept {
	return _renderableContext.camera;
}

void AfterglowSystem::startSystemThread() {
	if (!_impl->systemThread) {
		_impl->systemThread = std::make_unique<std::jthread>([this](std::stop_token stopToken) { systemLoop(stopToken); });
	}
}

void AfterglowSystem::stopSystemThread() {
	_impl->systemThread.reset();
}

bool AfterglowSystem::destroyEntity(AfterglowEntity& entity) {
	if (!_scene->isExists(entity)) {
		return false;
	}

	removeComponents<reg::RegisteredComponentTypes>(entity);
	_scene->destroyEntity(entity);
	// _impl->destroyEntityCache.push_back(&entity);
	return true;
}

AfterglowComponentBase* AfterglowSystem::addComponent(AfterglowEntity& destEntity, std::type_index typeIndex) {
	return _componentPool.create(destEntity, typeIndex, [this](auto& component){ specializedAddBehaviour(component); });
}

void AfterglowSystem::setMainCamera(AfterglowCameraComponent& camera) noexcept {
	_renderableContext.camera = &camera;
}

AfterglowRenderableContext& AfterglowSystem::renderableContext() noexcept {
	return _renderableContext;
}

void AfterglowSystem::systemLoop(std::stop_token stopToken) {
	while (!stopToken.stop_requested()) {
		_impl->ticker.tick();
		_impl->window.input().update();
		// applyDestroyEntityCache();
		updateComponents<reg::RegisteredComponentTypes>();
	}
}

ubo::GlobalUniform& AfterglowSystem::globalUniform() const {
	return _impl->materialManager.globalUniform();
}

//inline void AfterglowSystem::applyDestroyEntityCache() {
//	// Destroy marked entities.
//	for (auto* entity : _impl->destroyEntityCache) {
//		removeComponents<reg::RegisteredComponentTypes>(*entity);
//		_scene->destroyEntity(*entity);
//	}
//	_impl->destroyEntityCache.clear();
//}

AfterglowWindow& AfterglowSystem::window() noexcept {
	return _impl->window;
}

const AfterglowInput& AfterglowSystem::input() const  noexcept {
	return _impl->window.input();
}

AfterglowTicker& AfterglowSystem::ticker() noexcept {
	return _impl->ticker;
}

const AfterglowTicker& AfterglowSystem::ticker() const noexcept {
	return _impl->ticker;
}

AfterglowMaterialManager& AfterglowSystem::materialManager() noexcept {
	return _impl->materialManager;
}

