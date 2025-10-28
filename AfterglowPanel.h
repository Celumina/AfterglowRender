#pragma once

#include <string>
#include <vector>

#include <imgui.h>
#include "AfterglowObject.h"
#include "AfterglowUtilities.h"
#include "Inreflect.h"


////////////
// For UI support (Maybe move some where without dependencies?)
#include "AfterglowEntity.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowTransform.h"
#include "AfterglowColor.h"
////////////


class AfterglowColor;

class AfterglowPanel : public AfterglowObject {
public:
	struct Message {
		// TODO: MessageTypeEnum (MessageHeader?)
		AfterglowPanel* from;
		std::string context;
	};

	using Messages = std::vector<Message>;

	// Greater value command has higer priority
	enum class Command : uint32_t {
		None = 0, 
		ShowPanel = 1 << 0, 
		HidePanel = 1 << 1
	};

	AfterglowPanel(const std::string& title = "");

	// @brief: If _isOpen == false, panel will not be show and not additional cost.
	void show();

	bool isOpen() const noexcept;
	void open() noexcept;
	void close() noexcept;
	bool* isOpenHandle() noexcept;

	const std::string& title() noexcept;
	void setTitle(const std::string& title) noexcept;

protected:
	// @param obj: if equal to nullptr, provides static members only.
	template<typename Type>
	void renderReflectionContext(Type* obj = nullptr, AfterglowPanel* messageReceiver = nullptr);

	// @brief: Provides const functions and attributes only.
	template<typename Type>
	void renderReflectionContext(const Type* obj = nullptr, AfterglowPanel* messageReceiver = nullptr);

	// @brief: Communicate with other panels.
	void sendMessage(AfterglowPanel& otherPanel, const std::string& messageContext);
	void sendCommand(AfterglowPanel& otherPanel, Command command);

	virtual void renderContext() = 0;
	virtual void processMessages(const Messages& messages) {};

private: 
	void processCommands() noexcept;

	static void renderInputTypeHint(const char* typeName, const ImVec4& color);

	// @return: Is valid param.
	// TODO: Same type confilt in one function..!!!
	template<typename Type>
	static bool renderInput();

	template<typename Type>
	static bool renderInput(Type* bindingValue);

	template<typename Type>
	static bool renderInput(Type& bindingValue);

	template<>
	static bool renderInput<bool>(bool& bindingValue);

	template<>
	static bool renderInput<int>(int& bindingValue);

	template<>
	static bool renderInput<uint32_t>(uint32_t& bindingValue);

	template<>
	static bool renderInput<float>(float& bindingValue);

	template<>
	static bool renderInput<double>(double& bindingValue);

	template<>
	static bool renderInput<std::string>(std::string& bindingValue);

	template<>
	static bool renderInput<glm::vec3>(glm::vec3& bindingValue);

	template<>
	static bool renderInput<glm::quat>(glm::quat& bindingValue);

	template<>
	static bool renderInput<AfterglowTransform>(AfterglowTransform& bindingValue);

	template<>
	static bool renderInput<AfterglowColor>(AfterglowColor& bindingValue);

	std::string _title;
	Messages _unprocessedMessages;
	uint32_t _unprocessedCommands = 0;
	bool _isOpen = false;

	/**
	* @brief: For static unique button ID, each function will be allocated a unique id.
	* @usage: static FunctionIDAllocator idAllocator(functionName);
	*/ 
	friend struct FunctionIDAllocator;
	struct FunctionIDAllocator { 
	public:
		FunctionIDAllocator(const char* functionName);
		const char* uniqueLabel();
		
	private:
		std::string _label;
		static inline uint32_t _functionAllocatedID = 0;
	};
};


INR_CLASS(AfterglowPanel::Command) {
	INR_ATTRS(
		INR_ENUM(None),
		INR_ENUM(ShowPanel),
		INR_ENUM(HidePanel)
	);
};

