#pragma once


#include <memory>

class LocalClock;

class AfterglowTicker {
public:

	AfterglowTicker();
	~AfterglowTicker();

	float fps() const noexcept;
	float maximumFPS() const noexcept;
	// TODO: Set a lower MaxFPS like UE when the window has not on focus?
	void setMaximumFPS(float fps) noexcept;
	
	const LocalClock& clock() const noexcept;

	/**
	* @brief: If current time is not enough to fill the frame interval, this thread will sleep until next frame time.
	*/
	void tick();

private: 
	struct Impl;
	std::unique_ptr<Impl> _impl;
	
};

