#pragma once
#include <tuple>
#include <vector>
#include "AfterglowVertex.h"
#include "TypeMagic.h"

namespace vert {
	using StandardIndex = uint32_t;

	using VertexP = AfterglowVertex<vert::Position>;

	using VertexPN = AfterglowVertex<vert::Position, vert::Normal>;
	using VertexPT = AfterglowVertex<vert::Position, vert::Tangent>;
	using VertexPC = AfterglowVertex<vert::Position, vert::Color>;
	using VertexPT0 = AfterglowVertex<vert::Position, vert::TexCoord0>;

	using VertexPNT = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent>;
	using VertexPNC = AfterglowVertex<vert::Position, vert::Normal, vert::Color>;
	using VertexPNT0 = AfterglowVertex<vert::Position, vert::Normal, vert::TexCoord0>;
	using VertexPTC = AfterglowVertex<vert::Position, vert::Tangent, vert::Color>;
	using VertexPTT0 = AfterglowVertex<vert::Position, vert::Tangent, vert::TexCoord0>;
	using VertexPCT0 = AfterglowVertex<vert::Position, vert::Color, vert::TexCoord0>;

	using VertexPNTC = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Color>;
	using VertexPNTT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::TexCoord0>;
	using VertexPNCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Color, vert::TexCoord0>;
	using VertexPTCT0 = AfterglowVertex<vert::Position, vert::Tangent, vert::Color, vert::TexCoord0>;

	using VertexPNTCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Color, vert::TexCoord0>;
	using VertexPNTBCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Bitangent, vert::Color, vert::TexCoord0>;

	using StandardVertex = VertexPNTCT0;

	// Register avaliable vertex type here
	using RegisteredVertexTypes = std::tuple<
		VertexP, 

		VertexPN, 
		VertexPT,
		VertexPC, 
		VertexPT0, 

		VertexPNT, 
		VertexPNC,
		VertexPNT0, 
		VertexPTC, 
		VertexPTT0, 
		VertexPCT0, 

		VertexPNTC, 
		VertexPNTT0, 
		VertexPNCT0,
		VertexPTCT0, 

		VertexPNTCT0, 
		VertexPNTBCT0
	>;

	using IndexArray = std::vector<vert::StandardIndex>;
	// For dynamic vertex types.
	using VertexData = std::vector<char>;

	template<typename ...Types>
	struct IsVertexTrait {
		constexpr static bool value = false;
	};

	template<typename ...Types>
	struct IsVertexTrait<AfterglowVertex<Types...>> {
		constexpr static bool value = true;
	};

	template<typename Type>
	constexpr bool IsVertex() { return IsVertexTrait<Type>::value; };

	template<typename Type>
	concept VertexType = IsVertexTrait<Type>::value;

	template<VertexType Type, uint32_t index = 0>
	constexpr uint32_t TypeID();

	template<typename CallbackType>
	void AsType(std::type_index typeIndex, CallbackType&& callback);
}

template<vert::VertexType Type, uint32_t index>
constexpr uint32_t vert::TypeID() {
	if constexpr (std::is_same_v<std::tuple_element_t<index, RegisteredVertexTypes>, Type>) {
		return index;
	}
	else if constexpr (index < std::tuple_size_v<RegisteredVertexTypes>) {
		return TypeID<Type, index + 1>();
	}
	else {
		static_assert(
			index < std::tuple_size_v<RegisteredVertexTypes>,
			"[vert::TypeID] Type is not found in RegisteredVertexTypes."
		);
	}
}

template<typename CallbackType>
void vert::AsType(std::type_index typeIndex, CallbackType&& callback) {
	typemagic::AsType<RegisteredVertexTypes>(typeIndex, std::forward<CallbackType>(callback));
}
