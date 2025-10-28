#pragma once

#include <array>
#include "AfterglowPipelineLayout.h"
#include "AfterglowShaderModule.h"
#include "AfterglowRenderPass.h"
#include "AfterglowDescriptorSetLayout.h"
#include "AfterglowVertexBuffer.h"

class AfterglowStructLayout;

class AfterglowPipeline : public AfterglowProxyObject<AfterglowPipeline, VkPipeline, VkGraphicsPipelineCreateInfo> {
public:
	enum class ShaderStageIndex : uint32_t {
		Vertex = 0,
		Fragment = 1,
		
		EnumCount
	};

	enum class BlendMode : uint32_t {
		Opaque, 
		Alpha, 

		EnumCount
	};

	using ShaderModulePointerArray = std::vector<AfterglowShaderModule*>;
	using ShaderStageCreateInfoArray = std::vector<VkPipelineShaderStageCreateInfo>;
	using DynamicStateArray = std::vector<VkDynamicState>;

	AfterglowPipeline(
		AfterglowRenderPass& renderPass, 
		render::Domain subpassDomain, 
		std::type_index vertexTypeIndex = util::TypeIndex<vert::StandardVertex>()
	);

	~AfterglowPipeline();

	inline AfterglowDevice& device() noexcept;
	inline AfterglowSwapchain& swapchain();
	AfterglowRenderPass& renderPass();
	AfterglowPipelineLayout& pipelineLayout();

	template<size_t index = 0>
	void assignVertex(std::type_index vertexTypeIndex);

	/**
	* @brief: Aquire vertex attribute and binding description for this pipeline, If this function have not been call, pipeline use vert::StandardVertex as default.
	*/ 
	template<typename VertexType>
	void assignVertex();

	/**
	* @brief: Assign vertex descriptions from runtime struct.
	*/
	void assignVertex(const AfterglowStructLayout& structLayout);

	void setVertexShader(AfterglowShaderModule& shaderModule);
	void setFragmentShader(AfterglowShaderModule& shaderModule);

	void setCullMode(VkCullModeFlags cullMode);
	void setTopology(render::Topology topology);

	void setBlendMode(BlendMode mode);

proxy_protected:
	void initCreateInfo();
	void create();

private:
	void verifyDependencies() const;

	static inline VkPrimitiveTopology vulkanTopology(render::Topology topology);
	static inline VkFormat vulkanVertexAttributeFormat(uint32_t attributeByteSize, uint32_t numAttributeComponents);

	struct CreateInfoDependencies {
		ShaderStageCreateInfoArray shaderStageCreateInfos;

		DynamicStateArray dynamicStates;
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;

		// Vertex
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
		VkVertexInputBindingDescription vertexInputBindingDescription;
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		// Input Assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;

		// Viewport
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo;

		// Rasterization
		VkPipelineRasterizationStateCreateInfo rasterizerStateCreateInfo;

		VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo;

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;

		// Depth and stencil test
		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo;
	};

	std::unique_ptr<CreateInfoDependencies> _dependencies;
	AfterglowRenderPass& _renderPass;
	AfterglowPipelineLayout::AsElement _pipelineLayout;
};

template<size_t index>
inline void AfterglowPipeline::assignVertex(std::type_index vertexTypeIndex) {
	if constexpr (index < std::tuple_size_v<vert::RegisteredVertexTypes>) {
		using Vertex = std::tuple_element_t<index, vert::RegisteredVertexTypes>;
		if (util::TypeIndex<Vertex>() == vertexTypeIndex) {
			assignVertex<Vertex>();
			return;
		}
		assignVertex<index + 1>(vertexTypeIndex);
	}
	else {
		throw runtimeError("Not suitable vertex type, verify that the type index corresponds to a registered vertex type.");
	}
}

template<typename VertexType>
inline void AfterglowPipeline::assignVertex() {
	static_assert(vert::IsVertex<VertexType>(), "[AfterglowPipeline] Can not assign type, due to parameter type is not a vertex type.");

	if (!_dependencies) {
		throw runtimeError("Can not assign vertex type due to pipeline was created.");
	}

	_dependencies->vertexInputBindingDescription = AfterglowVertexBufferTemplate<VertexType>::getBindingDescription();
	_dependencies->vertexInputAttributeDescriptions = AfterglowVertexBufferTemplate<VertexType>::getAttributeDescriptions();

	_dependencies->vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(_dependencies->vertexInputAttributeDescriptions.size());
	_dependencies->vertexInputStateCreateInfo.pVertexAttributeDescriptions = _dependencies->vertexInputAttributeDescriptions.data();
}
