#include "AfterglowComputePipeline.h"
#include "Configurations.h"

AfterglowComputePipeline::AfterglowComputePipeline(AfterglowDevice& device) : 
	_device(device), 
	_pipelineLayout(AfterglowPipelineLayout::makeElement(device)), 
	_dependencies(std::make_unique<CreateInfoDependencies>()) {
}

AfterglowComputePipeline::~AfterglowComputePipeline() {
	destroy(vkDestroyPipeline, _device, data(), nullptr);
}

void AfterglowComputePipeline::setComputeShader(AfterglowShaderModule& shaderModule) {
	if (!_dependencies) {
		throw runtimeError("Pipeline had been created, could not reset the shader.");
	}
	_dependencies->shaderStageCreateInfo.module = shaderModule;
}

AfterglowPipelineLayout& AfterglowComputePipeline::pipelineLayout() {
	return _pipelineLayout;
}

void AfterglowComputePipeline::initCreateInfo() {
	_dependencies->shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	_dependencies->shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	_dependencies->shaderStageCreateInfo.pName = cfg::shaderEntryName;

	info().sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info().stage = _dependencies->shaderStageCreateInfo;
}

void AfterglowComputePipeline::create() {
	// Delay binding for modify pipline layout after create info.
	info().layout = _pipelineLayout;

	if (vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create compute pipeline.");
	}
	_dependencies.reset();
}
