#pragma once
#include "AfterglowPassInterface.h"
class AfterglowBloomPass : public AfterglowPassInterface {
public: 
	AfterglowBloomPass(AfterglowPassInterface& srcPass, float scale, uint32_t sequenceID);

	inline static const std::string& horizontalBlurSubpassName() noexcept { return _horizontalBlurSubpassName; }
	inline static const std::string& verticalBlurCombinationSubpassName() noexcept { return _verticalBlurCombinationSubpassName; }

	constexpr static std::string_view combinedTextureName() noexcept { return "bloomCombinedTexture"; }

	std::string_view passName() const override { return _passName; }

private:
	constexpr static std::string_view horizontalBlurTextureName() noexcept { return "bloomHorizontalBlurTexture"; }

	std::string _passName;

	static inline std::string _horizontalBlurSubpassName = "HorizontalBlur";
	static inline std::string _verticalBlurCombinationSubpassName = "VerticalBlurCombination";
};

