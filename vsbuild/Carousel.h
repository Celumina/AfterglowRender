#pragma once

#include <array>

template<typename ElementType, size_t Size>
class Carousel {
	static_assert(Size > 1, "[Carousel] Invalid size.");
public:
	void push(const ElementType& element);

	const ElementType* data() const noexcept;
	constexpr size_t size() const noexcept;

private: 
	static constexpr size_t _offsetBegin = Size - 1;

	// Double size from parameter for rolling.
	std::array<ElementType, Size * 2> _data = {};
	size_t _offset = _offsetBegin;
};

template<typename ElementType, size_t Size>
inline void Carousel<ElementType, Size>::push(const ElementType& element) {
	if (_offset > Size) {
		_data[_offset - Size] = _data[_offset];
	}
	++_offset;
	if (_offset >= _data.size()) {
		_offset = _offsetBegin;
	}
	_data[_offset] = element;
}

template<typename ElementType, size_t Size>
inline const ElementType* Carousel<ElementType, Size>::data() const noexcept {
	return &_data[_offset - _offsetBegin];
}

template<typename ElementType, size_t Size>
inline constexpr size_t Carousel<ElementType, Size>::size() const noexcept {
	return Size;
}
