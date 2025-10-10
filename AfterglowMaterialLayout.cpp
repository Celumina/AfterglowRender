#include "AfterglowMaterialLayout.h"

#include "AfterglowMaterialAsset.h"
#include "DebugUtilities.h"

AfterglowMaterialLayout::AfterglowMaterialLayout(AfterglowRenderPass& renderPass, const AfterglowMaterial& refMaterial) :
	_renderPass(renderPass), _material(refMaterial) {	
}

AfterglowMaterialLayout::DescriptorSetLayouts& AfterglowMaterialLayout::descriptorSetLayouts() {
	return _descriptorSetLayouts;
}

const AfterglowMaterialLayout::DescriptorSetLayouts& AfterglowMaterialLayout::descriptorSetLayouts() const {
	return _descriptorSetLayouts;
}

AfterglowMaterialLayout::RawDescriptorSetLayouts& AfterglowMaterialLayout::rawDescriptorSetLayouts() {
	return _rawDescriptorSetLayouts;
}

const AfterglowMaterialLayout::RawDescriptorSetLayouts& AfterglowMaterialLayout::rawDescriptorSetLayouts() const {
	return _rawDescriptorSetLayouts;
}

AfterglowDevice& AfterglowMaterialLayout::device() {
	return _renderPass.device();
}

AfterglowPipeline& AfterglowMaterialLayout::pipeline() {
	if (isComputeOnly()) {
		throw runtimeError("This material is compute only, graphics pipeline will never be created.");
	}
	return _pipeline;
}

AfterglowComputePipeline& AfterglowMaterialLayout::computePipeline() {
	verifyComputeTask();
	return _computeLayout->pipeline;
}

bool AfterglowMaterialLayout::shouldInitSSBOs() {
	return _computeLayout->shouldInitSSBOs[device().currentFrameIndex()];
}

bool AfterglowMaterialLayout::shouldInitSSBOsTrigger() {
	verifyComputeTask();
	// TODO: Here read only ssbo will also initialize twice, bad performance.
	bool shouldInitSSBOs = _computeLayout->shouldInitSSBOs[device().currentFrameIndex()];
	_computeLayout->shouldInitSSBOs[device().currentFrameIndex()] = false;
	return shouldInitSSBOs;
}

AfterglowComputePipeline::Array& AfterglowMaterialLayout::ssboInitComputePipelines() {
	verifyComputeTask();
	return _computeLayout->ssboInitPipelines;
}

AfterglowMaterial& AfterglowMaterialLayout::material() {
	return _material;
}

void AfterglowMaterialLayout::setMaterial(const AfterglowMaterial& material) {
	_material = material;
}

void AfterglowMaterialLayout::compileVertexShader(const std::string& shaderCode) {
	if (isComputeOnly()) {
		throw runtimeError("Failed to compute vertex shader due to this material is compute only.");
	}
	_vertexShader.recreate(
		device(), shader::Stage::Vertex, shaderCode, _material.vertexShaderPath()
	);
}

void AfterglowMaterialLayout::compileFragmentShader(const std::string& shaderCode) {
	if (isComputeOnly()) {
		throw runtimeError("Failed to compute fragment shader due to this material is compute only.");
	}
	_fragmentShader.recreate(
		device(), shader::Stage::Fragment, shaderCode, _material.fragmentShaderPath()
	);
}

void AfterglowMaterialLayout::compileComputeShader(const std::string& shaderCode) {
	verifyComputeTask();
	_computeLayout->shader.recreate(
		device(), shader::Stage::Compute, shaderCode, _material.computeTask().computeShaderPath()
	);
}

