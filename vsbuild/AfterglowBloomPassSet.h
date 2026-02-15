#pragma once

#include "AfterglowPassSet.h"

class AfterglowMeshResource;
class AfterglowSystemUtilities;

// TODO: Abstract more functions into super class.
// TODO: apply bloom switch for performance test.
class AfterglowBloomPassSet : public AfterglowPassSet<render::Domain::Transparency> {
public: 
	struct ParamName {
		static inline const std::string resolutionScale = "resolutionScale";
		static inline const std::string resolutionInvScale = "resolutionInvScale";
		static inline const std::string useCombinedTexture = "useCombinedTexture";
		static inline const std::string bloomIntensity = "bloomIntensity";
	};

	AfterglowBloomPassSet(
		const AfterglowSystemUtilities& sysUtils, 
		AfterglowMeshResource& meshResource, 
		render::Domain importDomain = render::Domain::Transparency, 
		render::Domain exportDomain = render::Domain::PostProcess
	);
	~AfterglowBloomPassSet();

	void enable() noexcept;
	void disable() noexcept;

	const std::string* downSamplingMaterialName(uint32_t index) const noexcept;
	const std::string* horizontalBlurMaterialName(uint32_t index) const noexcept;
	const std::string* verticalBlurCombinationMaterialInstanceName(uint32_t index) const noexcept;

	//const std::string& outputSubpassName() const;

	void submitCommands(AfterglowDrawCommandBuffer& drawCommandBuffer) override;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};
