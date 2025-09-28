#include "AfterglowPipeline.h"
#include "Configurations.h"

AfterglowPipeline::AfterglowPipeline(AfterglowRenderPass& renderPass, render::Domain subpassDomain, std::type_index vertexTypeIndex) :
	_renderPass(renderPass), 
	_pipelineLayout(AfterglowPipelineLayout::makeElement(renderPass.device())) {
	// Pipeline layout: the uniform and push values referenced by the shader
	// that can be updated at draw time
	_dependencies = std::make_unique<CreateInfoDependencies>();

	// Herr are some default settings.
	
	// Shader loaded in the MaterialManager, do not make a default, because these are costly.
	// Use a automatically compile and show shader system.

	// Vertex Input
	assignVertex(vertexTypeIndex);

	// Fill stage infos.
	for (uint32_t index = 0; index < util::EnumValue(ShaderStageIndex::EnumCount); ++index) {
		_dependencies->shaderStageCreateInfos.push_back(VkPipelineShaderStageCreateInfo{});
		_dependencies->shaderStageCreateInfos.back().sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Shader's entrypoint.
		_dependencies->shaderStageCreateInfos.back().pName = cfg::shaderEntryName;
	}

	// Pipeline subpass index 
	auto& subpassContext = _renderPass.subpassContext();
	info().subpass = subpassContext.subpassIndex(subpassDomain);
	_dependencies->multisamplingStateCreateInfo.rasterizationSamples = subpassContext.rasterizationSampleCount(subpassDomain);

	// Default blend mode
	setBlendMode(BlendMode::Opaque);
}

AfterglowPipeline::~AfterglowPipeline() {
	destroy(vkDestroyPipeline, device(), data(), nullptr);
}

inline AfterglowDevice& AfterglowPipeline::device() {
	return _renderPass.device();
}

inline AfterglowSwapchain& AfterglowPipeline::swapchain() {
	return _renderPass.swapchain();
}

AfterglowRenderPass& AfterglowPipeline::renderPass() {
	return _renderPass;
}

AfterglowPipelineLayout& AfterglowPipeline::pipelineLayout() {
	return _pipelineLayout;
}

