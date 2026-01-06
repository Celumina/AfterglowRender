#pragma once
#include "AfterglowShaderModule.h"
#include "AfterglowPipelineLayout.h"

class AfterglowComputePipeline : public AfterglowProxyObject<AfterglowComputePipeline, VkPipeline,VkComputePipelineCreateInfo> {
public:
	AfterglowComputePipeline(AfterglowDevice& device);
	~AfterglowComputePipeline();

	void setComputeShader(AfterglowShaderModule& shaderModule);
	AfterglowPipelineLayout& pipelineLayout();

proxy_protected:

	void initCreateInfo();
	void create();

private:
	struct CreateInfoDependencies{
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	};

	std::unique_ptr<CreateInfoDependencies> _dependencies;

	AfterglowDevice& _device;
	AfterglowPipelineLayout::AsElement _pipelineLayout;
};

