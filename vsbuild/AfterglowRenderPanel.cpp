#include "AfterglowRenderPanel.h"

#include <vector>

#include <imgui.h>
#include "AfterglowRenderStatus.h"
#include "AfterglowFPSCarousel.h"

struct AfterglowRenderPanel::Impl {
	Impl();

	AfterglowRenderStatus* renderStatus;
	AfterglowPanel* console = nullptr;

	AfterglowFPSCarousel<128> fpsCarousel;
};

AfterglowRenderPanel::Impl::Impl() {
	fpsCarousel.setSmoothFactor(0.2f);
	fpsCarousel.setFrameInterval(8);
}


AfterglowRenderPanel::AfterglowRenderPanel() : 
	AfterglowPanel("RenderStatus"), 
	_impl(std::make_unique<Impl>()) {
}

AfterglowRenderPanel::~AfterglowRenderPanel() {

}

void AfterglowRenderPanel::bindRenderStatus(AfterglowRenderStatus& renderStatus) {
	_impl->renderStatus = &renderStatus;
}

void AfterglowRenderPanel::bindConsole(AfterglowPanel& console) {
	_impl->console = &console;
}

void AfterglowRenderPanel::renderContext() {
	auto* renderStatus = _impl->renderStatus;
	if (!renderStatus) {
		return;
	}

	_impl->fpsCarousel.update(static_cast<float>(_impl->renderStatus->fps()));
	ImGui::Text("Device: %s", _impl->renderStatus->deviceName());
	ImGui::Text("FPS: %f", _impl->fpsCarousel.smoothFPS());
	ImGui::PlotLines(
		"##FramePerSecond", 
		_impl->fpsCarousel.data(),
		static_cast<int>(_impl->fpsCarousel.size()),
		0, 
		nullptr, 
		0.0f,
		512.0f, 
		ImVec2(ImGui::GetContentRegionAvail().x, 128.0f)
	);

	renderReflectionContext(renderStatus, _impl->console);
}