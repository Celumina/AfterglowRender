#include "AfterglowComputeTask.h"

#include <format>

#include "DebugUtilities.h"
#include "Configurations.h"
#include "DebugUtilities.h"

//AfterglowComputeTask::SSBOInfos& AfterglowComputeTask::ssboInfos() {
//	return _ssboInfos;
//}

void AfterglowComputeTask::appendSSBOInfo(AfterglowSSBOInfo&& ssboInfo) {
	auto& info = _ssboInfos.emplace_back(std::move(ssboInfo));

	if (info.usage() == compute::SSBOUsage::IndexInput) {
		info.setElementLayout(makeIndexSSBOLayout());
	}
	if (info.usage() == compute::SSBOUsage::Indirect) {
		info.rename(compute::indirectSSBOName);
		info.setElementLayout(makeIndexedIndirectSSBOLayout());
	}
}

void AfterglowComputeTask::removeSSBOInfo(SSBOInfos::const_iterator iterator) {
	if (iterator >= _ssboInfos.cbegin() && iterator < _ssboInfos.end()) {
		_ssboInfos.erase(iterator);
	}
	else {
		DEBUG_CLASS_WARNING("The iterator is out of range.");
	}
}

const AfterglowComputeTask::SSBOInfos& AfterglowComputeTask::ssboInfos() const {
	return _ssboInfos;
}

bool AfterglowComputeTask::isMultipleSSBOs(const AfterglowSSBOInfo& ssboInfo) const noexcept {
	return ssboInfo.accessMode() == compute::SSBOAccessMode::ReadWrite;
}

uint32_t AfterglowComputeTask::numSSBOs(const AfterglowSSBOInfo& ssboInfo) const noexcept {
	return isMultipleSSBOs(ssboInfo) ? cfg::maxFrameInFlight : 1;
}

std::vector<std::string> AfterglowComputeTask::ssboInfoDeclarationNames(const AfterglowSSBOInfo& ssboInfo) const {
	// Single SSBO
	if (!isMultipleSSBOs(ssboInfo)) {
		return { ssboInfo.name()};
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
		names.emplace_back(ssboInfo.name() + suffix);
	}
	return names;
}

void AfterglowComputeTask::setComputeOnly(bool computeOnly) noexcept {
	_computeOnly = computeOnly;
}

void AfterglowComputeTask::setComputeShader(const std::string& computeShaderPath) {
	_computeShaderPath = computeShaderPath;
}

void AfterglowComputeTask::setDispatchGroup(const compute::DispatchGroup& dispatchGroup) noexcept {
	_dispatchGroup = dispatchGroup;
}

void AfterglowComputeTask::setDispatchFrequency(compute::DispatchFrequency dispatchFrequency) noexcept {
	// Handling "manually dispatch again".
	for (auto& status : _inFlightDispatchStatuses) {
		if (dispatchFrequency == compute::DispatchFrequency::Once &&
			status == DispatchStatus::OnceCompleted) {
			status = DispatchStatus::Initialized;	
		}
	}
	_dispatchFrequency = dispatchFrequency;
}

void AfterglowComputeTask::setDispatchStatus(uint32_t frameIndex, DispatchStatus dispatchStatus) {
	_inFlightDispatchStatuses[frameIndex] = dispatchStatus;
}

void AfterglowComputeTask::setDispatchStatuses(DispatchStatus dispatchStatus) {
	std::fill(_inFlightDispatchStatuses.begin(), _inFlightDispatchStatuses.end(), dispatchStatus);
}

bool AfterglowComputeTask::isComputeOnly() const noexcept{
	return _computeOnly;
}

const AfterglowSSBOInfo* AfterglowComputeTask::vertexInputSSBOInfo() const {
	return findFirstUsageSSBOInfo(compute::SSBOUsage::VertexInput);
}

const AfterglowSSBOInfo* AfterglowComputeTask::indexInputSSBOInfo() const {
	return findFirstUsageSSBOInfo(compute::SSBOUsage::IndexInput);
}

const std::string& AfterglowComputeTask::computeShaderPath() const noexcept {
	return _computeShaderPath;
}

const compute::DispatchGroup AfterglowComputeTask::dispatchGroup() const noexcept {
	return _dispatchGroup;
}

compute::DispatchFrequency AfterglowComputeTask::dispatchFrequency() const noexcept {
	return _dispatchFrequency;
}

AfterglowComputeTask::DispatchStatus AfterglowComputeTask::dispatchStatus(uint32_t frameIndex) const {
	return _inFlightDispatchStatuses[frameIndex];
}

