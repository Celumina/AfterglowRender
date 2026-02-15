#include "AfterglowBloomPassSet.h"
#include <format>
#include <stdexcept>
#include <gcem.hpp>
#include "AfterglowSystemUtilities.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowMeshResource.h"
#include "AfterglowPassManager.h"
#include "AfterglowDownSamplingPass.h"
#include "AfterglowBloomPass.h"
#include "ExceptionUtilities.h"

struct AfterglowBloomPassSet::Impl {
	constexpr static uint32_t downSamplingCount = 6;
	
	static inline std::string vertexShaderPath = "Shaders/PostProcess_VS.hlsl";
	static inline std::string downSamplingFSPath = "Shaders/DownSampling_FS.hlsl";
	static inline std::string horizontalBlurFSPath = "Shaders/HorizontalBlur_FS.hlsl";
	static inline std::string verticalBlurCombinationFSPath = "Shaders/VerticalBlurCombination_FS.hlsl";

	static inline std::array<float, downSamplingCount> defaultBloomIntensities = {
		0.175f, 0.07f, 0.06f, 0.03f, 0.03f, 0.02f
	};

	inline void buildPasses();
	inline void submitCommands(AfterglowDrawCommandBuffer& drawCommandBuffer);

	void buildDownsamplingPasses();
	void buildBloomPasses();

	void initDrawRecordInfo(AfterglowMeshResource& inMeshResource);

	inline AfterglowMaterial& createMaterial(
		AfterglowPassInterface& pass, 
		const std::string& materialName, 
		const std::string& fragmentShaderPath, 
		const std::string& subpassName = ""
	);

	inline void drawMaterial(AfterglowDrawCommandBuffer& drawCommandBuffer, const std::string& materialName, bool switchPipelineState);

	AfterglowBloomPassSet* passSet;

	// External infos
	const AfterglowSystemUtilities* sysUtils = nullptr;
	AfterglowPassInterface* importPass = nullptr;
	AfterglowPassInterface* exportPass = nullptr;

	bool materialUniformSubmitted = false;

	bool enabled = true;
	VkFormat colorFormat = VK_FORMAT_UNDEFINED;
	
	std::vector<std::string> materialNames;

	AfterglowDrawCommandBuffer::RecordInfo recordInfo{};
	AfterglowMeshResource* meshResource = nullptr;
};

inline void AfterglowBloomPassSet::Impl::buildPasses() {
	buildDownsamplingPasses();
	buildBloomPasses();

	// Register export attachment.
	auto* exportBloomPass = passSet->passes().back().get();
	uint32_t importBloomAttachmentIndex = exportPass->subpassContext().appendAttachment(
		AfterglowSubpassContext::transferAttachment(exportBloomPass->colorFormat(), AfterglowSubpassContext::PassUsage::Import)
	);
	exportPass->recordImportAttachments(
		exportBloomPass,
		AfterglowPassInterface::ColorAttachment::Color,
		importBloomAttachmentIndex
	);
}

inline void AfterglowBloomPassSet::Impl::submitCommands(AfterglowDrawCommandBuffer& drawCommandBuffer) {
	if (!enabled) {
		return;
	}
	// TODO: Multi-threads for submission.
	//DEBUG_COST_BEGIN("COMMIT");
	uint32_t materialIndex = 0;
	for (auto& pass : passSet->passes()) {
		// Switch pipeline state only if actually needed.
		// 0: first downsampling; >= downSamplingCount: horizontal and vertical blur.
		bool switchPipelineState = false;
		if (materialIndex == 0 || materialIndex >= downSamplingCount) {
			switchPipelineState = true;
		}
		drawCommandBuffer.beginRenderPass(*pass);
		drawMaterial(drawCommandBuffer, materialNames[materialIndex], switchPipelineState);

		++materialIndex;
		if (materialIndex > downSamplingCount) {
			drawCommandBuffer.nextSubpass();
			drawMaterial(drawCommandBuffer, materialNames[materialIndex], switchPipelineState);
			++materialIndex;
		}

		drawCommandBuffer.endRenderPass();
		drawCommandBuffer.barrier(*pass);
	}
	materialUniformSubmitted = true;
	//DEBUG_COST_END;
}

