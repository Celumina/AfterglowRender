#pragma once
#include <mutex>

#include "AfterglowContext.h"
#include "AfterglowComponentRegistery.h"

class AfterglowComponentPool : public AfterglowContext {
public:
	using LockGuard = std::lock_guard<std::mutex>;
	using UniqueLock = std::unique_lock<std::mutex>;

	template<typename ComponentType>
	class ComponentArray : public AfterglowObject {
	public:
		using Component = ComponentType;
		using Container = std::vector<ComponentType>;

		std::vector<ComponentType>* operator->();
		std::vector<ComponentType>& operator*();

	private:
		Container _data;
	};

	AfterglowComponentPool();

	// @thread_safety
	template<typename ComponentType>
	ComponentType& create(AfterglowEntity& destEntity);

	/**
	@thread_safety
	@param callback: [](auto& component){...}
	*/ 
	template<typename CallbackType = void*, uint32_t index = 0>
	AfterglowComponentBase* create(AfterglowEntity& destEntity, std::type_index typeIndex, CallbackType callback = nullptr);

	// @thread_safety
	template<typename ComponentType>
	bool destroy(ComponentType& component);

	template<typename ComponentType>
	ComponentArray<ComponentType>::Container& components();

	// @thread_safety
	template<typename ComponentType>
	ComponentType* component(AfterglowComponentBase::ID id);

	UniqueLock lock() const;

private:
	mutable std::mutex _mutex;

	template<typename TupleType, size_t Index = 0>
	void initializeComponentTypes();

	template<typename ComponentType>
	void updateEntityReferences(ComponentArray<ComponentType>::Container& container);
};


template<typename ComponentType>
inline ComponentType& AfterglowComponentPool::create(AfterglowEntity& destEntity) {
	LockGuard lockGuard{_mutex};
	auto& container = components<ComponentType>();
	auto* oldData = container.data();

	container.emplace_back(ComponentType{});
	auto& component = container.back();
	component.setEntity(destEntity);
	destEntity.bindComponent(component);

	// If data changed, update entities' binding.
	// Oops, if entity tree changed.... Don't worry about that, that is my container.
	if (oldData != container.data()) {
		updateEntityReferences<ComponentType>(container);
	}

	return component;
}

template<typename CallbackType, uint32_t index>
inline AfterglowComponentBase* AfterglowComponentPool::create(AfterglowEntity& destEntity, std::type_index typeIndex, CallbackType callback) {
	if constexpr (index < std::tuple_size_v<reg::RegisteredComponentTypes>) {
		using CurrentComponentType = std::tuple_element_t<index, reg::RegisteredComponentTypes>;
		if (std::type_index(typeid(CurrentComponentType)) == typeIndex) {
			auto& component = create<CurrentComponentType>(destEntity);
			if constexpr (!std::is_same_v<CallbackType, void*>) {
				callback(component);
			}
			return &component;
		}
		else {
			return create<CallbackType, index + 1>(destEntity, typeIndex, callback);
		}
	}
	else {
		return nullptr;
	}
}

template<typename ComponentType>
inline bool AfterglowComponentPool::destroy(ComponentType& component) {
	LockGuard lockGuard{ _mutex };
	auto& container = components<ComponentType>();
	auto iterator = std::find(container.begin(), container.end(), component);
	if (iterator != container.end()) {
		(*iterator).entity().unbindComponent<ComponentType>();
		container.erase(iterator);
		// TODO: Index begin optimization.
		updateEntityReferences<ComponentType>(container);
		return true;
	}
	return false;
}

template<typename ComponentType>
inline AfterglowComponentPool::ComponentArray<ComponentType>::Container& AfterglowComponentPool::components() {
	return *get<ComponentArray<ComponentType>>();
}

template<typename ComponentType>
inline ComponentType* AfterglowComponentPool::component(AfterglowComponentBase::ID id) {
	auto& components = *get<ComponentArray<ComponentType>>();
	// Binary search because componet ids are increasing order.
	auto iterator = std::lower_bound(
		components.begin(), components.end(), id, 
		[](const ComponentType& component, AfterglowComponentBase::ID id){
			return component.id() < id;
		}
	);
	// In-Order Search.
	//for (const auto& component : components) {
	//	if (component.id() == id) {
	//		return &component;
	//	}
	//}
	if (iterator != components.end()) {
		return &(*iterator);
	}
	return nullptr;
}

template<typename TupleType, size_t Index>
inline void AfterglowComponentPool::initializeComponentTypes() {
	// Initialize ElementType by Index.
	initialize<ComponentArray<std::tuple_element_t<Index, TupleType>>>();
	if constexpr (Index + 1 < std::tuple_size_v<TupleType>) {
		initializeComponentTypes<TupleType, Index + 1>();
	}
}

template<typename ComponentType>
inline void AfterglowComponentPool::updateEntityReferences(ComponentArray<ComponentType>::Container& container) {
	for (auto& component : container) {
		component.entity().bindComponent(component);
	}
}

template<typename ComponentType>
inline std::vector<ComponentType>* AfterglowComponentPool::ComponentArray<ComponentType>::operator->() {
	return &_data;
}

template<typename ComponentType>
inline std::vector<ComponentType>& AfterglowComponentPool::ComponentArray<ComponentType>::operator*() {
	return _data;
}
