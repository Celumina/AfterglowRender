#pragma once
#include "AfterglowPassInterface.h"

// Present Pass
class AfterglowUserInterfacePass : public AfterglowPassInterface {
public: 
	AfterglowUserInterfacePass(AfterglowDevice& device, AfterglowPassInterface* prevPass = nullptr);

	std::string_view passName() const override { return inreflect::EnumName(render::Domain::UserInterface); }
};
