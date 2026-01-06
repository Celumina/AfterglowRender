#pragma once
#include <tuple>

// TODO: Generate the reggistered tuple Automatically.
#include "AfterglowCameraComponent.h"
#include "AfterglowStaticMeshComponent.h"
#include "AfterglowTransformComponent.h"
#include "AfterglowDirectionalLightComponent.h"
#include "AfterglowPostProcessComponent.h"
#include "AfterglowComputeComponent.h"
#include "ActionComponentLibrary.h"
#include "TypeMagic.h"
#include "DebugUtilities.h"


namespace reg {
	// Registered Components
	using RegisteredComponentTypes = std::tuple<
		AfterglowCameraComponent, 
		AfterglowStaticMeshComponent, 
		AfterglowTransformComponent, 
		AfterglowDirectionalLightComponent, 
		AfterglowPostProcessComponent, 
		AfterglowComputeComponent, 
		acl::EntityRotator, 
		acl::SimpleController, 
		acl::InteractiveTest
	>;

	template<typename ComponentType>
	concept ActionComponentType = std::is_base_of_v<AfterglowActionComponent<ComponentType>, ComponentType>;

	template<typename ComponentType>
	constexpr bool IsComponentRegistered();

	template<typename ComponentType>
	constexpr static bool IsActionComponent();

	template<typename ComponentType>
	constexpr bool AllComponentsRegistered();

	template<typename FirstComponentType, typename ...OtherComponentTypes>
	constexpr bool AllComponentsRegistered();

	// Cast component type by type_index info.
	template<typename CallbackType>
	void AsType(std::type_index typeIndex, CallbackType&& callback);
};

namespace {
	// Functionalities
	template<typename ComponentType>
	struct Registered {
		constexpr static bool value = false;
	};

	// Form size decrease to zero, and specified Index = 0
	template<typename ComponentType, typename TupleType, size_t Index = std::tuple_size_v<TupleType> - 1>
	struct TupleRegistered {
		constexpr static bool value = (Index < 0)
			? false
			: std::is_same_v<ComponentType, std::tuple_element_t<Index, TupleType>>
			|| TupleRegistered<ComponentType, TupleType, Index - 1>::value;
	};

	template<typename ComponentType, typename TupleType>
	struct TupleRegistered<ComponentType, TupleType, -1> {
		constexpr static bool value = false;
	};
}

template<typename ComponentType>
constexpr bool reg::IsComponentRegistered() {
	return TupleRegistered<ComponentType, RegisteredComponentTypes>::value;
}

template<typename ComponentType>
constexpr bool reg::IsActionComponent() {
	return std::is_base_of_v<AfterglowActionComponent<ComponentType>, ComponentType>;
}

template<typename ComponentType>
constexpr bool reg::AllComponentsRegistered() {
	return IsComponentRegistered<ComponentType>();
}

template<typename FirstComponentType, typename ...OtherComponentTypes>
constexpr bool reg::AllComponentsRegistered() {
	return IsComponentRegistered<FirstComponentType>() && AllComponentsRegistered<OtherComponentTypes...>();
}

template<typename CallbackType>
void reg::AsType(std::type_index typeIndex, CallbackType&& callback) {
	typemagic::AsType<RegisteredComponentTypes>(typeIndex, std::forward<CallbackType>(callback));
}
