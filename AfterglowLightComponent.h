#pragma once
#include "AfterglowComponent.h"
#include "AfterglowColor.h"

template<typename ComponentType>
class AfterglowLightComponent : public AfterglowComponent<ComponentType> {
public:

	float intensity() const noexcept;
	AfterglowColor color() const noexcept;

	void setIntensity(float intensity) noexcept;
	void setColor(AfterglowColor color) noexcept;

private:
	AfterglowColor _color = AfterglowColor::White();
	float _intensity = 1.0;
};

template<typename ComponentType>
float AfterglowLightComponent<ComponentType>::intensity() const noexcept {
	return _intensity;
}

template<typename ComponentType>
AfterglowColor AfterglowLightComponent<ComponentType>::color() const noexcept {
	return _color;
}

template<typename ComponentType>
void AfterglowLightComponent<ComponentType>::setIntensity(float intensity) noexcept {
	_intensity = intensity;
}

template<typename ComponentType>
inline void AfterglowLightComponent<ComponentType>::setColor(AfterglowColor color) noexcept {
	_color = color;
}
