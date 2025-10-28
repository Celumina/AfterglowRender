#pragma once
#include <string>
#include <typeindex>
#include <array>
#include <type_traits>
#include <concepts>
#include <algorithm>

#include "TemplateString.h"
#include "FunctionTraits.h"

// TODO: Reflect auto sort by offset?
// TODO: Check integrity of attribute in Compile-time?

#define INR_CLASS(Type) \
template<> \
struct Inreflect<Type> : public InreflectDefinition<Type>

// @deprecated
//#define INR_TEMPLATE_CLASS(Type, ...) \
//template<template <typename...> typename Type __VA_OPT__(, typename) __VA_ARGS__> \
//struct Inreflect<Type<__VA_ARGS__>> : public InreflectDefinition<Type<__VA_ARGS__>>

// InreflectDerivedType: A keyword for OVERLOADED_FUNC and BASE_CLASSES in INR_CRTP_CLASS.
#define INR_CRTP_CLASS(Type, DerivedType, ...) \
template<typename InreflectDerivedType> \
concept DeriviedFrom##Type = std::is_base_of_v<Type<InreflectDerivedType>, InreflectDerivedType>; \
template<DeriviedFrom##Type InreflectDerivedType __VA_OPT__(, typename) __VA_ARGS__> \
struct Inreflect<Type<InreflectDerivedType __VA_OPT__(, ) __VA_ARGS__>> : public InreflectDefinition<Type<InreflectDerivedType __VA_OPT__(, ) __VA_ARGS__>>

// @warning: Make sure it has same order with class declaration.
#define INR_BASE_CLASSES using BaseClasses = std::tuple

#define INR_ATTRS static constexpr auto attributes = std::make_tuple

