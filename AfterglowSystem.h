#pragma once

#include <algorithm>
#include <thread>
#include <memory>

#include "AfterglowRenderableContext.h"
#include "AfterglowScene.h"
#include "AfterglowSystemUtilities.h"


class AfterglowMaterialManager;
class AfterglowWindow;
class AfterglowTicker;
class AfterglowGUI;

// TODO: SystemThread
class AfterglowSystem {
public: 
	AfterglowSystem(
		AfterglowWindow& window, 
		AfterglowMaterialManager& materialManager, 
		AfterglowGUI& ui
	);
	~AfterglowSystem();

	std::weak_ptr<AfterglowScene> scene() noexcept;
	AfterglowComponentPool& componentPool() noexcept;

	AfterglowCameraComponent* mainCamera() noexcept;

	void startSystemThread();
	void stopSystemThread();

	// TODO: Create in a parent entity.
	// TODO: Transform additivity.
	template<typename ...ComponentTypes>
	AfterglowEntity&  createEntity(const std::string& name, util::OptionalRef<AfterglowEntity> parent = std::nullopt);

	// @brief: Also destroy all components of this entity.
	// @bool: Destroy successfully.
	bool destroyEntity(AfterglowEntity& entity);

	// @return: Add component successfully.
	// @thread_safety
	template<typename ComponentType>
	bool addComponent(AfterglowEntity& destEntity);

	template<typename FirstComponentType, typename ...OtherComponentTypes>
	void addComponents(AfterglowEntity& destEntity);

	// @thread_safety
	template<typename ComponentType>
	void removeComponent(AfterglowEntity& destEntity);

	template<typename TupleType, size_t Index = 0>
	void removeComponents(AfterglowEntity& destEntity);

	// @warning: For SystemUtilities, add component from typeIndex will never call specializedAddBehaviour automatically.
	AfterglowComponentBase* addComponent(AfterglowEntity& destEntity, std::type_index typeIndex);

	void setMainCamera(AfterglowCameraComponent& camera) noexcept;

	AfterglowRenderableContext& renderableContext() noexcept;

	AfterglowWindow& window() noexcept;
	const AfterglowInput& input() const noexcept;


	AfterglowTicker& ticker() noexcept;
	const AfterglowTicker& ticker() const noexcept;

	AfterglowMaterialManager& materialManager() noexcept;


private:
	template<typename ComponentType>
	void updateTypeComponents();

	template<typename TupleType, size_t Index = 0>
	void updateComponents();

	void systemLoop();

	template<typename ComponentType>
	void specializedAddBehaviour(ComponentType& component);

	template<reg::ActionComponentType ComponentType>
	void specializedAddBehaviour(ComponentType& component);

	template<>
	void specializedAddBehaviour(AfterglowCameraComponent& component);

	template<>
	void specializedAddBehaviour<>(AfterglowDirectionalLightComponent& component);

	template<>
	void specializedAddBehaviour<>(AfterglowPostProcessComponent& component);

	template<typename ComponentType>
	void specializedRemoveBehaviour(ComponentType* component);

	template<>
	void specializedRemoveBehaviour(AfterglowCameraComponent* component);

	template<>
	void specializedRemoveBehaviour<>(AfterglowDirectionalLightComponent* component);

	template<>
	void specializedRemoveBehaviour<>(AfterglowPostProcessComponent* component);

	template<typename ComponentType>
	void refreshRenderableContext(ComponentType* component, const std::string& varName);

	// Share scene with weak ptr, to handle the scene change.
	std::shared_ptr<AfterglowScene> _scene;
	AfterglowComponentPool _componentPool;
	AfterglowRenderableContext _renderableContext;
	AfterglowSystemUtilities _utilities;

	struct Impl;
	std::unique_ptr<Impl> _impl;
};

template<typename ...ComponentTypes>
inline AfterglowEntity& AfterglowSystem::createEntity(const std::string& name, util::OptionalRef<AfterglowEntity> parent) {
	auto& entity = _scene->createEntity(name, parent);
	// Every scene entity need a tranform info, for able to put it in the world.
	addComponent<AfterglowTransformComponent>(entity);
	if constexpr (sizeof ...(ComponentTypes) != 0) {
		addComponents<ComponentTypes...>(entity);
	}
	return entity;
}

template<typename ComponentType>
inline bool AfterglowSystem::addComponent(AfterglowEntity& destEntity) {
	static_assert(
		reg::IsComponentRegistered<ComponentType>(), 
		"This Component is not registered, register it in AfterglowComponentRegistery."
		);
	if (destEntity.component<ComponentType>()) {
		return false;
	}

	auto& component = _componentPool.create<ComponentType>(destEntity);
	specializedAddBehaviour(component);
	return true;
}

