#include "AfterglowSSBOInitializer.h"
#include "AfterglowStructuredData.h"
#include "AfterglowSSBOInfo.h"

#include <random>
#include <glm/glm.hpp>


AfterglowSSBOInitializer::AfterglowSSBOInitializer(const AfterglowSSBOInfo& ssboInfo) {
	switch (ssboInfo.initMode) {
	case compute::SSBOInitMode::Zero: 
		_data = std::make_unique<AfterglowStructuredData>(ssboInfo.elementLayout, ssboInfo.numElements);
		break;
	case compute::SSBOInitMode::InternalFunction:
		_data = std::make_unique<AfterglowStructuredData>(ssboInfo.elementLayout, ssboInfo.numElements);
		Inreflect<AfterglowSSBOInitializer>::forEachFunction([this, &ssboInfo](auto functionInfo){
			if (functionInfo.name == ssboInfo.initResource) {
				functionInfo.call(*this);
			}
		});
		break;
	case compute::SSBOInitMode::ComputeShader:
		// Initialize in renderer.
		_data = std::make_unique<AfterglowStructuredData>(ssboInfo.elementLayout, ssboInfo.numElements);
		break;
	default:
		// TODO: ...
		DEBUG_CLASS_FATAL(std::format("Unsupported init mode: {}", util::EnumValue(ssboInfo.initMode)));
		throw std::runtime_error("[AfterglowSSBOInitializer] Unsupported init mode.");
	}

}

AfterglowSSBOInitializer::~AfterglowSSBOInitializer() {
}

const char* AfterglowSSBOInitializer::data() const {
	return _data->data();
}

void AfterglowSSBOInitializer::initExampleParticles() {
	std::default_random_engine randomEngine(time(nullptr));
	std::uniform_real_distribution<float> randomDistribution(0.0f, 1.0f);
	
	// TODO: Vertex inpute attribute order bad. attribute info offset looks not right
	_data->forEachElement<ExampleParticle>(
		[](ExampleParticle& particle, std::default_random_engine& randEngine, std::uniform_real_distribution<float>& randDistribution){
			float radius = 0.25f * sqrt(randDistribution(randEngine));
			float theta = randDistribution(randEngine) * 2.0 * constant::pi;
			particle.position.x = radius * cos(theta) * 1000.0f;
			particle.position.y = radius * sin(theta) * 1000.0f;
			particle.position.z = radius * 1000.0f;

			particle.color = glm::vec4(
				randDistribution(randEngine),
				randDistribution(randEngine),
				randDistribution(randEngine),
				1.0f
			);

			particle.velocity = glm::normalize(particle.position) * 50.0f + (particle.color - 0.5f) * 50.0f;
		}, 
		randomEngine, randomDistribution
	);

}

uint64_t AfterglowSSBOInitializer::byteSize() const {
	return _data->byteSize();
}