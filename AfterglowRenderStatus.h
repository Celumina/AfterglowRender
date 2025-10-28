#pragma once

#include <memory>

#include "Inreflect.h"

class AfterglowRenderer;

class AfterglowRenderStatus {
public:
	AfterglowRenderStatus(AfterglowRenderer& renderer);
	~AfterglowRenderStatus();

	float fps() const noexcept;
	float maximumFPS() const noexcept;

	void setMaximumFPS(float fps) noexcept;

	const char* deviceName() noexcept;

	void INVALID_PARAM_TEST(double d);

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};


INR_CLASS(AfterglowRenderStatus) {
	INR_FUNCS (
		INR_FUNC(fps), 
		INR_FUNC(maximumFPS),
		INR_FUNC(setMaximumFPS), 
		INR_FUNC(deviceName), 
		INR_FUNC(INVALID_PARAM_TEST)
	);
};
