#include "AfterglowRenderStatus.h"

#include "vulkan/vulkan.h"

#include "AfterglowRenderer.h"
#include "AfterglowTicker.h"

struct AfterglowRenderStatus::Impl {
	AfterglowRenderer& renderer;
};

AfterglowRenderStatus::AfterglowRenderStatus(AfterglowRenderer& renderer) : 
	_impl(std::make_unique<Impl>(renderer)) {
}

AfterglowRenderStatus::~AfterglowRenderStatus() {
}

float AfterglowRenderStatus::fps() const noexcept {
	return _impl->renderer.ticker().fps();
}

float AfterglowRenderStatus::maximumFPS() const noexcept {
	return _impl->renderer.ticker().maximumFPS();
}

void AfterglowRenderStatus::setMaximumFPS(float fps) noexcept {
	_impl->renderer.ticker().setMaximumFPS(fps);
}

const char* AfterglowRenderStatus::deviceName() noexcept {
	return _impl->renderer.physicalDeviceProperties().deviceName;
}

void AfterglowRenderStatus::INVALID_PARAM_TEST(double d) {
}

