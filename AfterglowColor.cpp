#include "AfterglowColor.h"

AfterglowColor::AfterglowColor()  noexcept  : _data(0) {
}

AfterglowColor::AfterglowColor(AfterglowColor&& color)  noexcept  : _data(color._data) {
}

AfterglowColor::AfterglowColor(const AfterglowColor& color) noexcept : _data(color._data) {
}

AfterglowColor::AfterglowColor(uint32_t color) noexcept : _data(color) {

}

void AfterglowColor::operator=(const AfterglowColor& color) noexcept {
	_data = color._data;
}

AfterglowColor AfterglowColor::White() noexcept {
	return AfterglowColor(0xFFFFFFFF);
}

AfterglowColor AfterglowColor::Black() noexcept {
	return AfterglowColor(0x000000FF);
}

AfterglowColor AfterglowColor::MutsumiGreen() noexcept {
	return AfterglowColor(0x779977FF);
}

uint8_t AfterglowColor::r() const noexcept {
	return (_data & Mask::Red) >>24;
}

uint8_t AfterglowColor::g() const noexcept {
	return (_data & Mask::Green) >> 16;
}

uint8_t AfterglowColor::b() const noexcept {
	return (_data & Mask::Blue) >> 8;
}

uint8_t AfterglowColor::a() const noexcept {
	return _data & Mask::Alpha;
}

void AfterglowColor::setR(uint8_t r) noexcept {
	_data = (_data & ~Mask::Red) | (r << 24);
}

void AfterglowColor::setG(uint8_t g) noexcept {
	_data = (_data & ~Mask::Green) | (g << 16);
}

void AfterglowColor::setB(uint8_t b) noexcept {
	_data = (_data & ~Mask::Blue) | (b << 8);
}

void AfterglowColor::setA(uint8_t a) noexcept {
	_data = (_data & ~Mask::Alpha) | a;
}

void AfterglowColor::setRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
	_data = (r << 24) | (g << 16) | (b << 8) | a;
}

void AfterglowColor::setFloatRGBA(float r, float g, float b, float a) noexcept {
	setRGBA(r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f);
}
