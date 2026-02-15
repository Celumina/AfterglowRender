#pragma once

#include <algorithm>
#include <thread>
#include <memory>

#include "AfterglowRenderableContext.h"
#include "AfterglowScene.h"
#include "AfterglowSystemUtilities.h"
#include "AfterglowCullingUtilities.h"
#include "ExceptionUtilities.h"


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

	// TODO: Transform additivity.
	template<typename ...ComponentTypes>
	AfterglowEntity&  createEntity(const std::string& name, util::OptionalRef<AfterglowEntity> parent = std::nullopt);

	// @brief: Also destroy all components of this entity.
	// @return: Destroy successfully.
	bool destroyEntity(AfterglowEntity& entity);

	/**
	* @thread_safety
	* @brief: Add component to the dest entity if it's not exists before.
	* @return: Component reference.
	* @warning: 
	*	The component reference could be dangling when the component container reallocation.
	*	For persistence, store its id from component->id().
	*/
	template<typename ComponentType>
	ComponentType& addComponent(AfterglowEntity& destEntity);

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
	template<typename Type>
	void updateTypeComponents() {}

	template<reg::ActionComponentType Type>
	void updateTypeComponents();

	template<>
	void updateTypeComponents<AfterglowTransformComponent> ();

	template<>
	void updateTypeComponents<AfterglowCameraComponent>();

	template<>
	void updateTypeComponents<AfterglowStaticMeshComponent>();

	template<typename TupleType, size_t Index = 0>
	void updateComponents();

	void systemLoop(std::stop_token stopToken);

	template<typename ComponentType>
	void specializedAddBehaviour(ComponentType& component);

	template<reg::ActionComponentType ComponentType>
	void specializedAddBehaviour(ComponentType& component);

	template<>
	void specializedAddBehaviour(AfterglowCameraComponent& component);

	template<>
	void specializedAddBehaviour<>(AfterglowDirectionalLightComponent& component);

	//template<>
	//void specializedAddBehaviour<>(AfterglowPostProcessComponent& component);

	template<typename ComponentType>
	void specializedRemoveBehaviour(ComponentType* component);

	template<>
	void specializedRemoveBehaviour(AfterglowCameraComponent* component);

	template<>
	void specializedRemoveBehaviour<>(AfterglowDirectionalLightComponent* component);

	//template<>
	//void specializedRemoveBehaviour<>(AfterglowPostProcessComponent* component);

	template<typename ComponentType>
	void refreshRenderableContext(ComponentType* component, const std::string& varName);

	//inline void applyDestroyEntityCache();
	// @brief: global uniform form material manager.
	ubo::GlobalUniform& globalUniform() const;

	struct Impl;
	std::unique_ptr<Impl> _impl;

	// Share scene with weak ptr, to handle the scene change.
	std::shared_ptr<AfterglowScene> _scene;
	AfterglowComponentPool _componentPool;
	AfterglowRenderableContext _renderableContext;
	AfterglowSystemUtilities _utilities;
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
inline ComponentType& AfterglowSystem::addComponent(AfterglowEntity& destEntity) {
	static_assert(
		reg::IsComponentRegistered<ComponentType>(), 
		"This Component is not registered, register it in AfterglowComponentRegistery."
		);
	auto* oldComponent = destEntity.component<ComponentType>();
	if (oldComponent) {
		return *oldComponent;
	}

	auto& component = _componentPool.create<ComponentType>(destEntity);
	specializedAddBehaviour(component);
	return component;
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

template<reg::ActionComponentType Type>
inline void AfterglowSystem::updateTypeComponents() {
	// Update ActionCompoents
	auto& components = _componentPool.components<Type>();
	// Update only if component type update() was overrided.
	if constexpr (!std::is_same_v<decltype(&Type::update), decltype(&AfterglowActionComponent<Type>::update)>) {
		for (auto& component : components) {
			if (component.enabled()) {
				component.update();
			}
		}
	}
	// TODO: Fixed update
}

template<>
inline void AfterglowSystem::updateTypeComponents<AfterglowTransformComponent>() {
	auto& components = _componentPool.components<AfterglowTransformComponent>();
	for (auto& component : components) {
		// Always enabled
		// Preparing transform matrix for rendering .
		component.updateGlobalMatrices();
	}
}

template<>
inline void AfterglowSystem::updateTypeComponents<AfterglowCameraComponent>() {
	auto& components = _componentPool.components<AfterglowCameraComponent>();
	for (auto& component : components) {
		if (component.enabled()) {
			component.updateMatrices();
		}
	}
}

template<>
inline void AfterglowSystem::updateTypeComponents<AfterglowStaticMeshComponent>() {
	auto& components = _componentPool.components<AfterglowStaticMeshComponent>();
	for (auto& component : components) {
		if (!component.enabled() || !component.meshResource()) {
			continue;
		}
		if (!component.property(renderable::Property::DynamicCulling)) {
			continue;
		}
		//DEBUG_COST_BEGIN("Update static mesh visibility");
		// Update static mesh visibility
		auto* aabb = component.meshResource()->aabb();
		if (!aabb) {
			continue;
		}
		auto& transformComponent = component.entity().get<AfterglowTransformComponent>();
		model::AABB aabbWorld = util::NonshearTransformAABB(*aabb, transformComponent.globalTransformMatrix());
		component.setProperty(
			renderable::Property::VisibleCache, 
			!util::FrustumCulling(globalUniform(), aabbWorld)
		);
		//DEBUG_COST_END;
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

//template<>
//inline void AfterglowSystem::specializedAddBehaviour(AfterglowPostProcessComponent& component) {
//	_renderableContext.postProcess = &component;
//}

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

//template<>
//inline void AfterglowSystem::specializedRemoveBehaviour(AfterglowPostProcessComponent* component) {
//	refreshRenderableContext(component, "postProcess");
//}

template<typename ComponentType>
inline void AfterglowSystem::refreshRenderableContext(ComponentType* component, const std::string& varName) {
	// ComponentType**
	auto var = Inreflect<AfterglowRenderableContext>::attribute<ComponentType*>(_renderableContext, varName);
	if (!var) {
		EXCEPT_CLASS_RUNTIME("Invalid renderable context variable name: " +  varName);
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