template<typename FirstComponentType, typename ...OtherComponentTypes>
inline void AfterglowSystem::addComponents(AfterglowEntity& destEntity) {
	addComponent<FirstComponentType>(destEntity);
	if constexpr (sizeof ...(OtherComponentTypes) != 0) {
		addComponents<OtherComponentTypes...>(destEntity);
	}
}

template<typename ComponentType>
inline void AfterglowSystem::removeComponent(AfterglowEntity& destEntity) {
	static_assert(
		reg::IsComponentRegistered<ComponentType>(),
		"This Component is not registered, register it in AfterglowComponentRegistery."
		);

	auto* destComponent = destEntity.component<ComponentType>();
	if (!destComponent) {
		return;
	}

	// Entities' ref also be updated inside here.
	_componentPool.destroy(*destComponent);

	// Update Rendeable Reference, Solving dangling pointer problems, Note that here destComponent was dangling, don't deref it.
	specializedRemoveBehaviour(destComponent);
}


template<typename TupleType, size_t Index>
inline void AfterglowSystem::removeComponents(AfterglowEntity& destEntity) {
	removeComponent<std::tuple_element_t<Index, TupleType>>(destEntity);
	if constexpr (Index + 1 < std::tuple_size_v<TupleType>) {
		removeComponents<TupleType, Index + 1>(destEntity);
	}
}

template<typename ComponentType>
inline void AfterglowSystem::updateTypeComponents() {
	// Update ActionCompoents
	if constexpr (std::is_base_of_v<AfterglowActionComponent<ComponentType>, ComponentType>) {
		auto& components = _componentPool.components<ComponentType>();
		for (auto& component : components) {
			if (component.enabled()) {
				component.update();
				// TODO: Fixed update
			}
		}
	}
}

template<typename TupleType, size_t Index>
inline void AfterglowSystem::updateComponents() {
	updateTypeComponents<std::tuple_element_t<Index, TupleType>>();
	if constexpr (Index + 1 < std::tuple_size_v<TupleType>) {
		updateComponents<TupleType, Index + 1>();
	}
}

template<typename ComponentType>
inline void AfterglowSystem::specializedAddBehaviour(ComponentType& component) {
	// DEBUG_TYPE_WARNING(ComponentType, "This component type have not specialized add behaviour.");
}

template<reg::ActionComponentType ComponentType>
inline void AfterglowSystem::specializedAddBehaviour(ComponentType& component) {
	component.bindSystemUtilities(_utilities);
	component.awake();
}

template<>
inline void AfterglowSystem::specializedAddBehaviour(AfterglowCameraComponent& component) {
	// Just replace it, due to update from component pool contain could make the _renderableContext member dangling.
	_renderableContext.camera = &component;
}

template<>
inline void AfterglowSystem::specializedAddBehaviour(AfterglowDirectionalLightComponent& component) {
	// TODO: Only one directional light in active now.
	_renderableContext.diectionalLight = &component;
}

template<>
inline void AfterglowSystem::specializedAddBehaviour(AfterglowPostProcessComponent& component) {
	_renderableContext.postProcess = &component;
}

template<typename ComponentType>
inline void AfterglowSystem::specializedRemoveBehaviour(ComponentType* component) {
}

template<>
inline void AfterglowSystem::specializedRemoveBehaviour(AfterglowCameraComponent* component) {
	refreshRenderableContext(component, "camera");
}

template<>
inline void AfterglowSystem::specializedRemoveBehaviour(AfterglowDirectionalLightComponent* component) {
	refreshRenderableContext(component, "directionalLight");
}

template<>
inline void AfterglowSystem::specializedRemoveBehaviour(AfterglowPostProcessComponent* component) {
	refreshRenderableContext(component, "postProcess");
}

template<typename ComponentType>
inline void AfterglowSystem::refreshRenderableContext(ComponentType* component, const std::string& varName) {
	// ComponentType**
	auto var = Inreflect<AfterglowRenderableContext>::attribute<ComponentType*>(_renderableContext, varName);
	if (!var) {
		DEBUG_CLASS_ERROR("Invalid renderable context variable name: " +  varName);
		throw std::runtime_error("Invalid renderable context variable name.");
	}
	if (*var == component) {
		auto& components = _componentPool.components<ComponentType>();
		if (!components.empty()) {
			// If make sure do it after pool->destroy, here can be replaced as *var = components[0];
			for (auto& poolComponent : components) {
				if (&poolComponent != component) {
					*var = &poolComponent;
					return;
				}
			}
		}
		// If not found any other component:
		*var = nullptr;
	}
}
