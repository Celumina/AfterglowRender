#pragma once
#include <stdint.h>
#include <cstring>
#include <type_traits>
#include <concepts>

namespace vert {
	using Text = const char*;

	template<typename Attribute>
	struct Vec4 {
		union { Attribute::Component x, r, s; };
		union { Attribute::Component y, g, t; };
		union { Attribute::Component  z, b, p; };
		union { Attribute::Component w, a, q; };
	};

	template<typename ComponentType, uint32_t NumComponentValue>
	struct AttributeTemplate {
		using Component = ComponentType;
		static constexpr uint32_t byteSize = sizeof(Component) * NumComponentValue;
		static constexpr uint32_t numComponents = NumComponentValue;
		static constexpr Text name = "attributeTemplate";
	};

	// These name use for shader variables.
	struct Position : AttributeTemplate<float, 3> { static constexpr Text name = "position"; };
	struct Normal : AttributeTemplate<float, 3> { static constexpr Text name = "normal"; };
	struct Tangent : AttributeTemplate<float, 3> { static constexpr Text name = "tangent"; };
	struct Bitangent : AttributeTemplate<float, 3> { static constexpr Text name = "bitangent"; };
	struct Color : AttributeTemplate<float, 4> { static constexpr Text name = "color"; };
	struct TexCoord0 : AttributeTemplate<float, 2> { static constexpr Text name = "texCoord0"; };
	struct TexCoord1 : AttributeTemplate<float, 2> { static constexpr Text name = "texCoord1"; };
	struct TexCoord2 : AttributeTemplate<float, 2> { static constexpr Text name = "texCoord2"; };
	struct TexCoord3 : AttributeTemplate<float, 2>{ static constexpr Text name = "texCoord3"; };

	struct PositionH : AttributeTemplate<double, 3> { static constexpr Text name = "positionH"; };
	struct NormalH : AttributeTemplate<double, 3> { static constexpr Text name = "normalH"; };
	struct TangentH : AttributeTemplate<double, 3> { static constexpr Text name = "tangentH"; };
	struct BitangentH : AttributeTemplate<double, 3> { static constexpr Text name = "bitangentH"; };
	struct ColorH : AttributeTemplate<double, 4> { static constexpr Text name = "colorH"; };
	struct TexCoord0H : AttributeTemplate<double, 2> { static constexpr Text name = "texCoord0H"; };
	struct TexCoord1H : AttributeTemplate<double, 2> { static constexpr Text name = "texCoord1H"; };
	struct TexCoord2H : AttributeTemplate<double, 2> { static constexpr Text name = "texCoord2H"; };
	struct TexCoord3H : AttributeTemplate<double, 2> { static constexpr Text name = "texCoord3H"; };

	template<typename Type>
	concept HasAttributeBaseDefinition = 
	requires {
		typename Type::Component;
		Type::byteSize;
		Type::numComponents;
		Type::name;
	};

	template<typename Type>
	constexpr bool IsAttribute();

	template<typename Type>
	concept AttributeType = requires { 
		IsAttribute<Type>(); 
	};
}

// e.g. auto v = AfterglowVertex<float, vert::Position, vert::Color, vert::UV0>;
//  v.set<vert::Position>({0.5, 0.5, 0.0, 1.0});
// v.set<vert::TexCoord0>({0.1, 0.2});
template<vert::AttributeType ...Attributes>
class AfterglowVertex {
public:
	// Type Params Check
	template <typename ...Attributes>
	struct AttributesValidation;

	template<>
	struct AttributesValidation<> {
		static constexpr bool value = true;
	};

	template<typename FirstAttribute, typename ...OtherAttributes>
	struct AttributesValidation<FirstAttribute, OtherAttributes...> {
		static constexpr bool value = std::is_base_of_v<vert::AttributeTemplate, FirstAttribute>&& AttributesValidation<OtherAttributes>::value;
	};

