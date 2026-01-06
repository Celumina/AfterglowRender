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
#include "AfterglowComponentBase.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowTransform.h"
#include "AfterglowColor.h"
////////////


class AfterglowPanel : public AfterglowObject {
public:
	struct Message {
		// TODO: MessageTypeEnum (MessageHeader?)
		AfterglowPanel* from;
		std::string context;
	};

	/**
	* @brief: To avoid the label conflition.
	* @usage: Create it and hold in a scope.
	*/ 
	struct ScopeID {
		ScopeID(void* address) { ImGui::PushID(address); }
		~ScopeID() { ImGui::PopID(); };
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
	virtual inline ImVec2 defaultWindowSize() const noexcept { return { 400.0, 200.0 }; };

private: 
	void processCommands() noexcept;

	static void renderInputTypeHint(const char* typeName, const ImVec4& color);

	template<typename Type>
		requires std::is_pointer_v<Type>
	static bool renderPointerInput(Type& bindingValue);

	// @return: Is valid param.
	// TODO: Same type confilt in one function..!!!

	template<typename Type>
	static bool renderInput();

	template<typename Type>
	static bool renderInput(Type& bindingValue);

	template<inreflect::ReflectibleEnumType Type>
	static bool renderInput(Type& bindingValue);

	static bool renderInput(bool& bindingValue);
	static bool renderInput(int& bindingValue);
	static bool renderInput(uint32_t& bindingValue);
	static bool renderInput(float& bindingValue);
	static bool renderInput(double& bindingValue);
	static bool renderInput(std::string& bindingValue);
	static bool renderInput(glm::vec3& bindingValue);
	static bool renderInput(glm::quat& bindingValue);
	static bool renderInput(AfterglowTransform& bindingValue);
	static bool renderInput(AfterglowColor& bindingValue);

	template<typename InstanceType, typename FuncInfoType, typename TupleType>
	inline void dispatchInvocation(InstanceType* obj, AfterglowPanel* messageReceiver, FuncInfoType funcInfo, TupleType& paramTuple);

	std::string _title;
	Messages _unprocessedMessages;
	uint32_t _unprocessedCommands = 0;
	bool _isOpen = false;

	constexpr static const char* _floatFormat = "%.6g";
	constexpr static const char* _doubleFormat = "%.15g";

