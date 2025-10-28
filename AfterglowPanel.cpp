#include "AfterglowPanel.h"

AfterglowPanel::AfterglowPanel(const std::string& title) : 
	_title(title) {
}

void AfterglowPanel::show() {
	if (!_isOpen) {
		return;
	}

	// Communications.
	processCommands();
	processMessages(_unprocessedMessages);
	_unprocessedCommands = 0;
	_unprocessedMessages.clear();

	static constexpr auto windowConfig = ImGuiWindowFlags_NoTitleBar;
		// ImGuiWindowFlags_NoCollapse;

	// Here _isOpen replace to nullptr to cancel the close button.
	if (!ImGui::Begin(_title.data(), &_isOpen, windowConfig)) {
		ImGui::End();
		return;
	}

	// Title
	ImGui::SetWindowFontScale(1.2f); 
	ImGui::Text( _title.data());
	ImGui::SetWindowFontScale(1.0f);
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::BeginChild("RenderContext");
	renderContext();
	ImGui::EndChild();

	ImGui::End();
}

bool AfterglowPanel::isOpen() const noexcept {
	return _isOpen;
}

void AfterglowPanel::open() noexcept {
	_isOpen = true;
}

void AfterglowPanel::close() noexcept {
	_isOpen = false;
}

bool* AfterglowPanel::isOpenHandle() noexcept {
	return &_isOpen;
}

void AfterglowPanel::sendMessage(AfterglowPanel& otherPanel, const std::string& messageContext) {
	otherPanel._unprocessedMessages.emplace_back(this, messageContext);
}

void AfterglowPanel::sendCommand(AfterglowPanel& otherPanel, Command command) {
	otherPanel._unprocessedCommands |= util::EnumValue(command);
}

const std::string& AfterglowPanel::title() noexcept {
	return _title;
}

void AfterglowPanel::setTitle(const std::string& title) noexcept {
	_title = title;
}

void AfterglowPanel::processCommands() noexcept {
	if (!_unprocessedCommands) {
		return;
	}
	Inreflect<Command>::forEachAttribute([this](auto enumInfo){
		if (!(enumInfo.value & _unprocessedCommands)) {
			return;
		}
		switch (enumInfo.raw) {
		case (Command::ShowPanel):
			_isOpen = true;
			break;
		case (Command::HidePanel):
			_isOpen = false;
			break;
		default:
			break;
		}
	});
}

void AfterglowPanel::renderInputTypeHint(const char* typeName, const ImVec4& color) {
	ImGui::PushStyleColor(ImGuiCol_Button, color);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	ImGui::Button(typeName);
	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
}

AfterglowPanel::FunctionIDAllocator::FunctionIDAllocator(const char* functionName) : 
	_label(std::format("{}##{}", functionName, _functionAllocatedID)) {
	++_functionAllocatedID;
}

const char* AfterglowPanel::FunctionIDAllocator::uniqueLabel() {
	return _label.data();
}