bool AfterglowComputeTask::isInitComputeShader(const std::string& shaderPath) const {
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.initMode() == compute::SSBOInitMode::ComputeShader && ssboInfo.initResource() == shaderPath) {
			return true;
		}
	}
	return false;
}

bool AfterglowComputeTask::hasDefaultElementLayout(compute::SSBOUsage usage) noexcept {
	return usage == compute::SSBOUsage::IndexInput
		|| usage == compute::SSBOUsage::Indirect;
}

AfterglowComputeTask::SSBOInfoRefs AfterglowComputeTask::computeShaderInitSSBOInfos() const {
	SSBOInfoRefs refs;
	for (auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.initMode() == compute::SSBOInitMode::ComputeShader) {
			refs.push_back(&ssboInfo);
		}
	}
	return refs;
}

const AfterglowSSBOInfo* AfterglowComputeTask::instancingSSBOInfo() const {
	return findFirstUsageSSBOInfo(compute::SSBOUsage::Instancing);
}

uint32_t AfterglowComputeTask::instanceCount() const {
	auto* ssboInfo = instancingSSBOInfo();
	if (!ssboInfo) {
		return 1;
	}
	return ssboInfo->numElements();
}

const AfterglowSSBOInfo* AfterglowComputeTask::indirectSSBOInfo() const {
	return findFirstUsageSSBOInfo(compute::SSBOUsage::Indirect);
}

uint32_t AfterglowComputeTask::numInitComputeShaders() const {
	uint32_t nums = 0;
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.initMode() == compute::SSBOInitMode::ComputeShader) {
			++nums;
		}
	}
	return nums;
}

AfterglowSSBOInfo* AfterglowComputeTask::findSSBOInfo(const std::string& name) {
	auto iterator = _ssboInfos.begin();
	while (iterator < _ssboInfos.end()) {
		if (iterator->name() == name) {
			return &*iterator;
		}
		++iterator;
	}
	return nullptr;
}

const AfterglowSSBOInfo* AfterglowComputeTask::findSSBOInfo(const std::string& name) const {
	return const_cast<AfterglowComputeTask*>(this)->findSSBOInfo(name);
}

const AfterglowComputeTask::ExternalSSBOs& AfterglowComputeTask::externalSSBOs() const noexcept {
	return _externalSSBOs;
}

AfterglowComputeTask::ExternalSSBOs& AfterglowComputeTask::externalSSBOs() noexcept {
	return _externalSSBOs;
}

bool AfterglowComputeTask::queryDispatchable(uint32_t frameIndex) noexcept {
	auto status = _inFlightDispatchStatuses[frameIndex];
	if (_dispatchFrequency == compute::DispatchFrequency::Never) {
		return false;
	}
	else if (_dispatchFrequency == compute::DispatchFrequency::Once
		&& status == AfterglowComputeTask::DispatchStatus::OnceCompleted) {
		return false;
	}
	else if (_dispatchFrequency == compute::DispatchFrequency::Once
		&& status == AfterglowComputeTask::DispatchStatus::Initialized) {
		setDispatchStatus(frameIndex, AfterglowComputeTask::DispatchStatus::OnceCompleted);
	}
	return true;
}

inline const AfterglowSSBOInfo* AfterglowComputeTask::findFirstUsageSSBOInfo(compute::SSBOUsage usage) const {
	for (const auto& ssboInfo : _ssboInfos) {
		if (ssboInfo.usage() == usage) {
			return &ssboInfo;
		}
	}
	return nullptr;
}

inline AfterglowStructLayout AfterglowComputeTask::makeIndexSSBOLayout() {
	AfterglowStructLayout layout;
	layout.addAttribute(AfterglowStructLayout::AttributeType::UnsignedInt4, "indexPack");
	return layout;
}

inline AfterglowStructLayout AfterglowComputeTask::makeIndexedIndirectSSBOLayout() {
	AfterglowStructLayout layout;
	layout.addAttribute(AfterglowStructLayout::AttributeType::UnsignedInt, "indexCount");
	layout.addAttribute(AfterglowStructLayout::AttributeType::UnsignedInt, "instanceCount");
	layout.addAttribute(AfterglowStructLayout::AttributeType::UnsignedInt, "firstIndex");
	layout.addAttribute(AfterglowStructLayout::AttributeType::Int, "vertexOffset");
	layout.addAttribute(AfterglowStructLayout::AttributeType::UnsignedInt, "firstInstance");
	return layout;
}
