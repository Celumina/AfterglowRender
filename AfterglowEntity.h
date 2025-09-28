#pragma once
#include <unordered_map>
#include <string>

#include "AfterglowComponentBase.h"

// Forwarding declaration.
class AfterglowScene;

class AfterglowEntity : public AfterglowObject {
public:
	using ID = uint64_t;
	using Components = std::unordered_map<std::type_index, AfterglowComponentBase*>;

	friend class AfterglowScene;
	friend class AfterglowComponentPool;

	AfterglowEntity();

	ID id() const;
	const std::string& name() const;

	AfterglowEntity* parent();
	const AfterglowEntity* parent() const;

	// @return: component pointer, if component not exist. return nullptr;
	template<typename ComponentType>
	ComponentType* component();

	template<typename ComponentType>
	const ComponentType* component() const;

	// @return: component pointer, if component not exist. throw an error;
	template<typename ComponentType>
	ComponentType& get();

	template<typename ComponentType>
	const ComponentType& get() const;

	AfterglowComponentBase* component(std::type_index typeIndex);

private:
	// @brief: This function could replace old one if it is exist.
	template<typename ComponentType>
	void bindComponent(ComponentType& component);

	// @return:Unbind successfully.
	template<typename ComponentType>
	bool unbindComponent();

	// Related to tree struct, so scene access only.
	// @return true if initialization successful.
	bool initID(ID id);
	// bool initSceneContext(const AfterglowSceneContext& sceneContext);
	void rename(const std::string& name);

	void setParent(AfterglowEntity& parent);

	ID _id;
	AfterglowEntity* _parent;
	std::string _name;
	Components _components;
	// AfterglowTransform _transform;
	// LifecycleModule _lifecycleModule;
	// const AfterglowSceneContext* _sceneContext;
};

template<typename ComponentType>
inline void AfterglowEntity::bindComponent(ComponentType& component) {
	auto typeIndex = std::type_index(typeid(ComponentType));
	_components[typeIndex] = &component;
}

template<typename ComponentType>
inline bool AfterglowEntity::unbindComponent() {
	auto typeIndex = std::type_index(typeid(ComponentType));
	auto iterator = _components.find(typeIndex);
	if (iterator != _components.end()) {
		_components.erase(iterator);
		return true;
	}
	return false;
}

template<typename ComponentType>
inline ComponentType* AfterglowEntity::component() {
	auto typeIndex = std::type_index(typeid(ComponentType));
	auto iterator = _components.find(typeIndex);
	if (iterator != _components.end()) {
		return reinterpret_cast<ComponentType*>(iterator->second);
	}
	return nullptr;
}

template<typename ComponentType>
inline const ComponentType* AfterglowEntity::component() const {
	return const_cast<AfterglowEntity*>(this)->component<ComponentType>();
}


template<typename ComponentType>
inline ComponentType& AfterglowEntity::get() {
	return *component<ComponentType>();
}

template<typename ComponentType>
inline const ComponentType& AfterglowEntity::get() const {
	return *component<ComponentType>();
}