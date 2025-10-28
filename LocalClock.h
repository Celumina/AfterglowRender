#pragma once
#include <chrono>

namespace unit {
	using Nanoseconds = std::chrono::nanoseconds;
	using Microseconds = std::chrono::microseconds;
	using Milliseconds = std::chrono::milliseconds;
	using Seconds = std::chrono::seconds;
	using Minutes = std::chrono::minutes;
	using Hours = std::chrono::hours;
	using Days = std::chrono::days;
	using Weeks = std::chrono::weeks;
	using Months = std::chrono::months;
	using Years = std::chrono::years;
};

// Regularly, projection independent classes should not add a prefix.
class LocalClock {
public:
	// Call this function once every frame for evaluate delta time.
	void update() noexcept;

	// Unit: Second
	double fps() const noexcept;
	double timeSec() const noexcept;
	double deltaTimeSec() const noexcept;
	double timeSinceLastUpdateSec() const noexcept;

	// Time from application start to now;
	template<typename DurationType = unit::Microseconds>
	uint64_t time() const noexcept;

	// Current frame spend time
	template<typename DurationType = unit::Microseconds>
	uint64_t deltaTime() const noexcept;

	template<typename DurationType = unit::Microseconds>
	uint64_t timeSinceLastUpdate() const noexcept;

private:
	using TimePoint = std::chrono::high_resolution_clock::time_point;

	TimePoint _startTime = std::chrono::high_resolution_clock::now();
	TimePoint _lastUpdateTime = _startTime;
	TimePoint _currentTime = _startTime;
	double _fps = 0.0;

};

template<typename DurationType>
inline uint64_t LocalClock::time() const noexcept {
	return std::chrono::duration_cast<DurationType>(_currentTime - _startTime).count();
}

template<typename DurationType>
inline uint64_t LocalClock::deltaTime() const noexcept {
	return std::chrono::duration_cast<DurationType>(_currentTime - _lastUpdateTime).count();
}

template<typename DurationType>
inline uint64_t LocalClock::timeSinceLastUpdate() const noexcept {
	return std::chrono::duration_cast<DurationType>(
		std::chrono::high_resolution_clock::now() - _currentTime
	).count();
}