void AfterglowBloomPassSet::Impl::buildDownsamplingPasses() {
	// Down Sampling: 1/2 to 1/64 extent
	float currentScale = 0.5f;
	auto* currentPass = importPass;
	for (int32_t index = 0; index < downSamplingCount; ++index) {
		currentPass = passSet->passes().emplace_back(std::make_unique<AfterglowDownSamplingPass>(
			*currentPass, currentScale, index
		)).get();

		// Create pass material.
		createMaterial(*currentPass, std::string(currentPass->passName()), downSamplingFSPath);

		// Next attachment extent
		currentScale *= 0.5;
	}
}

void AfterglowBloomPassSet::Impl::buildBloomPasses() {
	// BloomX and BloomY combination: 1/64 to 1/2
	float currentScale = 1.0f / gcem::pow(2, downSamplingCount);
	for (int32_t index = downSamplingCount - 1; index >= 0; --index) {
		auto& downSamplingPass = *passSet->passes()[index];
		auto& currentPass = passSet->passes().emplace_back(std::make_unique<AfterglowBloomPass>(
			*passSet->passes().back(), currentScale, index
		));

		auto& currentBloomPass = reinterpret_cast<AfterglowBloomPass&>(*currentPass);
		uint32_t downSamplingColorAttachmentIndex = currentBloomPass.subpassContext().appendAttachment(
			AfterglowSubpassContext::transferAttachment(currentBloomPass.colorFormat(), AfterglowSubpassContext::PassUsage::Import)
		);
		currentBloomPass.recordImportAttachments(
			&downSamplingPass,
			AfterglowPassInterface::ColorAttachment::Color, 
			downSamplingColorAttachmentIndex
		);

		// Pass materials
		createMaterial(
			currentBloomPass, 
			std::format("{}{}", currentBloomPass.horizontalBlurSubpassName(), index), 
			horizontalBlurFSPath, 
			currentBloomPass.horizontalBlurSubpassName()
		);
		auto verticalBlurCombinationMaterialName = std::format("{}{}", currentBloomPass.verticalBlurCombinationSubpassName(), index);
		createMaterial(
			currentBloomPass, 
			verticalBlurCombinationMaterialName,
			verticalBlurCombinationFSPath, 
			currentBloomPass.verticalBlurCombinationSubpassName()
		);
		if (index == downSamplingCount - 1) {
			currentBloomPass.importAttachments()[0].attachmentName = currentBloomPass.combinedTextureName();
			sysUtils->materialInstance(verticalBlurCombinationMaterialName)->setScalar(
			shader::Stage::Fragment, ParamName::useCombinedTexture, 0.0f
			);
		}

		// Next scale
		currentScale *= 2.0;
	}
}

inline AfterglowMaterial& AfterglowBloomPassSet::Impl::createMaterial(
	AfterglowPassInterface& pass,
	const std::string& materialName,
	const std::string& fragmentShaderPath,
	const std::string& subpassName
) {
	// TODO: Shared descriptor for materials
	materialNames.emplace_back(materialName);
	auto& material = sysUtils->createMaterial(materialName);
	material.setScalar(shader::Stage::Fragment, ParamName::resolutionScale, pass.scale().x);
	material.setScalar(shader::Stage::Fragment, ParamName::resolutionInvScale, 1.0f / pass.scale().x);
	material.setScalar(shader::Stage::Fragment, ParamName::useCombinedTexture, 1.0f);
	uint32_t sequenceID = downSamplingCount - static_cast<uint32_t>(std::log2(1.0f / pass.scale().x));
	float bloomIntensity = defaultBloomIntensities[sequenceID];
	material.setVector(
		shader::Stage::Fragment, 
		ParamName::bloomIntensity, 
		{ bloomIntensity, bloomIntensity, bloomIntensity, 0.0f }
	);
	material.setVertexShader(vertexShaderPath);
	material.setFragmentShader(fragmentShaderPath);
	// TOOD: Gosh, It's troublesome, find a way to set this value automatically.
	material.setVertexTypeIndex(util::TypeIndex<vert::VertexPT0>());

	material.setCustomPass(std::string(pass.passName()));
	material.setSubpass(subpassName);

	sysUtils->submitMaterial(materialName);
	sysUtils->materialSubmitMeshUniform(materialName, meshResource->meshUniform());

	return material;
}

