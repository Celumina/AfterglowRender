#pragma once

#include "Carousel.h"

template<size_t Size>
class AfterglowFPSCarousel {
public: 
	void update(float fps);

	const float* data() const noexcept;
	size_t size() const noexcept;

	float smoothFPS() const noexcept;

	void setSmoothFactor(float smoothFactor) noexcept;
	void setFrameInterval(uint32_t frameInterval) noexcept;

private:
	Carousel<float, Size> _carousel;
	// Interval status
	float _smoothFPS = 0.0f;
	uint32_t _frameIndex = 0;

	// Options
	float _smoothFactor = 0.0f;
	uint32_t _frameInterval = 0;
};

template<size_t Size>
inline void AfterglowFPSCarousel<Size>::update(float fps) {
	if (_frameIndex == 0) {
		_carousel.push(_smoothFPS);
	}
	_smoothFPS = _smoothFactor * fps + (1.0f - _smoothFactor) * _smoothFPS;
	_frameIndex = (_frameIndex + 1) % _frameInterval;
}

template<size_t Size>
inline const float* AfterglowFPSCarousel<Size>::data() const noexcept {
	return _carousel.data();
}

template<size_t Size>
inline size_t AfterglowFPSCarousel<Size>::size() const noexcept {
	return _carousel.size();
}

template<size_t Size>
inline float AfterglowFPSCarousel<Size>::smoothFPS() const noexcept {
	return _smoothFPS;
}

template<size_t Size>
inline void AfterglowFPSCarousel<Size>::setSmoothFactor(float smoothFactor) noexcept {
	_smoothFactor = smoothFactor;
}

template<size_t Size>
inline void AfterglowFPSCarousel<Size>::setFrameInterval(uint32_t frameInterval) noexcept {
	_frameInterval = frameInterval;
}
