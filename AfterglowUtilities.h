#pragma once
#include <cstdint>
#include <type_traits>
#include <optional>
#include <string>
#include <typeindex>

namespace constant {
	static constexpr float pi_float = 3.1415927f;
	static constexpr double pi_double = 3.14159265358979323846;

	static constexpr float pi = pi_float;
	static constexpr float inv_pi = 1.0f / pi;
}

namespace util {
	template<typename Type>
	concept Trivial = std::is_trivial_v<Type>;

	template<typename Type>
	using OptionalRef = const std::optional<std::reference_wrapper<const Type>>&;

	template<typename Type>
	using MutableOptionalRef = const std::optional<std::reference_wrapper<Type>>&;

	constexpr uint32_t MakeVersion(uint32_t major, uint32_t minor, uint32_t patch);
	constexpr uint32_t MakeApiVersion(uint32_t variant, uint32_t major, uint32_t minor, uint32_t patch);
	
	std::wstring ToWstring(const std::string& str);
	std::string ToString(const std::wstring& wstr);
	std::string UpperCase(const std::string& str);

	size_t Align(size_t value, size_t alignment);

	template<typename Type>
	inline constexpr std::type_index TypeIndex();

	template<typename EnumType>
	inline constexpr std::underlying_type_t<EnumType> EnumValue(EnumType e);

	template<typename Type, typename WeightType>
	Type inline constexpr Lerp(Type a, Type b, WeightType t);
}

constexpr uint32_t util::MakeVersion(uint32_t major, uint32_t minor, uint32_t patch) {
	return (major << 22U) | (minor << 12U) | (patch);
}

constexpr uint32_t util::MakeApiVersion(uint32_t variant, uint32_t major, uint32_t minor, uint32_t patch) {
	return  (variant << 29U) | (major << 22U) | (minor << 12U) | (patch);
}

// @return: radians
constexpr long double operator"" _deg(unsigned long long degrees) {
	return static_cast<long double>(degrees) * constant::pi_double / 180.0;
};

// @return: radians
constexpr long double operator"" _deg(long double degrees) {
	return degrees * constant::pi_double / 180.0f;
};

template<typename Type>
inline constexpr std::type_index util::TypeIndex() {
	return std::type_index(typeid(Type));
}

template<typename EnumType>
inline constexpr std::underlying_type_t<EnumType> util::EnumValue(EnumType e) {
	return static_cast<std::underlying_type_t<EnumType>>(e);
}

template<typename Type, typename WeightType>
constexpr Type util::Lerp(Type a, Type b, WeightType t) {
	return a * (static_cast<WeightType>(1) - t) + b * t;
}