inline void AfterglowBloomPassSet::Impl::drawMaterial(AfterglowDrawCommandBuffer& drawCommandBuffer, const std::string& materialName, bool switchPipelineState) {
	// Mesh Uniform seems not require to update every frame
	if (!materialUniformSubmitted) {
		sysUtils->materialSubmitMeshUniform(materialName, meshResource->meshUniform());
	}

	if (switchPipelineState) {
		auto* matResource = sysUtils->materialResource(materialName);
		auto* setRefs = sysUtils->materialDescriptorSetReferences(materialName, meshResource->meshUniform());
		if (matResource && setRefs) {
			drawCommandBuffer.setupPipeline(matResource->materialLayout().pipeline());
			drawCommandBuffer.setupDescriptorSets(*setRefs);
		}
		else {
			DEBUG_CLASS_ERROR("Material resource or set references are not found.");
		}
	}
	drawCommandBuffer.draw(recordInfo);
}

AfterglowBloomPassSet::AfterglowBloomPassSet(
	const AfterglowSystemUtilities& sysUtils,
	AfterglowMeshResource& meshResource,
	render::Domain importDomain,
	render::Domain exportDomain
) : _impl(std::make_unique<Impl>()) {
	// Procedural model: 
	// downsampling                        (To DownSamplingAttachment) -> 
	// horizontalBlur                      (To BloomXAttachment) -> 
	// verticalBlur and CombinePrevLevels  (To DownSamplingAttachment)
	_impl->passSet = this;
	_impl->sysUtils = &sysUtils;
	_impl->importPass = sysUtils.passManager().findPass(importDomain);
	_impl->exportPass = sysUtils.passManager().findPass(exportDomain);
	if (!_impl->importPass || !_impl->exportPass) {
		EXCEPT_CLASS_INVALID_ARG("The import domain or the export domain are not exist.");
	}

	_impl->colorFormat = _impl->importPass->colorFormat();

	_impl->initDrawRecordInfo(meshResource);
	_impl->buildPasses();
}

AfterglowBloomPassSet::~AfterglowBloomPassSet() {
}

void AfterglowBloomPassSet::enable() noexcept {
	_impl->enabled = true;
}

void AfterglowBloomPassSet::disable() noexcept {
	_impl->enabled = false;
}

//void AfterglowBloomPassSet::updateMaterials() {
//	bool submitSuccessfully = true;
//	for (auto& materialName : _impl->materialNames) {
//		submitSuccessfully &= _impl->sysUtils->submitMaterial(materialName);
//	}
//	if (!submitSuccessfully) {
//		DEBUG_CLASS_ERROR("Some materials were failed to sumit.");
//	}
//}

const std::string* AfterglowBloomPassSet::downSamplingMaterialName(uint32_t index) const noexcept {
	// materialNames 0 ~ downSampleCount
	if (index >= _impl->downSamplingCount) {
		return nullptr;
	}
	return &_impl->materialNames[index];
}

const std::string* AfterglowBloomPassSet::horizontalBlurMaterialName(uint32_t index) const noexcept {
	// Intersect with verticalBlurCombinationMaterial Names
	if (index >= _impl->downSamplingCount) {
		return nullptr;
	}
	return &_impl->materialNames[static_cast<size_t>(Impl::downSamplingCount + index * 2 - 1)];
}

const std::string* AfterglowBloomPassSet::verticalBlurCombinationMaterialInstanceName(uint32_t index) const noexcept {
	if (index >= _impl->downSamplingCount) {
		return nullptr;
	}
	return &_impl->materialNames[static_cast<size_t>(Impl::downSamplingCount + index * 2)];
}

void AfterglowBloomPassSet::submitCommands(AfterglowDrawCommandBuffer& drawCommandBuffer) {
	_impl->submitCommands(drawCommandBuffer);
}

void AfterglowBloomPassSet::Impl::initDrawRecordInfo(AfterglowMeshResource& inMeshResource) {
	meshResource = &inMeshResource;
	recordInfo.indexBuffer = meshResource->indexBuffers()[0];
	recordInfo.indexCount = (*meshResource->indexBuffers()[0]).indexCount();
	recordInfo.vertexBuffer = meshResource->vertexBufferHandles()[0].buffer;
	recordInfo.vertexCount = meshResource->vertexBufferHandles()[0].vertexCount;
}
