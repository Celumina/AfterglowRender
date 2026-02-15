#pragma once
#include "AfterglowPassInterface.h"
class AfterglowDownSamplingPass : public AfterglowPassInterface {
public:
	AfterglowDownSamplingPass(AfterglowPassInterface& srcPass, float scale, uint32_t sequenceID);

	std::string_view downSampledTextureName() const { return "downSampledTexture"; }
	std::string_view passName() const override { return _passName; }

private: 
	std::string _passName;
};

