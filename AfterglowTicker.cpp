#include "AfterglowTicker.h"

#include <thread>
#include "LocalClock.h"


struct AfterglowTicker::Impl {
	LocalClock clock;
	// Unit::Nanosecond
	uint64_t frameInterval = 0;
	uint64_t sleepDelay = 0;
	float maximumFPS = std::numeric_limits<float>::infinity();

	inline void tick();
};

inline void AfterglowTicker::Impl::tick() {
	// Wait for limited framerate and update clock.
	// High precision High CPU occupation.
	// while (clock.timeSinceLastUpdate<unit::Nanoseconds>() < frameInterval) {}

	// Low precision Low CPU occupation.
	uint64_t timeSinceLastUpdate = clock.timeSinceLastUpdate<unit::Nanoseconds>();
	if (timeSinceLastUpdate < frameInterval) {
		uint64_t sleepTime = frameInterval - timeSinceLastUpdate - sleepDelay;
		std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
		sleepDelay = clock.timeSinceLastUpdate<unit::Nanoseconds>() - timeSinceLastUpdate - sleepTime;
	}
	clock.update();
}

AfterglowTicker::AfterglowTicker() : _impl(std::make_unique<Impl>()) {
}

AfterglowTicker::~AfterglowTicker() {
}

float AfterglowTicker::fps() const noexcept {
	return static_cast<float>(_impl->clock.fps());
}

float AfterglowTicker::maximumFPS() const noexcept {
	return _impl->maximumFPS;
}

void AfterglowTicker::setMaximumFPS(float fps) noexcept {
	if (fps == std::numeric_limits<float>::infinity() || fps <= 0.0) {
		_impl->frameInterval = 0;
	}
	else {
		_impl->frameInterval = static_cast<uint64_t>(1.e+9 / fps);
	}
	_impl->maximumFPS = fps;
}

const LocalClock& AfterglowTicker::clock() const noexcept {
	return _impl->clock;
}

void AfterglowTicker::tick() {
	_impl->tick();
}
