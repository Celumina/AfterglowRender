#pragma once

#include <memory>

#include "Inreflect.h"

class AfterglowRenderer;

enum TestEnum {
	Apple, 
	Banana, 
	Candle, 
	Door, 
	Earth, 
	Flower,
	Gold
};

INR_CLASS(TestEnum) {
	INR_ATTRS(
		INR_ENUM(Apple), 
		INR_ENUM(Banana),
		INR_ENUM(Candle),
		INR_ENUM(Door),
		INR_ENUM(Earth),
		INR_ENUM(Flower),
		INR_ENUM(Gold)
	);
};

class AfterglowRenderStatus {
public:
	AfterglowRenderStatus(AfterglowRenderer& renderer);
	~AfterglowRenderStatus();

	float fps() const noexcept;
	float maximumFPS() const noexcept;

	void setMaximumFPS(float fps) noexcept;

	const char* deviceName() noexcept;

	inline void INVALID_PARAM_TEST(uint8_t u8) {}
	inline void POINTER_PARAM_TEST(double* d) { printf("%f\n", *d); }
	inline void ENUM_PARAM_TEST(TestEnum e) { printf("%d\n", e); }

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};


INR_CLASS(AfterglowRenderStatus) {
	INR_FUNCS (
		INR_FUNC(fps), 
		INR_FUNC(maximumFPS),
		INR_FUNC(setMaximumFPS), 
		INR_FUNC(deviceName), 
		INR_FUNC(INVALID_PARAM_TEST), 
		INR_FUNC(POINTER_PARAM_TEST), 
		INR_FUNC(ENUM_PARAM_TEST)
	);
};
