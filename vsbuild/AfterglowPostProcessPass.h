#pragma once
#include "AfterglowPassInterface.h"
class AfterglowPostProcessPass : public AfterglowPassInterface {
public:
	AfterglowPostProcessPass(AfterglowDevice& device, AfterglowPassInterface* prevPass);
	std::string_view passName() const override { return inreflect::EnumName(render::Domain::PostProcess); }
};

