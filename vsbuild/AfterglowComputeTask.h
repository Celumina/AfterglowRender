#pragma once
#include <vector>
#include <map>

#include "AfterglowSSBOInfo.h"
#include "Configurations.h"

// TODO: Inrefect Attributes For Serialize Support
// TODO: Serialize template, support for class attribute, like Serialize<std::string>::read(); Serialize<std::string>::write();


// Resource and configuarations desciptions relative to compute shader.
// TODO: Priority for external ref.
class AfterglowComputeTask {
public:
	using SSBOInfos = std::vector<AfterglowSSBOInfo>;
	using SSBOInfoRefs = std::vector<const AfterglowSSBOInfo*>;

	// TODO: HOST_VISIBLE feature.

	// In-order, OnceCompleted also means Initialized is completed.
	enum class DispatchStatus : uint16_t {
		None, 
		Initialized, 
		OnceCompleted
	};

	// TODO: ... Use other ssbos of compute task in this task.
	// Not allow in init compute shader? due to srcSSBO may not be initialized.
	struct ExternalSSBO {
		std::string materialName;
		std::string ssboName;
	};
	using ExternalSSBOs = std::vector<ExternalSSBO>;

	// SSBOInfos& ssboInfos();

	/**
	* @brief: Append a ssboInfo into the compute task.
	* @desc: 
	*	If the ssboInfo has specified usage e.g. indexInput or Indirect, 
	*	It's ElementLayout will be replace to the preset ElementLayout, 
	*	see makeIndexSSBOLayout() etc.
	*/
	void appendSSBOInfo(AfterglowSSBOInfo&& ssboInfo);
	void removeSSBOInfo(SSBOInfos::const_iterator iterator);
	const SSBOInfos& ssboInfos() const;

	// Including multiple SSBOs for frame in flight (if exists).
	bool isMultipleSSBOs(const AfterglowSSBOInfo& ssboInfo) const noexcept;
	uint32_t numSSBOs(const AfterglowSSBOInfo& ssboInfo) const noexcept;
	std::vector<std::string> ssboInfoDeclarationNames(const AfterglowSSBOInfo& ssboInfo) const;

	// @brief: If computeOnly == true, material layout will skip graphics layout.
	void setComputeOnly(bool computeOnly) noexcept;
	void setComputeShader(const std::string& computeShaderPath);
	void setDispatchGroup(const compute::DispatchGroup& dispatchGroup) noexcept;
	void setDispatchFrequency(compute::DispatchFrequency dispatchFrequency) noexcept;
	void setDispatchStatus(uint32_t frameIndex, DispatchStatus dispatchStatus);
	// @brief: Set dispatch status for all frame indices.
	void setDispatchStatuses(DispatchStatus dispatchStatus);

	bool isComputeOnly() const noexcept;
	// @return: fist VetexInput usage ssbo index. If vertex input ssbo not found, return a nullptr.
	const AfterglowSSBOInfo* vertexInputSSBOInfo() const;
	const AfterglowSSBOInfo* indexInputSSBOInfo() const;

	const std::string& computeShaderPath() const  noexcept;
	const compute::DispatchGroup dispatchGroup() const noexcept;
	compute::DispatchFrequency dispatchFrequency() const noexcept;
	DispatchStatus dispatchStatus(uint32_t frameIndex) const;

	// @return: Ture if shader path is one of the SSBOInfos' ComputeShader InitResource.
	bool isInitComputeShader(const std::string& shaderPath) const;
	static bool hasDefaultElementLayout(compute::SSBOUsage usage) noexcept;

	// @return: SSBOs which should be initialized from compute shaders.
	SSBOInfoRefs computeShaderInitSSBOInfos() const;

	// For static mesh instancing.
	//const SSBOInstancingInfos& ssboInstancingInfos() const;
	const AfterglowSSBOInfo* instancingSSBOInfo() const;
	uint32_t instanceCount() const;

	const AfterglowSSBOInfo* indirectSSBOInfo() const;

	// @desc: all initComputeShaders belong to shader::Stage::Compute;
	uint32_t numInitComputeShaders() const;

	// @brief: If SSBOInfo not found, return nullptr.
	AfterglowSSBOInfo* findSSBOInfo(const std::string& name);
	const AfterglowSSBOInfo* findSSBOInfo(const std::string& name) const;

	const ExternalSSBOs& externalSSBOs() const noexcept;
	ExternalSSBOs& externalSSBOs() noexcept;

	// @return: Is the task able to dispatch according the current status.
	bool queryDispatchable(uint32_t frameIndex) noexcept;

private:
	// TODO: Call every frame, try to cache it?
	inline const AfterglowSSBOInfo* findFirstUsageSSBOInfo(compute::SSBOUsage usage) const;

	inline AfterglowStructLayout makeIndexSSBOLayout();
	inline AfterglowStructLayout makeIndexedIndirectSSBOLayout();
	// TODO: Other specified layout from callback functions.


	bool _computeOnly = false;
	compute::DispatchFrequency _dispatchFrequency = compute::DispatchFrequency::Never;
	std::array<DispatchStatus, cfg::maxFrameInFlight> _inFlightDispatchStatuses = { DispatchStatus::None };
	std::string _computeShaderPath;
	compute::DispatchGroup _dispatchGroup = {};
	SSBOInfos _ssboInfos;
	ExternalSSBOs _externalSSBOs;
};

