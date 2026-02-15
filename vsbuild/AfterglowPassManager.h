#pragma once
#include "AfterglowPassInterface.h"
#include "AfterglowPassSetBase.h"
#include "AfterglowUtilities.h"

class AfterglowPassManager : public AfterglowObject {
public:
	constexpr static inline uint32_t domainCount = util::EnumValue(render::Domain::EnumCount);
	using FixedPasses = std::array<std::unique_ptr<AfterglowPassInterface>, domainCount>;
	using PerDomainCustomPassSets = std::vector<std::unique_ptr<AfterglowPassSetBase>>;
	using CustomPassSets = std::array<std::unique_ptr<PerDomainCustomPassSets>, domainCount>;

	AfterglowPassManager(AfterglowDevice& device);

	inline AfterglowDevice& device() noexcept { return _device; }

	template<render::PassSetType Type, typename ...Params>
	Type& installCustomPassSet(Params&& ...constructParams);

	// @param callbak: void(*)(AfterglowPassInterface&, ParamTypes...)
	template<typename CallbackType, typename ...ParamTypes>
	void forEachPass(CallbackType&& callback, ParamTypes&& ...params);

	inline PerDomainCustomPassSets* domainCustomPassSets(render::Domain domain) { return _customPassSets[util::EnumValue(domain)].get(); }

	// @note: invoke after installCustomPassSets.
	//void concretePasses(AfterglowDevice& device);

	// @brief: Find render pass from fixed passes.
	AfterglowPassInterface* findPass(render::Domain domain);
	// @brief: Find render pass from fixed passes and custom passes 
	AfterglowPassInterface* findPass(const std::string& name, render::Domain beginDomain = render::Domain::Undefined);
	// @brief: Find render pass from specified PerDomainCustomPassSets 
	AfterglowPassInterface* findPass(const std::string& name, PerDomainCustomPassSets& domainCustomPassSets);

	FixedPasses& fixedPasses() noexcept { return _fixedPasses; }
	inline bool isFinalPass(AfterglowPassInterface& pass) const noexcept { return &pass == _finalPass; }

private:
	AfterglowDevice& _device;
	FixedPasses _fixedPasses;
	CustomPassSets _customPassSets;

	AfterglowPassInterface* _finalPass = nullptr;
};

template<render::PassSetType Type, typename ...Params>
inline Type& AfterglowPassManager::installCustomPassSet(Params&& ...constructParams) {
	if (!_customPassSets[util::EnumValue(Type::beginDomain)]) {
		_customPassSets[util::EnumValue(Type::beginDomain)] = std::make_unique<PerDomainCustomPassSets>();
	}
	auto& passSet = _customPassSets[util::EnumValue(Type::beginDomain)]->emplace_back(
		std::make_unique<Type>(std::forward<Params>(constructParams)...)
	);
	return reinterpret_cast<Type&>(*passSet);
}

template<typename CallbackType, typename ...ParamTypes>
inline void AfterglowPassManager::forEachPass(CallbackType&& callback, ParamTypes && ...params) {
	for (uint32_t index = 0; index < domainCount; ++index) {
		auto& fixedPass = _fixedPasses[index];
		if (fixedPass) {
			callback(*fixedPass, params...);
		}
		auto& domainCustomPassSets = _customPassSets[index];
		if (!domainCustomPassSets) {
			continue;
		}
		for (auto& customPassSet : *domainCustomPassSets) {
			if (!customPassSet) {
				continue;
			}
			auto& passes = customPassSet->passes();
			for (auto& pass : passes) {
				if (!pass) {
					continue;
				}
				callback(*pass, params...);
			}
		}
	}
}
