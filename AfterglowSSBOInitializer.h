#pragma once

#include <memory>

#include "AfterglowSSBOInfo.h"
#include "AfterglowStructuredData.h"
#include "glm/glm.hpp"

class AfterglowSSBOInitializer {
public:
	AfterglowSSBOInitializer(const AfterglowSSBOInfo& ssboInfo);

	const char* data() const;
	uint64_t byteSize() const;

private:
	std::unique_ptr<AfterglowStructuredData> _data;

	// Temporary methods
	INR_ENABLE_PRIVATE_REFLECTION(AfterglowSSBOInitializer);
	
	// TODO: For indexed ssbo meshes, Inplemented in ComputeShader.
	void initIndexBuffer();

	// @deprecated: init ssbo buffer by another compute shader.
	struct alignas(16) ExampleParticle {
		glm::vec4 position;
		glm::vec4 velocity;
		glm::vec4 color;
	};
	void initExampleParticles();
};


INR_CLASS(AfterglowSSBOInitializer) {
	INR_FUNCS (
		INR_FUNC(initExampleParticles)
	);
};