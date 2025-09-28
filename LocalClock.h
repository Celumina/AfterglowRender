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
	void update();

	// Unit: Second
	double fps() const;
	double timeSec()  const;
	double deltaTimeSec() const;

	// Time from application start to now;
	template<typename DurationType = unit::Microseconds>
	uint64_t time() const;

	// Current frame spend time
	template<typename DurationType = unit::Microseconds>
	uint64_t deltaTime() const;

private:
	using TimePoint = std::chrono::high_resolution_clock::time_point;

	TimePoint _startTime = std::chrono::high_resolution_clock::now();
	TimePoint _lastUpdateTime = _startTime;
	TimePoint _currentTime = _startTime;
};

template<typename DurationType>
inline uint64_t LocalClock::time() const {
	return std::chrono::duration_cast<DurationType>(_currentTime - _startTime).count();
}

template<typename DurationType>
inline uint64_t LocalClock::deltaTime() const {
	return std::chrono::duration_cast<DurationType>(_currentTime - _lastUpdateTime).count();
}
