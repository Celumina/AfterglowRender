#pragma once
#include <typeindex>
#include <format>

#include "TypeConstraints.h"

// Meta-Programming Library.

namespace typemagic {
	// Cast type by type_index info.
	template<constraint::TupleType RegisteredTupleType, typename CallbackType, uint32_t index = 0>
	void AsType(std::type_index typeIndex, CallbackType&& callback);

	// Cast type by element index.
	template<constraint::TupleType RegisteredTupleType, typename CallbackType, uint32_t index = 0>
	void AsType(uint32_t elementIndex, CallbackType&& callback);
}

template<constraint::TupleType RegisteredTupleType, typename CallbackType, uint32_t index>
void typemagic::AsType(std::type_index typeIndex, CallbackType&& callback) {
	using Current = std::tuple_element_t<index, RegisteredTupleType>;
	if (std::type_index(typeid(Current)) == typeIndex) {
		callback.template operator() <Current> ();
		return;
	}
	else if constexpr (index + 1 < std::tuple_size_v<RegisteredTupleType>) {
		typemagic::AsType<RegisteredTupleType, CallbackType, index + 1>(typeIndex, std::forward<CallbackType>(callback));
		return;
	}
	throw std::runtime_error(
		std::format("[typemagic::AsType] TypeIndex \"{}\" is not a registered type from tuple. ", typeIndex.name())
	);
}

template<constraint::TupleType RegisteredTupleType, typename CallbackType, uint32_t index>
void typemagic::AsType(uint32_t elementIndex, CallbackType&& callback) {
	using Current = std::tuple_element_t<index, RegisteredTupleType>;
	if (index == elementIndex) {
		callback.template operator() <Current> ();
		return;
	}
	else if constexpr (index + 1 < std::tuple_size_v<RegisteredTupleType>) {
		typemagic::AsType<RegisteredTupleType, CallbackType, index + 1>(elementIndex, std::forward<CallbackType>(callback));
		return;
	}
	throw std::runtime_error(
		std::format("[typemagic::AsType] ElementIndex \"{}\" is not a in range of the tuple. ", elementIndex)
	);
}
