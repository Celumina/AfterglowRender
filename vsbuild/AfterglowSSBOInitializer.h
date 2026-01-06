#pragma once

#include <memory>
#include "glm/glm.hpp"

#include "Inreflect.h"
#include "ACESUtilities.h"

class AfterglowStructuredData;
class AfterglowSSBOInfo;

class AfterglowSSBOInitializer {
public:
	AfterglowSSBOInitializer(const AfterglowSSBOInfo& ssboInfo);
	~AfterglowSSBOInitializer();

	const char* data() const;
	uint64_t byteSize() const;

private:
	std::unique_ptr<AfterglowStructuredData> _data;

	// Temporary methods
	INR_ENABLE_PRIVATE_REFLECTION(AfterglowSSBOInitializer);

	// @deprecated: init ssbo buffer by another compute shader.
	struct alignas(16) ExampleParticle {
		glm::vec4 position;
		glm::vec4 velocity;
		glm::vec4 color;
	};
	void initExampleParticles();

	void initACESOutputTransformParams();
	void initACESReachMTable();
	void initACESUpperHullGammaTable();
	void initACESGamutCuspsTable();
};


INR_CLASS(AfterglowSSBOInitializer) {
	INR_FUNCS (
		INR_FUNC(initExampleParticles), 
		INR_FUNC(initACESOutputTransformParams),
		INR_FUNC(initACESReachMTable), 
		INR_FUNC(initACESUpperHullGammaTable),
		INR_FUNC(initACESGamutCuspsTable)
	);
};