	static_assert(
		AttributesValidation<Attributes>::value, 
		"All these Attribute Types should derive from vert::AttributeTemplate."
		);

	template<typename Attribute>
	static constexpr uint32_t byteOffset();

	template<typename Attribute>
	void set(const vert::Vec4<Attribute>& value);

	template<typename Attribute>
	const vert::Vec4<Attribute> get();

	// Vertex attribute count.
	static constexpr uint32_t count();

	// Vertex attribute order index;
	template<typename Attribute>
	static constexpr uint32_t index();

	template<typename Attribute>
	static constexpr bool hasAttribute();

	// Empty type as a error type return.
	struct Empty;

	// First Attribute
	template<typename...Attributes>
	struct FirstAttribute;

	template<>
	struct FirstAttribute<> {
		// Represent a null type.
		using Type = Empty;
	};

	template<typename First, typename... Others>
	struct FirstAttribute<First, Others...> {
		using Type = First;
	};

	using First = FirstAttribute<Attributes...>::Type;

	// Next Attribute
	template<typename...Attributes>
	struct NextAttribute;

	template<typename Source>
	struct NextAttribute<Source> {
		using Type = Empty;
	};

	template<typename Source, typename First>
	struct NextAttribute<Source, First> {
		using Type = Empty;
	};

	template<typename Source, typename First, typename Next, typename... Others>
	struct NextAttribute<Source, First, Next, Others...> {
		// Remember that use a using Type require a typename ahead.
		using Type = std::conditional<std::is_same_v<Source, First>, Next, typename NextAttribute<Source, Next, Others...>::Type>::type;
	};

	template<typename Source>
	using Next = NextAttribute<Source, Attributes...>::Type;

private:
	template <typename Attribute>
	using Component = Attribute::Component;

	// Static Assert
	template<typename TargetAttribute, typename... Attributes>
	struct AttributeInPack;

	template<typename TargetAttribute>
	struct AttributeInPack<TargetAttribute> {
		static constexpr bool value = false;
	};

	template<typename TargetAttribute, typename FirstAttribute, typename... OtherAttributes>
	struct AttributeInPack<TargetAttribute, FirstAttribute, OtherAttributes...> {
		static constexpr bool value = std::is_same_v<TargetAttribute, FirstAttribute>
			|| AttributeInPack<TargetAttribute, OtherAttributes...>::value;
	};

	// Vertex Byte Size
	template<typename ...Attributes>
	struct ByteSize;

	template<>
	struct ByteSize<> {
		static constexpr uint32_t value = 0;
	};

	template<typename FirstAttribute, typename ...OtherAttributes>
	struct ByteSize<FirstAttribute, OtherAttributes...> {
		static constexpr uint32_t value = FirstAttribute::numComponents * sizeof(FirstAttribute::Component) + ByteSize<OtherAttributes...>::value;
	};

	// Attribute Byte Offset
	template<typename TargetAttribute, typename... AttributeValues>
	struct Offset;

	template<typename TargetAttribute>
	struct Offset<TargetAttribute> {
		static constexpr uint32_t value = 0;
	};

	template<typename TargetAttribute, typename FirstAttribute, typename... OtherAttributes>
	struct Offset<TargetAttribute, FirstAttribute, OtherAttributes...> {
		static constexpr uint32_t value = !std::is_same_v<TargetAttribute, FirstAttribute> ?
			FirstAttribute::byteSize + Offset<TargetAttribute, OtherAttributes...>::value : 0;
	};

	// Attribute's Component Byte Offset
	template<typename Attribute, uint32_t ComponentIndex, typename... Attributes>
	struct ComponentOffset;

	template<typename Attribute, uint32_t ComponentIndex, typename... Attributes>
	struct ComponentOffset<Attribute, ComponentIndex, Attributes...> {
		static constexpr uint32_t value = Offset<Attribute, Attributes...>::value + sizeof(Attribute::Component) * ComponentIndex;
	};

	// Attribute Count
	template<typename... Attributes>
	struct Count;