template<typename Type>
inline void AfterglowPanel::renderReflectionContext(Type* obj, AfterglowPanel* messageReceiver) {
	std::string instanceName(std::format("{}##{}", typeid(std::remove_pointer_t<decltype(obj)>).name(), reinterpret_cast<void*>(obj)));
	bool expanded = ImGui::CollapsingHeader(instanceName.data(), ImGuiTreeNodeFlags_DefaultOpen);
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text(std::format("InstanceAddress: {}", reinterpret_cast<void*>(obj)).data());
		ImGui::EndTooltip();
	}
	if (!expanded) {
		return;
	}
	ImGui::BeginChild(instanceName.data(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
	Inreflect<Type>::forEachFunction([this, &obj, &messageReceiver](auto funcInfo){
		static FunctionIDAllocator idAllocator(funcInfo.name.data());
		ImGui::BeginChild(funcInfo.name.data(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
		ImGui::Text(funcInfo.name.data());
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text(typeid(typename decltype(funcInfo)::Function).name());
			ImGui::EndTooltip();
		}
		ImGui::Separator();
		// ImGui::SeparatorText(funcInfo.name.data());
		// TODO: Cool! let make all forEach Type use this method.
		using ParamTuple = typename decltype(funcInfo)::ParamTuple;
		using DecayedParamTuple = inreflect::DecayedTuple<ParamTuple>::Tuple;
		static bool paramsValid = true;
		static std::unique_ptr<DecayedParamTuple> params = nullptr;
		if constexpr (inreflect::IsDefaultConstructibleTuple<DecayedParamTuple>()) {
			// static typename decltype(funcInfo)::DecayedParamTuple params;
			if (!params) {
				params = std::make_unique<DecayedParamTuple>();
			}
			funcInfo.forEachParam([]<typename ParamType, uint32_t ParamIndex>() {
				paramsValid &= renderInput(std::get<ParamIndex>(*params));
			});
		}
		else {
			// TODO: These refs are hardly to hanlde.
			paramsValid = false;
			funcInfo.forEachParam([]<typename ParamType, uint32_t ParamIndex>() {
				if constexpr (std::is_default_constructible_v<ParamType>) {
					// Arbitrary param, it will never be used.
					static ParamType param;
					renderInput(&param);
				}
				else {
					renderInput<ParamType>();
				}
			});
		}
		// Invalid button.
		if (!paramsValid) {
			ImGui::BeginDisabled(true);
			ImGui::Button(idAllocator.uniqueLabel(), {ImGui::GetContentRegionAvail().x, 0});
			ImGui::EndDisabled();
			ImGui::EndChild();
			return;
		}
		// Valid button.
		if (!ImGui::Button(idAllocator.uniqueLabel(), { ImGui::GetContentRegionAvail().x, 0 })) {
			ImGui::EndChild();
			return;
		}
		ImGui::EndChild();
		// Call functions when the button click down.
		// TODO: If return is pointer, print its instance id, if param type is pointer or ref, Provide a pointer input.
		if constexpr (inreflect::IsDefaultConstructibleTuple<DecayedParamTuple>()) {
			if (!messageReceiver || !util::Formattable<typename decltype(funcInfo)::Return>) {
				if constexpr (funcInfo.isStatic) {
					funcInfo.call(*params);
				}
				else {
					funcInfo.call(*obj, *params);
				}
			
			}
			// TODO: Print nothing from std::string&?
			else if constexpr (util::Formattable<typename decltype(funcInfo)::Return>) {
				if constexpr (funcInfo.isStatic) {
					sendMessage(
						*messageReceiver,
						std::format("[{}] [{}] {}", _title, funcInfo.name, funcInfo.call(*params))
					);
				}
				else {
					sendMessage(
						*messageReceiver,
						std::format("[{}] [{}] {}", _title, funcInfo.name, funcInfo.call(*obj, *params))
					);
				}
			}
		}
	});
	ImGui::EndChild();
}

template<typename Type>
inline bool AfterglowPanel::renderInput() {
	ImGui::TextDisabled(std::format("NonInputType: {}", typeid(Type).name()).data());
	return false;
}

template<typename Type>
inline bool AfterglowPanel::renderInput(Type* bindingValue) {
	ImGui::TextDisabled(std::format("UnknownType: {}", typeid(Type).name()).data());
	return false;
}

template<typename Type>
static inline bool AfterglowPanel::renderInput(Type& bindingValue) {
	ImGui::TextDisabled(std::format("UnknownType: {}", typeid(Type).name()).data());
	return false;
}

template<>
inline bool AfterglowPanel::renderInput(bool& bindingValue) {
	renderInputTypeHint("bool", { 0.6f, 0.7f, 0.3f, 1.0f });
	ImGui::Checkbox("##bool", &bindingValue);
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(int& bindingValue) {
	renderInputTypeHint("int", { 0.3f, 0.7f, 0.4f, 1.0f });
	ImGui::InputInt("##int", &bindingValue);
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(uint32_t& bindingValue) {
	renderInputTypeHint("uint", { 0.3f, 0.6f, 0.45f, 1.0f });
	ImGui::InputScalar("##uint", ImGuiDataType_U32, &bindingValue);
	return true;
}

template<>
static inline bool AfterglowPanel::renderInput<float>(float& bindingValue) {
	renderInputTypeHint("float", {0.3f, 0.5f, 0.8f, 1.0f});
	ImGui::InputFloat("##float", &bindingValue);
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(double& bindingValue) {
	renderInputTypeHint("double", { 0.3f, 0.4f, 0.8f, 1.0f });
	ImGui::InputDouble("##double", &bindingValue);
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(std::string& bindingValue) {
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

template<>
inline bool AfterglowPanel::renderInput(glm::vec3& bindingValue) {
	renderInputTypeHint("float3", { 0.5f, 0.5f, 0.75f, 1.0f });
	ImGui::InputFloat3("##float3", reinterpret_cast<float*>(&bindingValue));
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(glm::quat& bindingValue) {
	renderInputTypeHint("quaternion", { 0.6f, 0.3f, 0.4f, 1.0f });
	ImGui::InputFloat4("##quaternion", reinterpret_cast<float*>(&bindingValue));
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(AfterglowTransform& bindingValue) {
	renderInputTypeHint("translation", { 0.4f, 0.4f, 0.75f, 1.0f });
	ImGui::InputFloat3("##translation", reinterpret_cast<float*>(&bindingValue.translation));
	renderInputTypeHint("rotation", { 0.6f, 0.3f, 0.4f, 1.0f });
	ImGui::InputFloat4("##rotation", reinterpret_cast<float*>(&bindingValue.rotation));
	renderInputTypeHint("scaling", { 0.4f, 0.4f, 0.75f, 1.0f });
	ImGui::InputFloat3("##scaling", reinterpret_cast<float*>(&bindingValue.scaling));
	return true;
}

template<>
inline bool AfterglowPanel::renderInput(AfterglowColor& bindingValue) {
	static ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
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
