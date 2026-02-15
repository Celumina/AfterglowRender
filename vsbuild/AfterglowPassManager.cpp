#include "AfterglowPassManager.h"

#include <stdexcept>
#include "AfterglowForwardPass.h"
#include "AfterglowTransparencyPass.h"
#include "AfterglowPostProcessPass.h"
#include "AfterglowUserInterfacePass.h"

AfterglowPassManager::AfterglowPassManager(AfterglowDevice& device) : _device(device) {
	_fixedPasses[util::EnumValue(render::Domain::Forward)] = std::make_unique<AfterglowForwardPass>(device);
	_fixedPasses[util::EnumValue(render::Domain::Transparency)] = std::make_unique<AfterglowTransparencyPass>(
		device, _fixedPasses[util::EnumValue(render::Domain::Forward)].get()
	);
	_fixedPasses[util::EnumValue(render::Domain::PostProcess)] = std::make_unique<AfterglowPostProcessPass>(
		device, _fixedPasses[util::EnumValue(render::Domain::Transparency)].get()
	);
	// Present pass (OnScreen)
	_fixedPasses[util::EnumValue(render::Domain::UserInterface)] = std::make_unique<AfterglowUserInterfacePass>(
		device, _fixedPasses[util::EnumValue(render::Domain::PostProcess)].get()
	);
	_finalPass = findPass(render::Domain::UserInterface);
}

AfterglowPassInterface* AfterglowPassManager::findPass(render::Domain domain) {
	auto& pass = _fixedPasses[util::EnumValue(domain)];
	if (pass) {
		return pass.get();
	}
	return nullptr;
	// throw std::invalid_argument("This domain have not be implemented.");
}

AfterglowPassInterface* AfterglowPassManager::findPass(const std::string& name, render::Domain beginDomain) {
	// Find fixed passes
	for (uint32_t index = util::EnumValue(beginDomain); index < _fixedPasses.size(); ++index) {
		if (!_fixedPasses[index]) {
			continue;
		}
		if (_fixedPasses[index]->passName() == name) {
			return _fixedPasses[index].get();
		}
	}

	// Find custom passes
	for (uint32_t index = util::EnumValue(beginDomain); index < _customPassSets.size(); ++index) {
		auto& perDomainPassSets = _customPassSets[index];
		if (!perDomainPassSets) {
			continue;
		}
		auto* pass = findPass(name, *perDomainPassSets);
		if (pass) {
			return pass;
		}
	}

	return nullptr;
}

AfterglowPassInterface* AfterglowPassManager::findPass(const std::string& name, PerDomainCustomPassSets& domainCustomPassSets) {
	for (auto& passSet : domainCustomPassSets) {
		if (!passSet) {
			continue;
		}
		for (auto& pass : passSet->passes()) {
			if (!pass) {
				continue;
			}
			if (pass->passName() == name) {
				return pass.get();
			}
		}
	}
	return nullptr;
}