void AfterglowPipeline::assignVertex(const AfterglowStructLayout& structLayout) {
	// Vertex input binding description
	_dependencies->vertexInputBindingDescription = {
		.binding = 0, 
		.stride = structLayout.byteSize(), 
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	// Vertex attribute descriptions
	auto& descriptions = _dependencies->vertexInputAttributeDescriptions;
	descriptions.clear();
	structLayout.forEachAttributeMemberWithOffset(
	[&descriptions](const AfterglowStructLayout::AttributeMember& attribute, uint32_t offset){
			descriptions.emplace_back(VkVertexInputAttributeDescription{
				.location = static_cast<uint32_t>(descriptions.size()),
				.binding = 0, 
				.format = vulkanVertexAttributeFormat(
					AfterglowStructLayout::attributeByteSize(attribute.type), 
					AfterglowStructLayout::numAttributeComponents(attribute.type)
				), 
				.offset = offset
			});
		}
	);
	_dependencies->vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(descriptions.size());
	_dependencies->vertexInputStateCreateInfo.pVertexAttributeDescriptions = descriptions.data();
}

void AfterglowPipeline::setVertexShader(AfterglowShaderModule& shaderModule) {
	verifyDependencies();
	// Vertex Stage
	_dependencies->shaderStageCreateInfos[util::EnumValue(ShaderStageIndex::Vertex)].stage = VK_SHADER_STAGE_VERTEX_BIT;
	_dependencies->shaderStageCreateInfos[util::EnumValue(ShaderStageIndex::Vertex)].module = shaderModule;
}

void AfterglowPipeline::setFragmentShader(AfterglowShaderModule& shaderModule) {
	verifyDependencies();
	// Fragment Stage
	_dependencies->shaderStageCreateInfos[util::EnumValue(ShaderStageIndex::Fragment)].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	_dependencies->shaderStageCreateInfos[util::EnumValue(ShaderStageIndex::Fragment)].module = shaderModule;
}

void AfterglowPipeline::setCullMode(VkCullModeFlags cullMode) {
	verifyDependencies();
	_dependencies->rasterizerStateCreateInfo.cullMode = cullMode;
}

void AfterglowPipeline::setTopology(render::Topology topology) {
	verifyDependencies();
	_dependencies->inputAssemblyStateCreateInfo.topology = vulkanTopology(topology);
}

void AfterglowPipeline::setBlendMode(BlendMode mode) {
	verifyDependencies();
	auto& colorBlendAttachment = _dependencies->colorBlendAttachment;
	switch (mode) {
	case BlendMode::Opaque:
		// FACTOR: weight
		// OP: operation
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		// Here new alpha replace old's.
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		_dependencies->depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		break;
	case BlendMode::Alpha:
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		_dependencies->depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
		break;
	default:
		DEBUG_CLASS_WARNING("Unknown blend mode.");
	}
}

void AfterglowPipeline::initCreateInfo() {
	// Dynamic state
	_dependencies->dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT, 
		VK_DYNAMIC_STATE_SCISSOR
	};

	_dependencies->dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	_dependencies->dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(_dependencies->dynamicStates.size());
	_dependencies->dynamicStateCreateInfo.pDynamicStates = _dependencies->dynamicStates.data();

	_dependencies->vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	_dependencies->vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	_dependencies->vertexInputStateCreateInfo.pVertexBindingDescriptions = &_dependencies->vertexInputBindingDescription;

	// Input Assembly
	_dependencies->inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// Triangle from every 3 vertices without resuse
	_dependencies->inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	_dependencies->inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Viewport and Scissor
	_dependencies->viewport.x = 0.0f;
	_dependencies->viewport.y = 0.0f;
	_dependencies->viewport.width = static_cast<float>(swapchain().extent().width);
	_dependencies->viewport.height = static_cast<float>(swapchain().extent().height);
	_dependencies->viewport.minDepth = 0.0f;
	_dependencies->viewport.maxDepth = 1.0f;

	_dependencies->scissor.offset = { 0, 0 };
	_dependencies->scissor.extent = swapchain().extent();

	_dependencies->viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	_dependencies->viewportStateCreateInfo.viewportCount = 1;
	_dependencies->viewportStateCreateInfo.pViewports = &_dependencies->viewport;
	_dependencies->viewportStateCreateInfo.scissorCount = 1;
	_dependencies->viewportStateCreateInfo.pScissors = &_dependencies->scissor;

	_dependencies->rasterizerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	_dependencies->rasterizerStateCreateInfo.depthClampEnable = VK_FALSE;
	_dependencies->rasterizerStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	_dependencies->rasterizerStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	// Determines how to generate the fragment
	// FILL: area of the polygon, most common use.
	// LINE: line frame of the polygon.
	// Point: point of the polygon.
	_dependencies->rasterizerStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	// thicker than 1.0f requires enbale the wideLines GPU feature.
	_dependencies->rasterizerStateCreateInfo.lineWidth = 1.0f;
	_dependencies->rasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	_dependencies->rasterizerStateCreateInfo.depthBiasEnable = VK_FALSE;
	_dependencies->rasterizerStateCreateInfo.depthBiasConstantFactor = 0.0f;
	_dependencies->rasterizerStateCreateInfo.depthBiasClamp = 0.0f;
	_dependencies->rasterizerStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	// Multisampling, required enable GPU Feature.
	// TODO: forward rendering only.
	_dependencies->multisamplingStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	_dependencies->multisamplingStateCreateInfo.sampleShadingEnable = VK_FALSE;
	_dependencies->multisamplingStateCreateInfo.minSampleShading = 1.0f;
	_dependencies->multisamplingStateCreateInfo.pSampleMask = nullptr;
	_dependencies->multisamplingStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	_dependencies->multisamplingStateCreateInfo.alphaToOneEnable = VK_FALSE;
	_dependencies->multisamplingStateCreateInfo.sampleShadingEnable = VK_TRUE;
	// This value closer to one is  smoother.
	_dependencies->multisamplingStateCreateInfo.minSampleShading = 0.2f;

	// TODO: Depth and stencil CreateInfo here (Optional)

	// Color Blend
	// Blend this object and render target with some options.
	_dependencies->colorBlendAttachment.colorWriteMask = {
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT
	};

	// Color blending.
	_dependencies->colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	_dependencies->colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	_dependencies->colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	_dependencies->colorBlendStateCreateInfo.attachmentCount = 1;
	_dependencies->colorBlendStateCreateInfo.pAttachments = &_dependencies->colorBlendAttachment;
	_dependencies->colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	_dependencies->colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	_dependencies->colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	_dependencies->colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	// Depth and Stencil test.
	_dependencies->depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	_dependencies->depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	_dependencies->depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	// Keep closer's and discard further's.
	_dependencies->depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	_dependencies->depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	_dependencies->depthStencilCreateInfo.minDepthBounds = 0.0f;
	_dependencies->depthStencilCreateInfo.maxDepthBounds = 1.0f;
	_dependencies->depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	_dependencies->depthStencilCreateInfo.front = {};
	_dependencies->depthStencilCreateInfo.back = {};

	// Finally...
	info().sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info().stageCount = _dependencies->dynamicStateCreateInfo.dynamicStateCount;
	info().pStages = _dependencies->shaderStageCreateInfos.data();
	info().pVertexInputState = &_dependencies->vertexInputStateCreateInfo;
	info().pInputAssemblyState = &_dependencies->inputAssemblyStateCreateInfo;
	info().pViewportState = &_dependencies->viewportStateCreateInfo;
	info().pRasterizationState = &_dependencies->rasterizerStateCreateInfo;
	info().pMultisampleState = &_dependencies->multisamplingStateCreateInfo;
	info().pDepthStencilState = &_dependencies->depthStencilCreateInfo;
	info().pColorBlendState = &_dependencies->colorBlendStateCreateInfo;
	info().pDynamicState = &_dependencies->dynamicStateCreateInfo;
	info().renderPass = _renderPass;
	info().subpass = 0; 
	// You can deriveing a base pipeline in vulkan.
	// Pipeline derivative is less expensive than set up a new pipeline.
	// TODO: Material Instance derive a base pipeline.
	info().basePipelineHandle = VK_NULL_HANDLE;
	info().basePipelineIndex = -1;
}

