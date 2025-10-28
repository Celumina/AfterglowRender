#pragma once
#include "AfterglowComponentBase.h"
#include "AfterglowEntity.h"

template<typename DerivedType>
class AfterglowComponent : public AfterglowComponentBase {
public:
	friend class AfterglowComponentPool;
	using Component = DerivedType;
	
	void enable();
	void disable();

	bool enabled() const;

	inline AfterglowEntity& entity();
	inline const AfterglowEntity& entity() const;

private:
	void setEntity(AfterglowEntity& entity);

	void internalEnable();
	void internalDisable();

	bool _enabled = true;
	AfterglowEntity* _entity = nullptr;
};

template<typename DerivedType>
inline void AfterglowComponent<DerivedType>::enable() {
	internalEnable();
	if constexpr (!std::is_same_v<decltype(&Component::enable), decltype(&AfterglowComponent::enable)>) {
		reinterpret_cast<Component*>(this)->enable();
	}
}

template<typename DerivedType>
inline void AfterglowComponent<DerivedType>::disable() {
	internalDisable();
	if constexpr (!std::is_same_v<decltype(&Component::disable), decltype(&AfterglowComponent::disable)>) {
		reinterpret_cast<Component*>(this)->disable();
	}
}

template<typename DerivedType>
inline bool AfterglowComponent<DerivedType>::enabled() const {
	return _enabled;
}

template<typename DerivedType>
inline void AfterglowComponent<DerivedType>::setEntity(AfterglowEntity& entity) {
	_entity = &entity;
}

template<typename DerivedType>
inline AfterglowEntity& AfterglowComponent<DerivedType>::entity() {
	// System do that setEntity, so just ref it.
	return *_entity;
}

template<typename DerivedType>
inline const AfterglowEntity& AfterglowComponent<DerivedType>::entity() const {
	return *_entity;
}

template<typename DerivedType>
inline void AfterglowComponent<DerivedType>::internalEnable() {
	_enabled = true;
}

template<typename DerivedType>
inline void AfterglowComponent<DerivedType>::internalDisable() {
	_enabled = false;
}

INR_CRTP_CLASS(AfterglowComponent, DerivedType) {
 	INR_BASE_CLASSES<AfterglowComponentBase>;
	INR_FUNCS(
		// Its work! 
		// InreflectMemberFunction<decltype(&Inreflect::InreflectDefinition::Class::enable), TEMPLATE_STR("enable"), &Inreflect::InreflectDefinition::Class::enable> {}
		INR_FUNC(enable), 
		INR_FUNC(disable), 
		INR_FUNC(enabled), 
		// InreflectDerivedType is the keyword for overloaded function in INR_CRTP_CLASS.
		INR_OVERLOADED_FUNC(AfterglowEntity&(AfterglowComponent<InreflectDerivedType>::*)(), entity),
		INR_OVERLOADED_FUNC(const AfterglowEntity&(AfterglowComponent<InreflectDerivedType>::*)() const, entity)
	);
};