#pragma once
#include <vector>
#include <unordered_map>
#include <functional>

#include "VertexStructs.h"
#include "Inreflect.h"

#include "AfterglowUtilities.h"

// Runtime GPU-CPU Interactive Structure.

/* HLSL Packing Rules (similar to std140)
- Base types (e.g. int, float) align to 4 bytes.
- 4 components as a group (16 bytes)
- If next attribute over than current 4 components group, then begin from a new group.
- If have not enough group bytes at the end of the struct, complier will append some padding to suit 16 bytes alignment.
- Struct will start a new group (different from std140).

<16 bytes>
struct Case0 {
	float a;
	float2 b;
	<padding 4 bytes>
};

<16 bytes>
struct Case1 {
	float3 a;
	float b;
};

<32 bytes>
struct Case2 {
	float2 a;
	<padding 8 bytes>
	float3 b;
	<padding 4 bytes>
};

<16 bytes>
struct Case3 {
	half a;
	<padding 2 bytes>
	double b;
	float c;
};
*/

class AfterglowStructLayout {
public: 
	enum class AttributeType {
		Undefined, 
		Half2, // 4 bytes is minimum attribute size in HLSL Struct.
		Half4, 
		Float, 
		Float2,
		Float3,
		Float4,
		Float2x2, 
		Float3x3,
		Float4x4,
		Double, 
		Double2, 
		Double3, 
		Double4, 
		Int, 
		Int2, 
		Int3, 
		Int4, 
		UnsignedInt, 
		UnsignedInt2,
		UnsignedInt3,
		UnsignedInt4,

		EnumCount
	};

	struct AttributeMember {
		AttributeType type;
		std::string name;
	};

	using AttributeMemberCallback = void(const AttributeMember&);
	using AttributeMemberWithOffsetCallback = void(const AttributeMember&, uint32_t);

	template<typename Type>
	static AfterglowStructLayout create() = delete;

	// @brief: create SSBOLayout from vertex type.
	template<vert::VertexType Type>
	static AfterglowStructLayout create();

	// @brief: create SSBOLayout from Inreflect type
	template<inreflect::InreflectType Type, typename BaseType = float>
 	static AfterglowStructLayout create();

	static uint32_t attributeByteSize(AttributeType type);
	static uint32_t numAttributeComponents(AttributeType type);
	static const std::string& hlslTypeName(AttributeType type);

	uint32_t numAttributes() const;

	uint32_t attributeByteSize(const std::string& name) const;
	uint32_t byteSize() const;
	uint32_t offset(const std::string& name, util::OptionalRef<std::function<AttributeMemberWithOffsetCallback>> callback = std::nullopt) const;
	
	void addAttribute(AttributeType type, const std::string& name);
	void removeAttribute(const std::string& name);
	// @return: Success to remove attribute.
	bool removeAttribute(uint32_t index);

	// template<std::invocable FuncType>
	void forEachAttributeMember(const std::function<AttributeMemberCallback>& callback) const;
	void forEachAttributeMemberWithOffset(const std::function<AttributeMemberWithOffsetCallback>& callback) const;

	// Adding paddings automatically.
	std::vector<AttributeMember> generateHLSLStructMembers() const;

private:
	template<vert::VertexType Type, vert::AttributeType CurrentAttributeType>
	static void createFromVertexImplement(AfterglowStructLayout& layout);

	template<typename Type> constexpr AttributeType attributeType() = delete;
	template<> constexpr AttributeType attributeType<float>() { return AttributeType::Float; };
	template<> constexpr AttributeType attributeType<double>() { return AttributeType::Double; };

	inline void appendPaddingAttributes(std::vector<AttributeMember>& dest, int32_t paddingSize) const;

	std::vector<AttributeMember> _attributes;

	struct AttributeInfo {
		std::string hlslTypeName;
		uint32_t size;
		uint32_t numComponents;
	};

	static inline uint32_t _structAlignment = 16;

	// HLSL Standard memory alignment.
	static inline const std::unordered_map<AttributeType, AttributeInfo> _attributeTypeInfos = {
		{AttributeType::Undefined, {"undefined", 0, 0}}, 
		{AttributeType::Half2, {"half2", 4, 2}},
		{AttributeType::Half4, {"half4", 8, 4}},
		{AttributeType::Float, {"float", 4, 1}},
		{AttributeType::Float2, {"float2", 8, 2}},
		{AttributeType::Float3, {"float3", 12, 3}},
		{AttributeType::Float4, {"float4", 16, 4}},
		{AttributeType::Float2x2, {"float2x2", 16, 4}},
		{AttributeType::Float3x3, {"float3x3", 36, 9}},
		{AttributeType::Float4x4, {"float4x4", 64, 16}},
		{AttributeType::Double, {"double", 8, 1}},
		{AttributeType::Double2, {"double2", 16, 2}},
		{AttributeType::Double3, {"double3", 24, 3}},
		{AttributeType::Double4, {"double4", 32, 4}},
		{AttributeType::Int, {"int", 4, 1}},
		{AttributeType::Int2, {"int2", 8, 2}},
		{AttributeType::Int3, {"int3", 12, 3}},
		{AttributeType::Int4, {"int4", 16, 4}},
		{AttributeType::UnsignedInt, {"uint", 4, 1}},
		{AttributeType::UnsignedInt2, {"uint2", 8, 2}},
		{AttributeType::UnsignedInt3, {"uint3", 12, 3}},
		{AttributeType::UnsignedInt4, {"uint4", 16, 4}}
	};
};


template<vert::VertexType Type>
static AfterglowStructLayout AfterglowStructLayout::create() {
	AfterglowStructLayout layout;
	createFromVertexImplement<Type, typename Type::First>(layout);
	return layout;
}

template<inreflect::InreflectType Type, typename BaseType>
inline static AfterglowStructLayout AfterglowStructLayout::create() {
	AfterglowStructLayout layout;
	Type::forEachAttribute([&](auto typeInfo){
		// Support float types only (e.g. glm::vec).
		static_assert(
			typeInfo.size % sizeof(BaseType) == 0,
			"Unsupported member from Inreflect."
			);
		constexpr uint32_t multiples = typeInfo.size / sizeof(BaseType);
		constexpr AttributeType type =
			static_cast<AttributeType>(util::EnumValue(attributeType<BaseType>()) + multiples - 1);
		layout.addAttribute(type, typeInfo.name);
	});
	return layout;
}

template<vert::VertexType Type, vert::AttributeType CurrentAttributeType>
inline void AfterglowStructLayout::createFromVertexImplement(AfterglowStructLayout& layout) {
	layout.addAttribute(attributeType<typename CurrentAttributeType::Component>(), CurrentAttributeType::name);
	if constexpr (!std::is_same_v<Type::template Next<CurrentAttributeType>, typename Type::Empty>) {
		createFromVertexImplement<Type, Type::template Next<CurrentAttributeType>>(layout);
	}
}