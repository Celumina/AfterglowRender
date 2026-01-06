#pragma once

#include <tuple>
#include <array>


namespace constraint {
	template <typename Type, template <typename...> typename TemplateType>
	struct IsSpecializationOf : std::false_type {};

	template <template <typename...> typename TemplateType, typename... ArgTypes>
	struct IsSpecializationOf<TemplateType<ArgTypes...>, TemplateType> : std::true_type {};

	template <typename Type>
	concept TupleType = IsSpecializationOf<Type, std::tuple>::value;

	template <typename Type>
	struct IsArrayType : std::false_type {};

	template <typename ElementType, size_t numElements>
	struct IsArrayType<std::array<ElementType, numElements>> : std::true_type {};

	template <typename Type>
	concept ArrayType = IsArrayType<Type>::value;
}
