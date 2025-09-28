#pragma once

#include <string_view>

#define TEMPLATE_STR(str) \
	TemplateString<TemplateChars<decltype(std::basic_string_view{str})::value_type, std::basic_string_view{str}.size()>{std::basic_string_view{str}}>

template<typename CharType, size_t dataSize>
struct TemplateChars {
	using Char = CharType;
	static constexpr size_t size = dataSize;
	Char data[size + 1] {};
	constexpr TemplateChars(std::basic_string_view<Char> str);
};

template<TemplateChars chars>
struct TemplateString {
	using Char = typename decltype(chars)::Char;

	template<typename Type>
	static constexpr bool is() { return std::is_same_v<Type, TemplateString>; };

	static constexpr auto size() { return chars.size; };
	static constexpr auto data() {return chars.data; };
	static constexpr std::basic_string_view<Char> view() { return chars.data; };
};


template<typename CharType, size_t dataSize>
inline constexpr TemplateChars<CharType, dataSize>::TemplateChars(std::basic_string_view<Char> str) {
	for (size_t index{ 0 }; index < size; ++index) {
		data[index] = str[index];
	}
}
