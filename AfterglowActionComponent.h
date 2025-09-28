#pragma once
#include "AfterglowComponent.h"
#include "AfterglowSystemUtilities.h"

template<typename DerivedClass>
class AfterglowActionComponent : public AfterglowComponent<DerivedClass> {
public:
	using Component = DerivedClass;

	friend class AfterglowSystem;

	~AfterglowActionComponent();

	void enable();
	void disable();

	// CRTP Partten
	void awake();
	void onEnable();
	void update();
	void fixedUpdate();
	void onDisable();
	void onDestroy();

protected:
	const AfterglowSystemUtilities& sysUtils();

private:
	void bindSystemUtilities(const AfterglowSystemUtilities& sysUtils);
	const AfterglowSystemUtilities* _sysUtils;
};

template<typename DerivedClass>
inline AfterglowActionComponent<DerivedClass>::~AfterglowActionComponent() {
	onDestroy();
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::enable() {
	onEnable();
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::disable() {
	onDisable();
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::awake() {
	if constexpr (!std::is_same_v<decltype(&Component::awake), decltype(&AfterglowActionComponent::awake)>) {
		reinterpret_cast<Component>(*this).awake();
	}
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::onEnable() {
	if constexpr (!std::is_same_v<decltype(&Component::onEnable), decltype(&AfterglowActionComponent::onEnable)>) {
		reinterpret_cast<Component>(*this).onEnable();
	}
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::update() {
	if constexpr (!std::is_same_v<decltype(&Component::update), decltype(&AfterglowActionComponent::update)>) {
		reinterpret_cast<Component>(*this).update();
	}
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::fixedUpdate() {
	if constexpr (!std::is_same_v<decltype(&Component::fixedUpdate), decltype(&AfterglowActionComponent::fixedUpdate)>) {
		Component::fixedUpdate();
		reinterpret_cast<Component>(*this).fixedUpdate();
	}
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::onDisable() {
	if constexpr (!std::is_same_v<decltype(&Component::onDisable), decltype(&AfterglowActionComponent::onDisable)>) {
		reinterpret_cast<Component>(*this).onDisable();
	}
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::onDestroy() {
	if constexpr (!std::is_same_v<decltype(&Component::onDestroy), decltype(&AfterglowActionComponent::onDestroy)>) {
		reinterpret_cast<Component>(*this).onDestroy();
	}
}

template<typename DerivedClass>
inline const AfterglowSystemUtilities& AfterglowActionComponent<DerivedClass>::sysUtils() {
	return *_sysUtils;
}

template<typename DerivedClass>
inline void AfterglowActionComponent<DerivedClass>::bindSystemUtilities(const AfterglowSystemUtilities& sysUtils) {
	_sysUtils = &sysUtils;
}