void AfterglowMaterialLayout::updateDescriptorSetLayouts(AfterglowDescriptorSetLayout& globalSetLayout, AfterglowDescriptorSetLayout& perObjectSetLayout) {
	// Material Sets:
	// Set 0: Global Set	(Managed in the MaterialManager instead of MaterialContext)
	// Set 1: PerObject Set 
	// Set 2: Material Vertex Stage Set
	// Set 3: Material Fragment Stage Set
	// Set 4: Material Shared Set

	// Compute Sets:
	// Set 5: Compute Set
	// Set 6: Compute Vertex Set
	// Set 7: Compute Fragment Set

	_descriptorSetLayouts.clear();

	// begin from shader::MaterialSetIndexBegin to ignore Global and PerObject Sets.
	uint32_t setIndexEnd = shader::MaterialSetIndexEnd;
	if (_material.hasComputeTask()) {
		setIndexEnd = shader::ComputeSetIndexEnd;
	}
	for (auto setIndex = shader::MaterialSetIndexBegin; setIndex < setIndexEnd; ++setIndex) {
		appendDescriptorSetLayout(static_cast<shader::Stage>(setIndex));
	}

	_rawDescriptorSetLayouts.clear();

	_rawDescriptorSetLayouts.push_back(globalSetLayout);
	_rawDescriptorSetLayouts.push_back(perObjectSetLayout);

	for (auto& [stage, setLayout] : _descriptorSetLayouts) {
		_rawDescriptorSetLayouts.push_back(setLayout);
	}
}

void AfterglowMaterialLayout::updatePipeline() {
	if (isComputeOnly()){
		return;
	}
	_pipeline.recreate(_renderPass, _material.domain(), _material.vertexTypeIndex());
	AfterglowPipeline& pipeline = _pipeline;

	if (_material.twoSided()) {
		// TODO: Filp normal if it is backface.
		pipeline.setCullMode(VK_CULL_MODE_NONE);
	}
	if (_material.domain() == render::Domain::Transparency) {
		pipeline.setBlendMode(AfterglowPipeline::BlendMode::Alpha);
	}
	// If use SSBO as vertex input.
	if (_material.hasComputeTask() && _material.computeTask().vertexInputSSBO()) {
		pipeline.assignVertex(_material.computeTask().vertexInputSSBO()->elementLayout);
	}

	pipeline.setTopology(_material.topology());
	fillPipelineLayout(pipeline.pipelineLayout());

	// Compile default shaders.
	if (!_vertexShader || !_fragmentShader) {
		auto materialAsset = AfterglowMaterialAsset(_material);
		auto& inputAttacmentInfos = _renderPass.subpassContext().inputAttachmentInfos();
		if (!_vertexShader) {
			compileVertexShader(materialAsset.generateShaderCode(shader::Stage::Vertex, inputAttacmentInfos));
		}
		if (!_fragmentShader) {
			compileFragmentShader(materialAsset.generateShaderCode(shader::Stage::Fragment, inputAttacmentInfos));
		}
	}
	pipeline.setVertexShader(_vertexShader);
	pipeline.setFragmentShader(_fragmentShader);
}

void AfterglowMaterialLayout::updateComputePipeline() {
	verifyComputeTask();

	_computeLayout->pipeline.recreate(device());
	AfterglowComputePipeline& computePipeline = _computeLayout->pipeline;
	
	fillPipelineLayout(computePipeline.pipelineLayout());

	std::unique_ptr<AfterglowMaterialAsset> materialAsset;
	if (!_computeLayout->shader) {
		materialAsset = std::make_unique<AfterglowMaterialAsset>(_material);
		// Compute shader does not use global texture (input attachments)
		compileComputeShader(materialAsset->generateShaderCode(shader::Stage::Compute));
	}

	computePipeline.setComputeShader(_computeLayout->shader);

	// SSBO Initialization compute pipelines
	auto computeShaderInitSSBOInfoRefs = _material.computeTask().computeShaderInitSSBOInfos();
	if (computeShaderInitSSBOInfoRefs.empty()) {
		return;
	}
	if (!materialAsset) {
		materialAsset = std::make_unique<AfterglowMaterialAsset>(_material);
	}
	std::fill(_computeLayout->shouldInitSSBOs.begin(), _computeLayout->shouldInitSSBOs.end(), true);

	_computeLayout->ssboInitPipelines.clear();
	_computeLayout->ssboInitShaders.clear();

	_computeLayout->ssboInitPipelines.resize(computeShaderInitSSBOInfoRefs.size());
	_computeLayout->ssboInitShaders.resize(computeShaderInitSSBOInfoRefs.size());
	for (uint32_t index = 0; index < computeShaderInitSSBOInfoRefs.size(); ++index) {
		auto& ssboInitPipeline = _computeLayout->ssboInitPipelines[index];
		auto& ssboInitShader = _computeLayout->ssboInitShaders[index];
		auto& ssboInfo = computeShaderInitSSBOInfoRefs[index];
		
		AfterglowShaderAsset shaderAsset{ ssboInfo->initResource };

		ssboInitPipeline.recreate(device());
		fillPipelineLayout((*ssboInitPipeline).pipelineLayout());

		// Handle init compute shader compile error.
		try {
			ssboInitShader.recreate(
				device(),
				shader::Stage::Compute,
				materialAsset->shaderDeclaration(shader::Stage::Compute) + shaderAsset.code(),
				_material.computeTask().computeShaderPath()
			);
		}
		catch (std::runtime_error& error) {
			DEBUG_CLASS_ERROR(std::format("Failed to compile SSBO initializer compute shader: {}", ssboInfo->initResource));
			std::fill(_computeLayout->shouldInitSSBOs.begin(), _computeLayout->shouldInitSSBOs.end(), false);
			break;
		}

		(*ssboInitPipeline).setComputeShader(ssboInitShader);
		// Comment out the code to reinitialize buffers when other shaders are modified.
		// ssboInfo->initMode = compute::SSBOInitMode::Zero;
	}
}

