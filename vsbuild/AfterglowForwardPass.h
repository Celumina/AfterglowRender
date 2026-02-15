#pragma once
#include "AfterglowPassInterface.h"
class AfterglowForwardPass : public AfterglowPassInterface {
public: 
	AfterglowForwardPass(
		AfterglowDevice& device, 
		AfterglowPassInterface* prevPass = nullptr
	);

	std::string_view passName() const override { return inreflect::EnumName(render::Domain::Forward); }
}; 

