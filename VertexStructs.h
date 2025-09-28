#pragma once
#include <tuple>
#include "AfterglowVertex.h"

namespace vert {
	using StandardIndex = uint32_t;

	using VertexP = AfterglowVertex<vert::Position>;
	using VertexPT0 = AfterglowVertex<vert::Position, vert::TexCoord0>;
	using VertexPCT0 = AfterglowVertex<vert::Position, vert::Color, vert::TexCoord0>;
	using VertexPNTBCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Bitangent, vert::Color, vert::TexCoord0>;

	using StandardVertex = VertexPNTBCT0;

	// Register avaliable vertex type here
	using RegisteredVertexTypes = std::tuple<
		VertexP, 
		VertexPT0, 
		VertexPCT0, 
		VertexPNTBCT0
	>;

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
}