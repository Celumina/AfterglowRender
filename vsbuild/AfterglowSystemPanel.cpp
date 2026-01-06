#include "AfterglowSystemPanel.h"

#include "AfterglowSystemUtilities.h"
#include "AfterglowFPSCarousel.h"

struct AfterglowSystemPanel::Impl {
	Impl();

	AfterglowSystemUtilities* sysUtils = nullptr;
	AfterglowPanel* console = nullptr;

	AfterglowFPSCarousel<128> fpsCarousel;
};

AfterglowSystemPanel::Impl::Impl() {
	fpsCarousel.setSmoothFactor(0.2f);
	fpsCarousel.setFrameInterval(8);
}

AfterglowSystemPanel::AfterglowSystemPanel() : 
	AfterglowPanel("SystemStatus"), _impl(std::make_unique<Impl>()) {
}

AfterglowSystemPanel::~AfterglowSystemPanel() {
}

void AfterglowSystemPanel::bindSystemUtilities(AfterglowSystemUtilities& sysUtils) {
	_impl->sysUtils = &sysUtils;
}

void AfterglowSystemPanel::bindConsole(AfterglowPanel& console) {
	_impl->console = &console;
}

void AfterglowSystemPanel::renderContext() {
	auto* sysUtils = _impl->sysUtils;
	if (!sysUtils) {
		return;
	}

	_impl->fpsCarousel.update(static_cast<float>(sysUtils->fps()));
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

	renderReflectionContext(sysUtils, _impl->console);
}