	/**
	* @brief: For static unique button ID, each function will be allocated a unique id.
	* @usage: static FunctionIDAllocator idAllocator(functionName);
	*/ 
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
		using PureParamTuple = inreflect::PureTuple<ParamTuple>;
		using PointerParamTuple = inreflect::PointerTuple<ParamTuple>;
		static bool paramsValid = true;
		static std::unique_ptr<PureParamTuple> params = nullptr;
		static std::unique_ptr<PointerParamTuple> pointerParams = nullptr;
		// TODO: Mixed mode for custom ref type.
		// Value mode
		if constexpr (inreflect::IsDefaultConstructibleTuple<PureParamTuple>()) {
			// static typename decltype(funcInfo)::DecayedParamTuple params;
			if (!params) {
				params = std::make_unique<PureParamTuple>();
			}
			funcInfo.forEachParam([]<typename ParamType, uint32_t ParamIndex>() {
				paramsValid &= renderInput(std::get<ParamIndex>(*params));
			});
		}
		// Reference mode
		else {
			if (!pointerParams) {
				pointerParams = std::make_unique<PointerParamTuple>();
			}
			// paramsValid = false;
			funcInfo.forEachParam([]<typename ParamType, uint32_t ParamIndex>() {
				paramsValid &= renderPointerInput(std::get<ParamIndex>(*pointerParams));
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
		if constexpr (inreflect::IsDefaultConstructibleTuple<PureParamTuple>()) {
			dispatchInvocation(obj, messageReceiver, funcInfo, *params);
		}
		else {
			// Call from address params
			dispatchInvocation(obj, messageReceiver, funcInfo, *pointerParams);
		}
	});
	ImGui::EndChild();
}

template<typename Type>
requires std::is_pointer_v<Type>
inline bool AfterglowPanel::renderPointerInput(Type& bindingValue) {
	ScopeID id(&bindingValue);
	//ImGui::TextDisabled(std::format("UnknownType: {}", typeid(Type).name()).data());
	//return false;
	renderInputTypeHint("address", { 0.6f, 0.6f, 0.6f, 1.0f });
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text(typeid(Type).name());
		ImGui::EndTooltip();
	}
	ImGui::InputScalar("##address", ImGuiDataType_U64, &bindingValue, nullptr, nullptr, "%llX");
	return true;
}

template<typename Type>
inline bool AfterglowPanel::renderInput() {
	ImGui::TextDisabled(std::format("NonInputType: {}", typeid(Type).name()).data());
	return false;
}

template<typename Type>
static inline bool AfterglowPanel::renderInput(Type& bindingValue) {
	ImGui::TextDisabled(std::format("UnknownType: {}", typeid(Type).name()).data());
	return false;
}

template<inreflect::ReflectibleEnumType Type>
inline bool AfterglowPanel::renderInput(Type& bindingValue) {
	ScopeID id(&bindingValue);
	renderInputTypeHint("enum", { 0.85f, 0.35f, 0.4f, 1.0f });
	static std::array<std::string, Inreflect<Type>::enumCount()> itemStrs;
	static std::once_flag itemStrInitialized;
	std::call_once(itemStrInitialized, [](){
		Inreflect<Type>::forEachAttribute([](auto enumInfo) {
			itemStrs[enumInfo.value] = enumInfo.name;
		});
	});
	static uint32_t selectedIndex = 0;
	if (!ImGui::BeginCombo("##SelectEnum", itemStrs[selectedIndex].data())) {
		return true;
	}
	for (uint32_t index = 0; index < itemStrs.size(); ++index) {
		bool isSelected = (selectedIndex == index);
		if (ImGui::Selectable(itemStrs[index].data(), &isSelected)) {
			selectedIndex = index;
			bindingValue = static_cast<Type>(index);
		}
		if (isSelected) {
			ImGui::SetItemDefaultFocus();
		}
	}
	ImGui::EndCombo();
	return true;
}

template<typename InstanceType, typename FuncInfoType, typename TupleType>
inline void AfterglowPanel::dispatchInvocation(InstanceType* obj, AfterglowPanel* messageReceiver, FuncInfoType funcInfo, TupleType& paramTuple) {
	using Return = typename decltype(funcInfo)::Return;
	static constexpr const char* messageFormat = "[{}] [{}] {}";
	if (!messageReceiver || !(util::Formattable<Return> || std::is_pointer_v<Return> || std::is_reference_v<Return>)) {
		if constexpr (funcInfo.isStatic) {
			funcInfo.call(paramTuple);
		}
		else {
			funcInfo.call(*obj, paramTuple);
		}

	}
	else if constexpr (util::Formattable<Return> && funcInfo.isStatic) {
		sendMessage(
			*messageReceiver,
			std::format(messageFormat, _title, funcInfo.name, funcInfo.call(paramTuple))
		);
	}
	else if constexpr (util::Formattable<Return> && !funcInfo.isStatic) {
		sendMessage(
			*messageReceiver,
			std::format(messageFormat, _title, funcInfo.name, funcInfo.call(*obj, paramTuple))
		);
	}
	// If return is pointer or unformattable reference, print its address.
	else if constexpr (std::is_pointer_v<Return> && funcInfo.isStatic) {
		sendMessage(
			*messageReceiver,
			std::format(messageFormat, _title, funcInfo.name, reinterpret_cast<const void*>(funcInfo.call(paramTuple)))
		);
	}
	else if constexpr (std::is_pointer_v<Return> && !funcInfo.isStatic) {
		sendMessage(
			*messageReceiver,
			std::format(messageFormat, _title, funcInfo.name, reinterpret_cast<const void*>(funcInfo.call(*obj, paramTuple)))
		);
	}
	else if constexpr (std::is_reference_v<Return> && funcInfo.isStatic) {
		sendMessage(
			*messageReceiver,
			std::format(messageFormat, _title, funcInfo.name, reinterpret_cast<const void*>(&funcInfo.call(paramTuple)))
		);
	}
	else if constexpr (std::is_reference_v<Return> && !funcInfo.isStatic) {
		sendMessage(
			*messageReceiver,
			std::format(messageFormat, _title, funcInfo.name, reinterpret_cast<const void*>(&funcInfo.call(*obj, paramTuple)))
		);
	}
}
