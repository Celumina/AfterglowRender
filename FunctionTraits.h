#pragma once
#include <tuple>

template <typename Type>
struct FunctionTraits;

template <typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(ParamTypes...)> {
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};

template <typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(*)(ParamTypes...)> {
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};

template <typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(* const)(ParamTypes...)> {
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};

template <typename ClassType, typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(ClassType::*)(ParamTypes...)> {
	using Class = ClassType;
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};

template <typename ClassType, typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(ClassType::*)(ParamTypes...) const> {
	using Class = ClassType;
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};

template <typename ClassType, typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(ClassType::* const)(ParamTypes...)> {
	using Class = ClassType;
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};

template <typename ClassType, typename ReturnType, typename... ParamTypes>
struct FunctionTraits<ReturnType(ClassType::* const)(ParamTypes...) const> {
	using Class = ClassType;
	using Return = ReturnType;
	using ParamTuple = std::tuple<ParamTypes...>;
};