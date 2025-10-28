#pragma once
#include <stdint.h>

class AfterglowColor {
public:
	struct Mask {
		enum : uint32_t {
			Red =   0xFF000000, 
			Green = 0x00FF0000, 
			Blue =  0x0000FF00, 
			Alpha = 0x000000FF
		};
	};

	AfterglowColor() noexcept;
	AfterglowColor(AfterglowColor&& color) noexcept;
	AfterglowColor(const AfterglowColor& color) noexcept;
	AfterglowColor(uint32_t color)  noexcept;

	void operator=(const AfterglowColor& color) noexcept;

	static AfterglowColor White() noexcept;
	static AfterglowColor Black() noexcept;
	static AfterglowColor MutsumiGreen() noexcept;

	uint8_t r() const  noexcept;
	uint8_t g() const  noexcept;
	uint8_t b() const  noexcept;
	uint8_t a() const noexcept;

	void setR(uint8_t r)  noexcept;
	void setG(uint8_t g)  noexcept;
	void setB(uint8_t b)  noexcept;
	void setA(uint8_t a)  noexcept;

	void setRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;

	// @brief: Range from 0 to 1.
	void setFloatRGBA(float r, float g, float b, float a) noexcept;

private:
	uint32_t _data;
};