#define INR_ATTR(attribute) \
	InreflectMemberAttribute<decltype(Inreflect::InreflectDefinition::Class::attribute), TEMPLATE_STR(#attribute), offsetof(Inreflect::InreflectDefinition::Class,attribute)> {}

#define INR_STATIC_ATTR(attribute) \
	InreflectStaticAttribute<decltype(Inreflect::InreflectDefinition::Class::attribute), TEMPLATE_STR(#attribute), &Inreflect::InreflectDefinition::Class::attribute> {}

#define INR_FUNCS static constexpr auto functions = std::make_tuple

#define INR_FUNC(func) \
	InreflectMemberFunction<decltype(&Inreflect::InreflectDefinition::Class::func), TEMPLATE_STR(#func), &Inreflect::InreflectDefinition::Class::func> {}

#define INR_OVERLOADED_FUNC(FuncType, func) \
	InreflectMemberFunction<FuncType, TEMPLATE_STR(#func), static_cast<FuncType>(&Inreflect::InreflectDefinition::Class::func)> {}

#define INR_STATIC_FUNC(func) \
	InreflectStaticFunction<decltype(&Inreflect::InreflectDefinition::Class::func), TEMPLATE_STR(#func), &Inreflect::InreflectDefinition::Class::func> {}

#define INR_OVERLOADED_STATIC_FUNC(FuncType, func) \
	InreflectStaticFunction<FuncType, TEMPLATE_STR(#func), static_cast<FuncType>(&Inreflect::InreflectDefinition::Class::func)> {}

#define INR_ENUM(enumeration) \
InreflectEnumAttribute<decltype(Inreflect::InreflectDefinition::Class::enumeration), TEMPLATE_STR(#enumeration), Inreflect::InreflectDefinition::Class::enumeration> {}


// If you want to enable private member reflection, Define it in the target class.
#define INR_ENABLE_PRIVATE_REFLECTION(Type) \
friend struct Inreflect<Type>


template<typename Type>
struct Inreflect;

namespace inreflect {
	template <typename Type>
	concept BaseClassesDefined = requires { typename Type::BaseClasses; };

	template <typename Type>
	concept AttributesDefined = requires { Type::attributes; };

	template <typename Type>
	concept FunctionsDefined = requires { Type::functions; };

	template <typename Type>
	concept ParentDefined = requires { typename Type::parent; };

	template <typename Type>
	struct InreflectTrait {
		constexpr static bool value = false;
	};

	template <typename Type>
	struct InreflectTrait<Inreflect<Type>> {
		constexpr static bool value = true;
	};

	template <typename Type>
	concept InreflectType = InreflectTrait<Type>::value;

	// Check if Type can be used as a template template parameter
	template <typename Type>
	concept IsTemplateClass = requires {
		[] <template <typename...> class TemplateType>() {
			TemplateType<void> dummy; 
		}.template operator() <Type> ();
	};

	template <typename Type>
	constexpr std::string_view EnumName(Type e) { return Inreflect<Type>::enumName(e); };

	template<typename Type>
	concept TupleType = requires {
		typename std::tuple_size<std::decay_t<Type>>;
		typename std::tuple_element<0, std::decay_t<Type>>;
	};

	// DecayTuple
	template <typename TupleType>
	struct DecayedTuple;

	template <typename... Types>
	struct DecayedTuple<std::tuple<Types...>> {
		using Tuple = std::tuple<std::decay_t<Types>...>;
	};

	template<TupleType Type, size_t... indices>
	constexpr bool IsDefaultConstructibleTupleImpl(std::index_sequence<indices...>) {
		return (... && std::is_default_constructible_v<std::tuple_element_t<indices, Type>>);
	}

	// Main check: Returns true if Tuple is default-constructible
	template<TupleType Type>
	constexpr bool IsDefaultConstructibleTuple() {
		constexpr size_t tupleSize = std::tuple_size_v<Type>;
		return IsDefaultConstructibleTupleImpl<Type>(std::make_index_sequence<tupleSize>{});
	}
}

template<typename MemberType, typename NameType, bool isStaticMember>
struct InreflectMemberBase {
	using Member = MemberType;
	using Name = NameType;
	
	static constexpr std::string_view name = NameType::view();
	static constexpr uint32_t size = sizeof(Member);
	static constexpr bool isStatic = isStaticMember;

	template<typename Type>
	static constexpr bool isType() { return std::is_same_v<Type, Member>; }

	template<typename NameType>
	static constexpr bool isNamed() { return std::is_same_v<NameType, Name>; };
};


template<typename AttributeType, typename NameType, size_t byteOffset>
struct InreflectMemberAttribute : public InreflectMemberBase<AttributeType, typename NameType, false> {
	using Attribute = AttributeType;
	static constexpr size_t offset = byteOffset;

	// For non-static member
	// TODO: Handle virtual.
	// static_assert(std::is_standard_layout_v<ClassType>, "[Inreflect] Class must be standard layout.");

	template<typename ClassType>
	static constexpr Attribute& value(ClassType& instance) {
		return *(reinterpret_cast<AttributeType*>(reinterpret_cast<char*>(&instance) + byteOffset));
	}

	template<typename ClassType>
	static constexpr const Attribute& value(const ClassType& instance) {
		return *(reinterpret_cast<const AttributeType*>(reinterpret_cast<const char*>(&instance) + byteOffset));
	}
};


template<typename AttributeType, typename NameType, AttributeType* address>
struct InreflectStaticAttribute : public InreflectMemberBase<AttributeType, typename NameType, true> {
	using Attribute = AttributeType;

	// For static member
	static constexpr Attribute& value = *address;
};


template<typename EnumType, typename NameType, EnumType enumValue>
struct InreflectEnumAttribute : public InreflectMemberBase<EnumType, typename NameType, true> {
	using Underlying = std::underlying_type_t<EnumType>;

	static constexpr Underlying value = static_cast<Underlying>(enumValue); 
	static constexpr EnumType raw = enumValue;
};


template<typename FunctionType, typename NameType, FunctionType functionAddress, bool isStaticFunction>
struct InreflectFunctionBase : public InreflectMemberBase<FunctionType, NameType, isStaticFunction> {
	using Function = FunctionType;
	using Return = FunctionTraits<Function>::Return;
	using ParamTuple = FunctionTraits<Function>::ParamTuple;
	static constexpr Function address = functionAddress;

	template<typename ...ParamTypes>
	static constexpr bool sameParamTypes() { return std::is_same_v<std::tuple<ParamTypes...>, ParamTuple>; }

	template<typename ReturnType>
	static constexpr bool sameReturnType() { return std::is_same_v<ReturnType, Return>; }

	// @param callback: [...]<typename ParamType, uint32_t ParamIndex>(){...}
	template<typename CallbackType>
	static constexpr void forEachParam(const CallbackType& callback) {
		// #0: Construct a lambda to receive a parameter pack (Unpack the tuple to parameter pack)
		// #1: Call this lambda immediately parameter from a nullptr (without any object is constructed)
		// #2: Inside the lamda, a folder expression (..., func<Types>()) to call every instantiated template func.
		// #3: callback.template operator()<Types>()  : 
		//     If callback is a generic lambda ([](auto x) { ... }), it has a templated operator(), so we invoke it with Ts... types.
		//     If callback is a functor with a template method, we use .template to disambiguate the operator() template.
		[&] <typename... Types, std::size_t... Indices>(std::tuple<Types...>*, std::index_sequence<Indices...>) {
			(..., callback.template operator() <Types, Indices> ());
		}((ParamTuple*)nullptr, std::make_index_sequence<std::tuple_size_v<ParamTuple>>());
	};
};


template<typename FunctionType, typename NameType, FunctionType functionAddress>
struct InreflectMemberFunction : public InreflectFunctionBase<FunctionType, NameType, functionAddress, false> {
	using Return = FunctionTraits<FunctionType>::Return;
	using ParamTuple = FunctionTraits<FunctionType>::ParamTuple;
	using DecayedParamTuple = typename inreflect::DecayedTuple<ParamTuple>::Tuple;
	
	template<typename ClassType, typename ...ParamTypes>
	static constexpr Return call(ClassType& instance, ParamTypes&& ...params) {
		return (instance.*functionAddress)(params...);
	}
	
	template<typename ClassType, typename ...ParamTypes>
	static constexpr Return call(const ClassType& instance, ParamTypes&& ...params) {
		return (instance.*functionAddress)(params...);
	}

	template<typename ClassType>
	static constexpr Return call(ClassType& instance, ParamTuple& params) {
		return std::apply([&instance](auto&&... args) -> Return {
			return (instance.*functionAddress)(std::forward<decltype(args)>(args)...);
		}, params);
	}

	template<typename ClassType>
	static constexpr Return call(const ClassType& instance, ParamTuple& params) {
		return std::apply([&instance](auto&&... args) -> Return {
			return (instance.*functionAddress)(std::forward<decltype(args)>(args)...);
		}, params);
	}

	template<typename ClassType>
		requires (!std::is_same_v<ParamTuple, DecayedParamTuple>)
	static constexpr Return call(ClassType& instance, DecayedParamTuple& params) {
		return std::apply([&instance](auto&&... args) -> Return {
			return (instance.*functionAddress)(std::forward<decltype(args)>(args)...);
		}, params);
	}

	template<typename ClassType>
		requires (!std::is_same_v<ParamTuple, DecayedParamTuple>)
	static constexpr Return call(const ClassType& instance, DecayedParamTuple& params) {
		return std::apply([&instance](auto&&... args) -> Return {
			return (instance.*functionAddress)(std::forward<decltype(args)>(args)...);
		}, params);
	}
};


template<typename FunctionType, typename NameType, FunctionType functionAddress>
struct InreflectStaticFunction : public InreflectFunctionBase<FunctionType, NameType, functionAddress, true> {
	using Return = FunctionTraits<FunctionType>::Return;
	using ParamTuple = FunctionTraits<FunctionType>::ParamTuple;
	using DecayedParamTuple = typename inreflect::DecayedTuple<ParamTuple>::Tuple;

	template<typename ...ParamTypes>
	static constexpr Return call(ParamTypes&& ...params) {
		return functionAddress(params...);
	}

	static constexpr Return call(ParamTuple& params) {
		return std::apply([](auto&&... args) -> Return {
			return functionAddress(std::forward<decltype(args)>(args)...);
		}, params);
	}

	// Force it to be a template function for requires
	template<typename Type = void>
		requires (!std::is_same_v<ParamTuple, DecayedParamTuple>)
	static constexpr Return call(DecayedParamTuple& params) {
		return std::apply([](auto&&... args) -> Return {
			return functionAddress(std::forward<decltype(args)>(args)...);
		}, params);
	}
};

template<typename Type>
struct InreflectDefinition {
	using Class = Type;

	// If not found, return nullptr.
	// Find Non-Static attribute
	template<typename AttributeType>
	static constexpr AttributeType* attribute(Class& instance, std::string_view name);

	template<typename AttributeType>
	static constexpr const AttributeType* attribute(const Class& instance, std::string_view name);

	// Find Static attribute
	template<typename AttributeType>
	static constexpr AttributeType* attribute(std::string_view name);

	// Compile-time static attribute.
	template<typename AttributeType, typename NameType>
	static constexpr AttributeType* attribute();

	// Find Function
	template<typename FuncType>
	static constexpr FuncType function(std::string_view name);
	
	// Compile-time function.
	template<typename FuncType, typename NameType>
	static constexpr FuncType function();

	// For enum class
	static constexpr std::string_view enumName(Class value);
	// Returns attribute count of registered attribute, base classes' attribute were not included.
	static constexpr uint32_t enumCount();

	template<typename CallbackType, typename CurrentType = Type, typename ParentType =  void, int32_t index = 0, int32_t classIndex = -1, size_t byteOffset = 0>
	static constexpr void forEachAttribute(const CallbackType& callback);

	template<typename CallbackType, typename CurrentType = Type, typename ParentType = void, int32_t index = 0, int32_t classIndex = -1>
	static constexpr void forEachFunction(const CallbackType& callback);
};


template<typename Type>
struct Inreflect : public InreflectDefinition<Type> {
	// Deferred Evaluation to check if struct have not been specialized.
	static_assert(
		!std::is_same_v<Type, Type>, "This class has not been registered as an inreflect class."
	);
	using BaseClasses = std::tuple<>;
	static constexpr auto attributes = std::tuple<>{};
	static constexpr auto functions = std::tuple<>{};
};


template<typename Type>
template<typename AttributeType>
inline constexpr AttributeType* InreflectDefinition<Type>::attribute(Class& instance, std::string_view name) {
	AttributeType* attribute = nullptr;
	forEachAttribute([&](auto typeInfo){
		if constexpr (!typeInfo.isStatic && typeInfo.isType<AttributeType>()) {
			if (typeInfo.name == name) {
				attribute = &typeInfo.value(instance);
			}
		}
	});
	return attribute;
}

template<typename Type>
template<typename AttributeType>
inline constexpr const AttributeType* InreflectDefinition<Type>::attribute(const Class& instance, std::string_view name) {
	return attribute<AttributeType>(const_cast<Class&>(instance), name);
}

template<typename Type>
template<typename AttributeType>
inline constexpr AttributeType* InreflectDefinition<Type>::attribute(std::string_view name) {
	AttributeType* attribute = nullptr;
	forEachAttribute([&](auto typeInfo) {
		if constexpr (typeInfo.isStatic && typeInfo.isType<AttributeType>()) {
			if (typeInfo.name == name) {
				attribute = &typeInfo.value;
			}
		}
	});
	return attribute;
}

template<typename Type>
template<typename AttributeType, typename NameType>
inline constexpr AttributeType* InreflectDefinition<Type>::attribute() {
	// TODO: Full compile-time method.
	AttributeType* attribute = nullptr;
	forEachAttribute([&](auto typeInfo) {
		if constexpr (typeInfo.isStatic && typeInfo.isType<AttributeType>() && typeInfo.isNamed<NameType>()) {
			attribute = &typeInfo.value;
		}
	});
	return attribute;
}

template<typename Type>
template<typename FuncType>
inline constexpr FuncType InreflectDefinition<Type>::function(std::string_view name) {
	FuncType func = nullptr;
	forEachFunction([&](auto typeInfo) {
		if constexpr (typeInfo.isType<FuncType>()) {
			if (typeInfo.name == name) {
				func = typeInfo.address;
			}
		}
	});
	return func;
}

template<typename Type>
template<typename FuncType, typename NameType>
inline constexpr FuncType InreflectDefinition<Type>::function() {
	// TODO: Full compile-time method.
	FuncType func = nullptr;
	forEachFunction([&](auto typeInfo) {
		if constexpr (typeInfo.isType<FuncType>() && typeInfo.isNamed<NameType>()) {
			func = typeInfo.address;
		}
	});
	return func;
}

template<typename Type>
inline constexpr std::string_view InreflectDefinition<Type>::enumName(Class value) {
	std::string_view name;
	forEachAttribute([&](auto enumInfo) {
		static_assert(
			std::is_same_v<typename decltype(enumInfo)::Member, Class>,
			"[Inreflect] This class is not a enum class, can not aquire its enumName."
		);
		if (enumInfo.raw == value) {
			name = enumInfo.name;
		}
	});
	return name;
}

template<typename Type>
inline constexpr uint32_t InreflectDefinition<Type>::enumCount() {
	return std::tuple_size_v<decltype(Inreflect<Class>::attributes)>;
}

template<typename Type>
template<typename CallbackType, typename CurrentType, typename ParentType, int32_t index, int32_t classIndex, size_t byteOffset >
constexpr inline void InreflectDefinition<Type>::forEachAttribute(const CallbackType& callback) {
	if constexpr (inreflect::AttributesDefined<Inreflect<CurrentType>>) {
		constexpr auto attributes = Inreflect<CurrentType>::attributes;
		using Attributes = decltype(attributes);

		if constexpr (index < std::tuple_size_v<Attributes>) {
			using SourceAttribute = std::tuple_element_t<index, Attributes>;
			if constexpr (SourceAttribute::isStatic) {
				constexpr SourceAttribute attribute{};
				callback(attribute);
			}
			else {
				constexpr InreflectMemberAttribute<typename SourceAttribute::Attribute, typename SourceAttribute::Name, SourceAttribute::offset + byteOffset> attribute{};
				callback(attribute);
			}
			forEachAttribute<CallbackType, CurrentType, ParentType, index + 1, classIndex, byteOffset>(callback);
			return;
		}
	}
	// BFS
	if constexpr (!std::is_same_v<ParentType, void>) {
		// Neighbour
		using Classes = Inreflect<ParentType>::BaseClasses;
		constexpr size_t numClasses = std::tuple_size_v<Classes>;
		if constexpr (classIndex + 1 < numClasses) {
			using NextClass = std::tuple_element_t<classIndex + 1, Classes>;
			constexpr size_t classByteSize = classIndex >= 0 ? byteOffset + std::max(sizeof(std::tuple_element_t<classIndex, Classes>), alignof(NextClass)) : byteOffset;
			forEachAttribute<CallbackType, NextClass, ParentType, 0, classIndex + 1, byteOffset + classByteSize>(callback);
		}
	}
	if constexpr (inreflect::BaseClassesDefined<Inreflect<CurrentType>>) {
		using BaseClasses = Inreflect<CurrentType>::BaseClasses;
		constexpr size_t numBaseClasses = std::tuple_size_v<BaseClasses>;
		// Child
		if constexpr (numBaseClasses > 0) {
			using FirstBaseClass = std::tuple_element_t<0, BaseClasses>;
			forEachAttribute<CallbackType, FirstBaseClass, CurrentType, 0, 0, byteOffset>(callback);
		}
	}
}

template<typename Type>
template<typename CallbackType, typename CurrentType, typename ParentType, int32_t index, int32_t classIndex>
inline constexpr void InreflectDefinition<Type>::forEachFunction(const CallbackType& callback) {
	if constexpr (inreflect::FunctionsDefined<Inreflect<CurrentType>>) {
		constexpr auto functions = Inreflect<CurrentType>::functions;
		using Functions = decltype(functions);
		if constexpr (index < std::tuple_size_v<Functions>) {
			using Function = std::tuple_element_t<index, Functions>;
			constexpr Function function{};
			callback(function);
			forEachFunction<CallbackType, CurrentType, ParentType, index + 1, classIndex>(callback);
			return;
		}
	}
	
	// BFS
	if constexpr (!std::is_same_v<ParentType, void>) {
		// Neighbour
		using Classes = Inreflect<ParentType>::BaseClasses;
		constexpr size_t numClasses = std::tuple_size_v<Classes>;
		if constexpr (classIndex + 1 < numClasses) {
			using NextClass = std::tuple_element_t<classIndex + 1, Classes>;
			forEachFunction<CallbackType, NextClass, ParentType, 0, classIndex + 1>(callback);
		}
	}
	if constexpr (inreflect::BaseClassesDefined<Inreflect<CurrentType>>) {
		using BaseClasses = Inreflect<CurrentType>::BaseClasses;
		constexpr size_t numBaseClasses = std::tuple_size_v<BaseClasses>;
		// Child
		if constexpr (numBaseClasses > 0) {
			using FirstBaseClass = std::tuple_element_t<0, BaseClasses>;
			forEachFunction<CallbackType, FirstBaseClass, CurrentType, 0, 0>(callback);
		}
	}
}