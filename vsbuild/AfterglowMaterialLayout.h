#pragma once

#include <map>
#include <vector>
#include "AfterglowMaterial.h"
#include "AfterglowDescriptorSetLayout.h"
#include "AfterglowPipeline.h"
#include "AfterglowComputePipeline.h"

class AfterglowShaderAsset;

// TODO: Many Pipelines for Different vertexTypes. and then remove the material vertexType mark.
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

	AfterglowDevice& device() noexcept;
	AfterglowPipeline& pipeline();
	AfterglowComputePipeline& computePipeline();
	AfterglowComputePipeline* indirectResetPipeline();
	
	AfterglowComputePipeline::Array& ssboInitComputePipelines();

	AfterglowMaterial& material() noexcept;
	const AfterglowMaterial& material() const noexcept;

	void setMaterial(const AfterglowMaterial& material);

	void compileVertexShader(const std::string& shaderCode);
	void compileFragmentShader(const std::string& shaderCode);
	void compileComputeShader(const std::string& shaderCode);

	// @brief: Apply material modification to layout.
	void updateDescriptorSetLayouts(AfterglowDescriptorSetLayout& globalSetLayout, AfterglowDescriptorSetLayout& perObjectSetLayout);

	// @brief: Extra descriptor set layout for compute shader external ssbos.
	void activateComputeExternalSSBOSetLayout(AfterglowDescriptorSetLayout& externalSSBOSetLayout);

	void updatePipeline();
	void updateComputePipeline();

	// @brief: Update both of the compute pipeline and the render pipeline.
	void updatePipelines();

	static VkShaderStageFlags vulkanShaderStage(shader::Stage stage);

private:
	struct ComputeLayout {
		AfterglowComputePipeline::AsElement pipeline;
		AfterglowShaderModule::AsElement shader;

		// For indirect buffer reset
		AfterglowComputePipeline::AsElement indirectResetPipeline;
		AfterglowShaderModule::AsElement indirectResetShader;

		AfterglowComputePipeline::Array ssboInitPipelines;
		AfterglowShaderModule::Array ssboInitShaders;
	};

	inline void appendDescriptorSetLayout(shader::Stage stage);

	// Here use uniform pipeline layout, including graphics pipeline and compute pipeline.
	inline void fillPipelineLayout(AfterglowPipelineLayout& pipelineLayout);

	inline bool isComputeOnly();
	inline std::runtime_error runtimeError(const char* text);

	inline void verifyComputeTask();
	inline AfterglowShaderAsset& indirectResetShaderAsset();

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

