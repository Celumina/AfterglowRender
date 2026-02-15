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


	// Default window size while initize the window.
	ImGui::SetNextWindowSize(defaultWindowSize(), ImGuiCond_FirstUseEver);

	// Here _isOpen replace to nullptr to cancel the close button.
	if (!ImGui::Begin(_title.data(), &_isOpen, windowConfig)) {
		ImGui::End();
		return;
	}

	// Title
	ImGui::SetWindowFontScale(1.2f); 
	ImGui::Text( _title.data());

	// Close button
	ImGui::SetWindowFontScale(0.5f);
	ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() - ImGui::GetStyle().WindowPadding.x);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.2f, 0.2f, 1.0f)); 
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.1f, 0.1f, 1.0f)); 
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.05f, 0.05f, 1.0f));
	if (ImGui::Button(" ", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
		_isOpen = false; // Close the window when button is pressed
	}
	ImGui::PopStyleColor(3);
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

bool AfterglowPanel::renderInput(bool& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("bool", { 0.6f, 0.7f, 0.3f, 1.0f });
	ImGui::Checkbox("##bool", &bindingValue);
	return true;
}

bool AfterglowPanel::renderInput(int& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("int", { 0.3f, 0.7f, 0.4f, 1.0f });
	ImGui::InputInt("##int", &bindingValue);
	return true;
}

bool AfterglowPanel::renderInput(uint8_t& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("uint8", { 0.3f, 0.55f, 0.45f, 1.0f });
	ImGui::InputScalar("##uint8", ImGuiDataType_U8, &bindingValue);
	return true;
}

bool AfterglowPanel::renderInput(uint16_t& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("uint16", { 0.35f, 0.5f, 0.45f, 1.0f });
	ImGui::InputScalar("##uint16", ImGuiDataType_U16, &bindingValue);
	return true;
}

bool AfterglowPanel::renderInput(uint32_t& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("uint", { 0.3f, 0.6f, 0.45f, 1.0f });
	ImGui::InputScalar("##uint", ImGuiDataType_U32, &bindingValue);
	return true;
}

bool AfterglowPanel::renderInput(float& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("float", { 0.3f, 0.5f, 0.8f, 1.0f });
	ImGui::InputFloat("##float", &bindingValue, 0.0f, 0.0f, _floatFormat);
	return true;
}

bool AfterglowPanel::renderInput(double& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("double", { 0.3f, 0.4f, 0.8f, 1.0f });
	ImGui::InputDouble("##double", &bindingValue, 0.0, 0.0, _doubleFormat);
	return true;
}

bool AfterglowPanel::renderInput(std::string& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("string", { 0.8f, 0.45f, 0.35f, 1.0f });
	if (bindingValue.capacity() < 256) {
		bindingValue.resize(256);
	}

	if (ImGui::InputText("##string", bindingValue.data(), bindingValue.capacity())) {
		// Sychronize the string size.
		bindingValue = bindingValue.data();
	}

	return true;
}

bool AfterglowPanel::renderInput(glm::vec3& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("float3", { 0.5f, 0.5f, 0.75f, 1.0f });
	ImGui::InputFloat3("##float3", reinterpret_cast<float*>(&bindingValue), _floatFormat);
	return true;
}

bool AfterglowPanel::renderInput(glm::vec4& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("float4", { 0.4f, 0.5f, 0.65f, 1.0f });
	ImGui::InputFloat4("##float4", reinterpret_cast<float*>(&bindingValue), _floatFormat);
	return true;
}

bool AfterglowPanel::renderInput(glm::quat& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("quaternion", { 0.6f, 0.3f, 0.4f, 1.0f });
	ImGui::InputFloat4("##quaternion", reinterpret_cast<float*>(&bindingValue), _floatFormat);
	return true;
}

bool AfterglowPanel::renderInput(AfterglowTransform& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("translation", { 0.4f, 0.4f, 0.75f, 1.0f });
	ImGui::InputFloat3("##translation", reinterpret_cast<float*>(&bindingValue.translation), _floatFormat);
	renderInputTypeHint("rotation", { 0.6f, 0.3f, 0.4f, 1.0f });
	ImGui::InputFloat4("##rotation", reinterpret_cast<float*>(&bindingValue.rotation), _floatFormat);
	renderInputTypeHint("scaling", { 0.4f, 0.4f, 0.75f, 1.0f });
	ImGui::InputFloat3("##scaling", reinterpret_cast<float*>(&bindingValue.scaling), _floatFormat);
	return true;
}

bool AfterglowPanel::renderInput(AfterglowColor& bindingValue) {
	ScopeID id(&bindingValue);
	static ImVec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
	renderInputTypeHint("color", { 0.85f, 0.45f, 0.35f, 1.0f });
	//if (ImGui::ColorEdit4("##color", reinterpret_cast<float*>(&color))) {
	//	bindingValue.setFloatRGBA(color.x, color.y, color.z, color.w);
	//}
	//ImGui::SameLine();
	// ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::ColorPicker4("##color", reinterpret_cast<float*>(&color), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview)) {
		bindingValue.setFloatRGBA(color.x, color.y, color.z, color.w);
	}
	return true;
}

AfterglowPanel::FunctionIDAllocator::FunctionIDAllocator(const char* functionName) : 
	_label(std::format("{}##{}", functionName, _functionAllocatedID)) {
	++_functionAllocatedID;
}

const char* AfterglowPanel::FunctionIDAllocator::uniqueLabel() {
	return _label.data();
}

