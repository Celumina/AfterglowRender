#pragma once

#include <map>
#include <vector>
#include "AfterglowMaterial.h"
#include "AfterglowDescriptorSetLayout.h"
#include "AfterglowPipeline.h"
#include "AfterglowComputePipeline.h"

class AfterglowMaterialLayout {
public: 
	using DescriptorSetLayouts = std::map<shader::Stage, AfterglowDescriptorSetLayout::AsElement>;
	using RawDescriptorSetLayouts = std::vector<typename AfterglowDescriptorSetLayout::Raw>;

	AfterglowMaterialLayout(
		AfterglowRenderPass& renderPass, 
		const AfterglowMaterial& refMaterial = AfterglowMaterial::emptyMaterial()
	);

	DescriptorSetLayouts& descriptorSetLayouts();
	const DescriptorSetLayouts& descriptorSetLayouts() const;

	RawDescriptorSetLayouts& rawDescriptorSetLayouts();
	const RawDescriptorSetLayouts& rawDescriptorSetLayouts() const;

	AfterglowDevice& device();
	AfterglowPipeline& pipeline();
	AfterglowComputePipeline& computePipeline();
	
	bool shouldInitSSBOs();
	// @brief: aquire shouldInitSSBOsTrigger bool flag, and then set flag to false, invoke by CommandManager. 
	bool shouldInitSSBOsTrigger();
	AfterglowComputePipeline::Array& ssboInitComputePipelines();

	AfterglowMaterial& material();
	const AfterglowMaterial& material() const;

	void setMaterial(const AfterglowMaterial& material);

	void compileVertexShader(const std::string& shaderCode);
	void compileFragmentShader(const std::string& shaderCode);
	void compileComputeShader(const std::string& shaderCode);

	// @brief: Apply material modification to layout.
	void updateDescriptorSetLayouts(AfterglowDescriptorSetLayout& globalSetLayout, AfterglowDescriptorSetLayout& perObjectSetLayout);
	void updatePipeline();
	void updateComputePipeline();

private:
	struct ComputeLayout {
		AfterglowComputePipeline::AsElement pipeline;
		AfterglowShaderModule::AsElement shader;

		std::array<bool, cfg::maxFrameInFlight> shouldInitSSBOs = { false };
		AfterglowComputePipeline::Array ssboInitPipelines;
		AfterglowShaderModule::Array ssboInitShaders;
	};

	inline void appendDescriptorSetLayout(shader::Stage stage);

	// Here use uniform pipeline layout, including graphics pipeline and compute pipeline.
	inline void fillPipelineLayout(AfterglowPipelineLayout& pipelineLayout);

	inline bool isComputeOnly();
	inline std::runtime_error runtimeError(const char* text);

	inline void verifyComputeTask();

	static VkShaderStageFlags vulkanShaderStage(shader::Stage stage);

	AfterglowRenderPass& _renderPass;

	AfterglowPipeline::AsElement _pipeline;
	AfterglowShaderModule::AsElement _vertexShader;
	AfterglowShaderModule::AsElement _fragmentShader;

	AfterglowMaterial _material;
	DescriptorSetLayouts _descriptorSetLayouts;
	// This raw array including the global set layout and per object (for mesh) set layout
	RawDescriptorSetLayouts _rawDescriptorSetLayouts;

	// For compute task
	std::unique_ptr<ComputeLayout> _computeLayout = nullptr;
};

