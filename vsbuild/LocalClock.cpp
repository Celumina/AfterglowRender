#include "LocalClock.h"

void LocalClock::update()noexcept {
	_lastUpdateTime = _currentTime;
	_currentTime = std::chrono::high_resolution_clock::now();
	_fps = 1.0e+9 / deltaTime<unit::Nanoseconds>();
}

double LocalClock::fps() const noexcept {
	return _fps;
}

double LocalClock::timeSec() const noexcept {
	return static_cast<double>(time<unit::Nanoseconds>()) * 1e-9;
}

double LocalClock::deltaTimeSec() const noexcept {
	return static_cast<double>(deltaTime<unit::Nanoseconds>()) * 1e-9;
}

double LocalClock::timeSinceLastUpdateSec() const noexcept {
	return static_cast<double>(timeSinceLastUpdate<unit::Nanoseconds>()) * 1e-9;
}