void AfterglowPipeline::create() {
	// Delay binding for modify pipline layout after create info.
	info().layout = _pipelineLayout;

	if (vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create graphics pipeline.");
	}
	_dependencies.reset();
}

void AfterglowPipeline::verifyDependencies() const {
	if (!_dependencies) {
		throw runtimeError("Pipeline had been created, can not access dependencies.");
	}
}

inline VkPrimitiveTopology AfterglowPipeline::vulkanTopology(render::Topology topology) {
	switch (topology) {
	case render::Topology::PointList:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case render::Topology::LineList:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case render::Topology::LineStrip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case render::Topology::TriangleList:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case render::Topology::TriangleStrip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case render::Topology::PatchList:
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	default:
		throw std::runtime_error("[AfterglowPipeline] Unknown topology.");
	}
}

inline VkFormat AfterglowPipeline::vulkanVertexAttributeFormat(uint32_t attributeByteSize, uint32_t numAttributeComponents) {
	// half
	uint32_t componentByteSize = attributeByteSize / numAttributeComponents;
	if (componentByteSize == 2) {
		switch (numAttributeComponents) {
		case 1: return VK_FORMAT_R16_SFLOAT;
		case 2: return VK_FORMAT_R16G16_SFLOAT;
		case 3: return VK_FORMAT_R16G16B16_SFLOAT;
		case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		}
	}
	// float, int
	else if (componentByteSize == 4) {
		switch (numAttributeComponents) {
		case 1: return VK_FORMAT_R32_SFLOAT;
		case 2: return VK_FORMAT_R32G32_SFLOAT;
		case 3: return VK_FORMAT_R32G32B32_SFLOAT;
		case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
	}
	// double
	else if (componentByteSize == 8) {
		switch (numAttributeComponents) {
		case 1: return VK_FORMAT_R64_SFLOAT;
		case 2: return VK_FORMAT_R64G64_SFLOAT;
		case 3: return VK_FORMAT_R64G64B64_SFLOAT;
		case 4: return VK_FORMAT_R64G64B64A64_SFLOAT;
		}
	}
	DEBUG_TYPE_ERROR(
		AfterglowPipeline, 
		std::format(
			"The attributeByteSize: {} and/or numAttributeComponents: {} is not supported.", 
			attributeByteSize, numAttributeComponents
		)
	);
	return VK_FORMAT_UNDEFINED;
}
