#include "AfterglowComputeTask.h"

#include <format>

#include "DebugUtilities.h"
#include "Configurations.h"

AfterglowComputeTask::SSBOInfos& AfterglowComputeTask::ssboInfos() {
	return _ssboInfos;
}

const AfterglowComputeTask::SSBOInfos& AfterglowComputeTask::ssboInfos() const {
	return _ssboInfos;
}

bool AfterglowComputeTask::isMultipleSSBOs(const AfterglowSSBOInfo& ssboInfo) const {
	return ssboInfo.accessMode == compute::SSBOAccessMode::ReadWrite;
}

uint32_t AfterglowComputeTask::numSSBOs(const AfterglowSSBOInfo& ssboInfo) const {
	return isMultipleSSBOs(ssboInfo) ? cfg::maxFrameInFlight : 1;
}

std::vector<std::string> AfterglowComputeTask::ssboInfoDeclarationNames(const AfterglowSSBOInfo& ssboInfo) const {
	// Single SSBO
	if (!isMultipleSSBOs(ssboInfo)) {
		return { ssboInfo.name };
	}
	// else: Multiple SSBOs for Frame in Flight
	std::vector<std::string> names;
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		std::string suffix;
		if (index < cfg::maxFrameInFlight - 1) {
			suffix = "In";
			if (index != 0) {
				suffix += std::to_string(index);
			}
		}
		else {
			suffix = "Out";
		}
		names.emplace_back(ssboInfo.name + suffix);
	}
	return names;
}

void AfterglowComputeTask::setComputeOnly(bool computeOnly) {
	_computeOnly = computeOnly;
}

void AfterglowComputeTask::setComputeShader(const std::string& computeShaderPath) {
	_computeShaderPath = computeShaderPath;
}

void AfterglowComputeTask::setDispatchGroup(const compute::DispatchGroup& dispatchGroup) {
	_dispatchGroup = dispatchGroup;
}

void AfterglowComputeTask::setDispatchFrequency(DispatchFrequency dispatchFrequency) {
	_dispatchFrequency = dispatchFrequency;
}

void AfterglowComputeTask::setDispatchStatus(DispatchStatus dispatchStatus) {
	_dispatchStatus = dispatchStatus;
}

//bool AfterglowComputeTask::setSSBOInstancingInfo(const std::string& ssboInfoName, const std::string& materialName, const std::string& modelPath) {
//	auto iterator = findSSBOInfo(ssboInfoName);
//	if (iterator == _ssboInfos.end()) {
//		DEBUG_CLASS_WARNING(std::format("SSBOInfo is not exist: {}", ssboInfoName));
//		return false;
//	}
//	auto& ssboInfo = *iterator;
//	if (ssboInfo.usage != compute::SSBOUsage::Instancing) {
//		DEBUG_CLASS_WARNING(std::format("SSBOInfo.usage is not Instancing: {}", ssboInfoName));
//		return false;
//	}
//	_ssboInstancingInfos[&ssboInfo] = { materialName, modelPath };
//	return true;
//}
//
//void AfterglowComputeTask::removeSSOBInstancingInfo(const std::string& ssboInfoName) {
//	auto iterator = findSSBOInfo(ssboInfoName);
//	if (iterator == _ssboInfos.end()) {
//		DEBUG_CLASS_WARNING(std::format("SSBOInfo is not exist: {}", ssboInfoName));
//	}
//	_ssboInstancingInfos.erase(&*iterator);
//}

bool AfterglowComputeTask::isComputeOnly() const {
	return _computeOnly;
}

const AfterglowSSBOInfo* AfterglowComputeTask::vertexInputSSBO() const {
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.usage == compute::SSBOUsage::VertexInput) {
			return &ssboInfo;
		}
	}
	return nullptr;
}

const std::string& AfterglowComputeTask::computeShaderPath() const {
	return _computeShaderPath;
}

const compute::DispatchGroup AfterglowComputeTask::dispatchGroup() const {
	return _dispatchGroup;
}

AfterglowComputeTask::DispatchFrequency AfterglowComputeTask::dispatchFrequency() const {
	return _dispatchFrequency;
}

AfterglowComputeTask::DispatchStatus AfterglowComputeTask::dispatchStatus() const {
	return _dispatchStatus;
}

bool AfterglowComputeTask::isInitComputeShader(const std::string& shaderPath) const {
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.initMode == compute::SSBOInitMode::ComputeShader && ssboInfo.initResource == shaderPath) {
			return true;
		}
	}
	return false;
}

AfterglowComputeTask::SSBOInfoRefs AfterglowComputeTask::computeShaderInitSSBOInfos() const {
	SSBOInfoRefs refs;
	for (auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.initMode == compute::SSBOInitMode::ComputeShader) {
			refs.push_back(&ssboInfo);
		}
	}
	return refs;
}

//const AfterglowComputeTask::SSBOInstancingInfos& AfterglowComputeTask::ssboInstancingInfos() const {
//	return _ssboInstancingInfos;
//}

const AfterglowSSBOInfo* AfterglowComputeTask::instancingSSBOInfo() const {
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.usage == compute::SSBOUsage::Instancing) {
			return &ssboInfo;
		}
	}
	return nullptr;
}

uint32_t AfterglowComputeTask::instanceCount() const {
	auto* ssboInfo = instancingSSBOInfo();
	if (!ssboInfo) {
		return 1;
	}
	return ssboInfo->numElements;
}

uint32_t AfterglowComputeTask::numInitComputeShaders() const {
	uint32_t nums = 0;
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.initMode == compute::SSBOInitMode::ComputeShader) {
			++nums;
		}
	}
	return nums;
}

AfterglowComputeTask::SSBOInfos::iterator AfterglowComputeTask::findSSBOInfo(const std::string& name) {
	auto iterator = _ssboInfos.begin();
	while (iterator < _ssboInfos.end()) {
		if (iterator->name == name) {
			break;
		}
		++iterator;
	}
	return iterator;
}