	template<>
	struct Count<> {
		static constexpr uint32_t value = 0;
	};

	template<typename FirstAttribute, typename... OtherAttributes>
	struct Count<FirstAttribute, OtherAttributes...> {
		static constexpr uint32_t value = 1 + Count<OtherAttributes...>::value;
	};

	// Attribute Index
	template<typename TargetAttribute, typename... Attributes>
	struct Index;

	template<typename TargetAttribute>
	struct Index<TargetAttribute> {
		static constexpr uint32_t value = 0;
	};

	template<typename TargetAttribute, typename FirstAttribute, typename... OtherAttributes>
	struct Index<TargetAttribute, FirstAttribute, OtherAttributes...> {
		static constexpr uint32_t value = !std::is_same_v<TargetAttribute, FirstAttribute> ?
			1 + Index<TargetAttribute, OtherAttributes...>::value : 0;
	};

	template<typename Attribute>
	void set(const Component<Attribute>* value);

	char _data[ByteSize<Attributes...>::value] = { 0 };
};

template<vert::AttributeType... Attributes>
template <typename Attribute>
inline void AfterglowVertex<Attributes...>::set(const Component<Attribute>* value) {
	static_assert(
		AttributeInPack<Attribute, Attributes...>::value, 
		"[AfterglowVertex::set] Attribute is not in AfterglowVertex. "
		);
	memcpy_s(&_data[Offset<Attribute, Attributes...>::value], Attribute::byteSize, value, Attribute::byteSize);
}

template<vert::AttributeType... Attributes>
template<typename Attribute>
inline constexpr uint32_t AfterglowVertex<Attributes...>::byteOffset() {
	static_assert(
		AttributeInPack<Attribute, Attributes...>::value, 
		"[AfterglowVertex::byteOffset] Attribute is not in AfterglowVertex. "
		);
	return Offset<Attribute, Attributes...>::value;
}

template<vert::AttributeType... Attributes>
template<typename Attribute>
inline void AfterglowVertex<Attributes...>::set(const vert::Vec4<Attribute>& value) {
	static_assert(
		AttributeInPack<Attribute, Attributes...>::value, 
		"[AfterglowVertex::set] Attribute is not in AfterglowVertex. "
		);
	set<Attribute>(reinterpret_cast<const Attribute::Component*>(&value));
}

template<vert::AttributeType... Attributes>
template <typename Attribute>
inline const vert::Vec4<Attribute> AfterglowVertex<Attributes...>::get() {
	static_assert(
		AttributeInPack<Attribute, Attributes...>::value, 
		"[AfterglowVertex::get] Attribute is not in AfterglowVertex. "
		);

	vert::Vec4<Attribute::Component> out = { 0 };
	memcpy_s(&out, Attribute::byteSize, &_data[Offset<Attribute, Attributes...>::value], Attribute::size);
	return out;
}

template<vert::AttributeType... Attributes>
inline constexpr uint32_t AfterglowVertex<Attributes...>::count() {
	return Count<Attributes...>::value;
}

template<vert::AttributeType... Attributes>
template<typename Attribute>
inline constexpr uint32_t AfterglowVertex<Attributes...>::index() {
	static_assert(
		AttributeInPack<Attribute, Attributes...>::value, 
		"[AfterglowVertex::index] Attribute is not in AfterglowVertex. "
		);
	return Index<Attribute, Attributes...>::value;
}

template<vert::AttributeType ...Attributes>
template<typename Attribute>
inline constexpr bool AfterglowVertex<Attributes...>::hasAttribute() {
	return AttributeInPack<Attribute, Attributes...>::value;
}

template<typename Type>
constexpr bool vert::IsAttribute() {
	if constexpr (HasAttributeBaseDefinition<Type>) {
		if constexpr (std::is_base_of_v<AttributeTemplate<typename Type::Component, Type::numComponents>, Type>) {
			return true;
		}
	}
	return false;
}