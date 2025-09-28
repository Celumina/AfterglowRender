#pragma once
#include "AfterglowSemaphores.h"
#include "AfterglowFences.h"
#include "Configurations.h"

class AfterglowSynchronizer : public AfterglowObject {
public:
	enum class SemaphoreFlag {
		ImageAvaliable,  
		RenderFinished, 
		ComputeFinished, 

		EnumCount
	};

	enum class  FenceFlag {
		RenderInFlight, 
		ComputeInFlight, 

		EnumCount
	};

	using InFlightSemaphores = std::array<AfterglowSemaphores::AsElement, cfg::maxFrameInFlight>;
	using InFlightFences = std::array<AfterglowFences::AsElement, cfg::maxFrameInFlight>;
	AfterglowSynchronizer(AfterglowDevice& device);

	// Wait for responsive from GPU.
	void wait(FenceFlag fenceFlag);

	// After waiting, reset fences manually.
	void reset(FenceFlag fenceFlag);

	VkSemaphore& semaphore(SemaphoreFlag semaphoreFlag);
	VkFence& fence(FenceFlag fenceFlag);

	AfterglowDevice& device();

private:
	InFlightSemaphores _inFlightSemaphores;
	InFlightFences _inFlightFences;

	AfterglowDevice& _device;
};

