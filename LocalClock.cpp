#include "LocalClock.h"

void LocalClock::update() {
	_lastUpdateTime = _currentTime;
	_currentTime = std::chrono::high_resolution_clock::now();
}

double LocalClock::fps()  const {
	return 1.0e+9 / deltaTime<unit::Nanoseconds>();
}

double LocalClock::timeSec() const {
	return static_cast<double>(time<unit::Nanoseconds>()) * 1e-9;
}

double LocalClock::deltaTimeSec() const {
	return static_cast<double>(deltaTime<unit::Nanoseconds>()) * 1e-9;
}
