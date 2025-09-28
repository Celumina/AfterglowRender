#pragma once
#include "ShaderDefinitions.h"
#include "ComputeDefinitions.h"
#include "AfterglowStructLayout.h"

struct AfterglowSSBOInfo {
	std::string name;
	shader::Stage stage;
	compute::SSBOUsage usage;
	compute::SSBOAccessMode accessMode;
	compute::SSBOInitMode initMode;
	std::string initResource;
	// Total byte size == elementLayout.byteSize() * numElements;
	AfterglowStructLayout elementLayout;
	uint64_t numElements;
};