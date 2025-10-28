#include "AfterglowConsolePanel.h"

#include "AfterglowRenderPanel.h"
#include "AfterglowSystemPanel.h"
#include "AfterglowScenePanel.h"

struct AfterglowConsolePanel::Impl {
	void renderPanels();

	AfterglowRenderPanel renderPanel;
	AfterglowSystemPanel systemPanel;
	AfterglowScenePanel scenePanel;

	std::deque<std::string> outputTexts;
	uint32_t maximumOutputTextCount = 64;
	bool outputTextUpdated = false;

	std::string inputText;
};

AfterglowConsolePanel::AfterglowConsolePanel() : AfterglowPanel("Console"), _impl(std::make_unique<Impl>()) {
	_impl->renderPanel.bindConsole(*this);
	_impl->systemPanel.bindConsole(*this);
	_impl->scenePanel.bindConsole(*this);
	_impl->inputText.resize(256);
}

AfterglowConsolePanel::~AfterglowConsolePanel() {
}

void AfterglowConsolePanel::maximumOutputTextCount(uint32_t count) noexcept {
	_impl->maximumOutputTextCount = count;
}

void AfterglowConsolePanel::bindRenderStatus(AfterglowRenderStatus& renderStatus) noexcept {
	_impl->renderPanel.bindRenderStatus(renderStatus);
}

void AfterglowConsolePanel::bindSystemUtilities(AfterglowSystemUtilities& sysUtils) noexcept {
	_impl->systemPanel.bindSystemUtilities(sysUtils);
	_impl->scenePanel.bindSystemUtilities(sysUtils);
}

void AfterglowConsolePanel::renderContext() {
	auto windowSize = ImGui::GetWindowSize();
	float inputHeight = ImGui::GetFrameHeightWithSpacing();
	float inputPosY = windowSize.y - inputHeight * 2.0;
	float paddingSize = 8.0f;
	ImGui::BeginChild("ConsoleOutput", { ImGui::GetContentRegionAvail().x, inputPosY - paddingSize}, ImGuiChildFlags_Borders);
	ImGui::SetWindowFontScale(0.8f);
	// TODO: Output log here.
	for (const auto& outputText : _impl->outputTexts) {
		ImGui::Text(outputText.data());
	}
	if (_impl->outputTextUpdated) {
		ImGui::SetScrollHereY(1.0f);
		_impl->outputTextUpdated = false;
	}
	ImGui::SetWindowFontScale(1.0f);
	ImGui::EndChild();

	_impl->renderPanels();
	inputPosY += ImGui::GetFrameHeightWithSpacing();

	ImGui::SetCursorPosY(inputPosY); // Optional spacing
	ImGui::SetNextItemWidth(windowSize.x);
	ImGui::InputTextWithHint("##Input", ">", _impl->inputText.data(), _impl->inputText.capacity());
}

void AfterglowConsolePanel::processMessages(const Messages& messages) {
	if (!messages.empty()) {
		_impl->outputTextUpdated = true;
	}
	for(const auto& message : messages) {
		_impl->outputTexts.push_back(message.context);
	}
	while (_impl->outputTexts.size() > _impl->maximumOutputTextCount) {
		_impl->outputTexts.pop_front();
	}
}

void AfterglowConsolePanel::Impl::renderPanels() {
	ImGui::Checkbox(renderPanel.title().data(), renderPanel.isOpenHandle());
	ImGui::SameLine();
	ImGui::Checkbox(systemPanel.title().data(), systemPanel.isOpenHandle());
	ImGui::SameLine();
	ImGui::Checkbox(scenePanel.title().data(), scenePanel.isOpenHandle());

	renderPanel.show();
	systemPanel.show();
	scenePanel.show();
}