void AfterglowMaterialLayout::updatePipelines() {
	updatePipeline();
	if (_material.hasComputeTask()) {
		updateComputePipeline();
	}
}

void AfterglowMaterialLayout::appendDescriptorSetLayout(shader::Stage stage) {
	auto vulkanStage = vulkanShaderStage(stage);

	_descriptorSetLayouts[stage].recreate(device());
	AfterglowDescriptorSetLayout& setLayout = _descriptorSetLayouts[stage];

	// Stage uniforms binding.
	setLayout.appendBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vulkanStage);

	// Stage texture bindings.
	// TODO: Sepreated texture and sampler binding for shared sampler.
	for (const auto& textureParam : _material.textures()[stage]) {
		setLayout.appendBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vulkanStage);
	}

	if (!_material.hasComputeTask()) {
		return;
	}
	// Storage buffers, behind these texture binds.
	const auto& computeTask = _material.computeTask();
	for (auto& ssboInfo : computeTask.ssboInfos()) {
		if (ssboInfo.stage != stage) {
			continue;
		}
		uint32_t numSSBOs = computeTask.numSSBOs(ssboInfo);
		for (uint32_t index = 0; index < numSSBOs; ++index) {
			setLayout.appendBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vulkanStage);
		}
	}
}

inline void AfterglowMaterialLayout::fillPipelineLayout(AfterglowPipelineLayout& pipelineLayout) {
	// Size of meshlSetLayout + materialSetLayouts
	pipelineLayout->setLayoutCount = static_cast<uint32_t>(_descriptorSetLayouts.size()) + shader::MaterialSetIndexBegin;
	pipelineLayout->pSetLayouts = _rawDescriptorSetLayouts.data();
}

inline bool AfterglowMaterialLayout::isComputeOnly() {
	return _material.hasComputeTask() && _material.computeTask().isComputeOnly();
}

inline std::runtime_error AfterglowMaterialLayout::runtimeError(const char* text) {
	DEBUG_CLASS_FATAL(text);
	return std::runtime_error(text);
}

inline void AfterglowMaterialLayout::verifyComputeTask() {
	if (!_material.hasComputeTask()) {
		throw runtimeError("This mateiral have not compute task, make sure it was initialized.");
	}
	if (!_computeLayout) {
		_computeLayout = std::make_unique<ComputeLayout>();
	}
}

VkShaderStageFlags AfterglowMaterialLayout::vulkanShaderStage(shader::Stage stage) {
	switch (stage) {
	case shader::Stage::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case shader::Stage::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case shader::Stage::Shared:
		return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	case shader::Stage::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	case shader::Stage::ComputeVertex:
		return VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	case shader::Stage::ComputeFragment:
		return VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	default:
		DEBUG_TYPE_ERROR(AfterglowMaterialLayout, "Unknown parameter stage.");
		throw std::runtime_error("[AfterglowMaterialLayout] Unknown parameter stage.");
	}
}