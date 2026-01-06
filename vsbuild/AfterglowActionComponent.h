#pragma once
#include "AfterglowComponent.h"
#include "AfterglowSystemUtilities.h"

template<typename DerivedType>
class AfterglowActionComponent : public AfterglowComponent<DerivedType> {
public:
	using Component = DerivedType;

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

template<typename DerivedType>
inline AfterglowActionComponent<DerivedType>::~AfterglowActionComponent() {
	onDestroy();
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::enable() {
	onEnable();
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::disable() {
	onDisable();
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::awake() {
	if constexpr (!std::is_same_v<decltype(&Component::awake), decltype(&AfterglowActionComponent::awake)>) {
		reinterpret_cast<Component>(*this).awake();
	}
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::onEnable() {
	if constexpr (!std::is_same_v<decltype(&Component::onEnable), decltype(&AfterglowActionComponent::onEnable)>) {
		reinterpret_cast<Component>(*this).onEnable();
	}
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::update() {
	if constexpr (!std::is_same_v<decltype(&Component::update), decltype(&AfterglowActionComponent::update)>) {
		reinterpret_cast<Component>(*this).update();
	}
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::fixedUpdate() {
	if constexpr (!std::is_same_v<decltype(&Component::fixedUpdate), decltype(&AfterglowActionComponent::fixedUpdate)>) {
		Component::fixedUpdate();
		reinterpret_cast<Component>(*this).fixedUpdate();
	}
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::onDisable() {
	if constexpr (!std::is_same_v<decltype(&Component::onDisable), decltype(&AfterglowActionComponent::onDisable)>) {
		reinterpret_cast<Component>(*this).onDisable();
	}
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::onDestroy() {
	if constexpr (!std::is_same_v<decltype(&Component::onDestroy), decltype(&AfterglowActionComponent::onDestroy)>) {
		reinterpret_cast<Component>(*this).onDestroy();
	}
}

template<typename DerivedType>
inline const AfterglowSystemUtilities& AfterglowActionComponent<DerivedType>::sysUtils() {
	return *_sysUtils;
}

template<typename DerivedType>
inline void AfterglowActionComponent<DerivedType>::bindSystemUtilities(const AfterglowSystemUtilities& sysUtils) {
	_sysUtils = &sysUtils;
}


INR_CRTP_CLASS(AfterglowActionComponent, DerivedType) {
	INR_BASE_CLASSES<AfterglowComponent<InreflectDerivedType>>;
	INR_FUNCS(
		INR_FUNC(enable),
		INR_FUNC(disable)
		// Don't reflect these action functions.
	);
};

