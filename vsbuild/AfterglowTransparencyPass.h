#pragma once
#include "AfterglowPassInterface.h"
// TODO: Order Independent Transparency support.
class AfterglowTransparencyPass : public AfterglowPassInterface {
public:
	AfterglowTransparencyPass(
		AfterglowDevice& device,
		AfterglowPassInterface* prevPass = nullptr
	);

	std::string_view passName() const override { return inreflect::EnumName(render::Domain::Transparency); }
};

