#include "AfterglowSSBOInitializer.h"
#include "AfterglowStructuredData.h"
#include "AfterglowSSBOInfo.h"

#include <random>
#include <glm/glm.hpp>


AfterglowSSBOInitializer::AfterglowSSBOInitializer(const AfterglowSSBOInfo& ssboInfo) {
	if (ssboInfo.isBuffer()) {
		_data = std::make_unique<AfterglowStructuredData>(ssboInfo.elementLayout(), ssboInfo.numElements());
	}
	else {
		_data = std::make_unique<AfterglowStructuredData>(compute::TexelByteSize(ssboInfo.textureMode()), ssboInfo.numElements());
	}
	
	bool functionFound = false;
	switch (ssboInfo.initMode()) {
	case compute::SSBOInitMode::Zero: 
		break;
	case compute::SSBOInitMode::InternalFunction:
		// TODO: Genereate SSBO Layout here.
		Inreflect<AfterglowSSBOInitializer>::forEachFunction([this, &ssboInfo, &functionFound](auto functionInfo){
			if (functionInfo.name == ssboInfo.initResource()) {
				functionInfo.call(*this);
				functionFound = true;
			}
		});
		if (!functionFound) {
			DEBUG_CLASS_ERROR("InitResource function was not found.");
		}
		break;
	case compute::SSBOInitMode::ComputeShader:
		// Be initialized in renderer.
		break;
	default:
		// TODO: Support more initModes.
		DEBUG_CLASS_ERROR(std::format("Unsupported init mode: {}", util::EnumValue(ssboInfo.initMode())));
		// throw std::runtime_error("[AfterglowSSBOInitializer] Unsupported init mode.");
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

void AfterglowSSBOInitializer::initACESOutputTransformParams() {
	_data->setElement(0, aces::OutputTransformContext().takeSSBO());
}

void AfterglowSSBOInitializer::initACESReachMTable() {
	 _data->fill(*aces::OutputTransformContext().takeReachMTable());
}

void AfterglowSSBOInitializer::initACESUpperHullGammaTable() {
	_data->fill(*aces::OutputTransformContext().takeUpperHullGammaTable());
}

void AfterglowSSBOInitializer::initACESGamutCuspsTable() {
	_data->fill(*aces::OutputTransformContext().takeGamutCuspsTable());
}

uint64_t AfterglowSSBOInitializer::byteSize() const {
	return _data->byteSize();
}