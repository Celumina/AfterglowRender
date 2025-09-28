#pragma once
#include <vector>
#include <map>

#include "AfterglowSSBOInfo.h"

// TODO: Inrefect Attributes For Serialize Support
// TODO: Serialize template, support for class attribute, like Serialize<std::string>::read(); Serialize<std::string>::write();


// Resource and configuarations desciptions relative to compute shader.
class AfterglowComputeTask {
public:
	using SSBOInfos = std::vector<AfterglowSSBOInfo>;
	using SSBOInfoRefs = std::vector<const AfterglowSSBOInfo*>;
	
	// TODO: Only one instancing buffer per compute task is supported yet.
	//struct InstancingInfo {
	//	std::string materialName;
	//	std::string modelPath;
	//};
	//using SSBOInstancingInfos = std::map<const AfterglowSSBOInfo*, InstancingInfo>;

	// TODO: HOST_VISIBLE feature.
	// TODO: Usage Enum: For vertexInput? build global textures?

	enum class DispatchFrequency {
		Never,         // Submit to command buffer manually
		Once,          // Submit once when compute resource is initialized, or you can set it on manually.
		PerFrame       // Submit every update

		// TODO:  Fixed update rate
	};

	SSBOInfos& ssboInfos();
	const SSBOInfos& ssboInfos() const;

	// Including multiple SSBOs for frame in flight (if exists).
	bool isMultipleSSBOs(const AfterglowSSBOInfo& ssboInfo) const;
	uint32_t numSSBOs(const AfterglowSSBOInfo& ssboInfo) const;
	std::vector<std::string> ssboInfoDeclarationNames(const AfterglowSSBOInfo& ssboInfo) const;

	// @brief: If computeOnly == true, material layout will skip graphics layout.
	void setComputeOnly(bool computeOnly);
	void setComputeShader(const std::string& computeShaderPath);
	void setDispatchGroup(const compute::DispatchGroup& dispatchGroup);
	void setDispatchFrequency(DispatchFrequency dispatchFrequency);

	//bool setSSBOInstancingInfo(const std::string& ssboInfoName, const std::string& materialName, const std::string& modelPath);
	//void removeSSOBInstancingInfo(const std::string& ssboInfoName);

	bool isComputeOnly() const;
	// @return: fist VetexInput usage ssbo index. If vertex input ssbo not found, return a nullptr.
	const AfterglowSSBOInfo* vertexInputSSBO() const;
	const std::string& computeShaderPath() const;
	const compute::DispatchGroup dispatchGroup() const;
	DispatchFrequency dispatchFrequency() const;

	// @return: SSBOs which should be initialized from compute shaders.
	SSBOInfoRefs computeShaderInitSSBOInfos() const;

	//// For static mesh instancing.
	//const SSBOInstancingInfos& ssboInstancingInfos() const;
	const AfterglowSSBOInfo* instancingSSBOInfo() const;
	uint32_t instanceCount() const;

	SSBOInfos::iterator findSSBOInfo(const std::string& name);

private:
	bool _computeOnly = false;
	std::string _computeShaderPath;
	compute::DispatchGroup _dispatchGroup = {};
	DispatchFrequency _dispatchFrequency = DispatchFrequency::Never;
	SSBOInfos _ssboInfos;
	// SSBOInstancingInfos _ssboInstancingInfos;
};

