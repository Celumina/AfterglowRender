#include "AfterglowCommandManager.h"
#include <backends/imgui_impl_vulkan.h>

#include "AfterglowDrawCommandBuffer.h"
#include "AfterglowComputeCommandBuffer.h"
#include "AfterglowFramebuffer.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowIndexBuffer.h"
#include "AfterglowComputePipeline.h"
#include "AfterglowPassManager.h"
#include "ComputeDefinitions.h"
#include "AfterglowComputeTask.h"

struct AfterglowCommandManager::Impl {
	template<typename PipelineType, typename RecordInfoType>
	struct RecordDependency {
		PipelineType* pipeline;
		AfterglowDescriptorSetReferences* setReferences;
		RecordInfoType recordInfo;
	};

	struct PreparingComputeDependency {
		AfterglowComputePipeline* pipeline;
		AfterglowComputeCommandBuffer::RecordInfo recordInfo;
	};

	struct ComputeRecordDependency : public RecordDependency<AfterglowComputePipeline, AfterglowComputeCommandBuffer::RecordInfo> {
		ComputeRecordDependency(
			AfterglowComputePipeline* inPipeline, 
			AfterglowDescriptorSetReferences* inSetReferences, 
			AfterglowComputeCommandBuffer::RecordInfo&& inRecordInfo, 
			std::unique_ptr<PreparingComputeDependency> inPreparing = nullptr
		) : RecordDependency(inPipeline, inSetReferences, std::move(inRecordInfo)), preparing(std::move(inPreparing)) {}

		std::unique_ptr<PreparingComputeDependency> preparing;
	};

	using DrawRecordDependency = RecordDependency<AfterglowPipeline, AfterglowDrawCommandBuffer::RecordInfo>;

	using PerSubpassRecordDependencies = std::vector<DrawRecordDependency>;
	using PerPassRecordDependencies = std::vector<PerSubpassRecordDependencies>;
	using DrawRecordDependencies = std::array<std::unique_ptr<PerPassRecordDependencies>, util::EnumValue(render::Domain::EnumCount)>;
	using ComputeRecordDependencies = std::vector<ComputeRecordDependency>;

	Impl(AfterglowPassManager& inPassManager);

	inline AfterglowDrawCommandBuffer::RecordInfo* aquireDrawRecordInfo(
		AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs
	);

	inline void drawCustomPassSets(uint32_t domainIndex);
	inline void applyPassDrawCommands(AfterglowPassInterface& pass, PerPassRecordDependencies& passRecordInfos, int32_t imageIndex);
	inline void applyDrawCommands(int32_t imageIndex);
	inline void applyComputeCommands();

	// @brief: If material is defined in a custom pass, return false. (don't draw them in the command manager directly)
	inline bool verifyMaterialDomain(AfterglowMaterialResource& matResource) const noexcept;

	AfterglowPassManager& passManager;
	AfterglowCommandPool commandPool;

	AfterglowDrawCommandBuffer drawCommandBuffer;
	DrawRecordDependencies drawRecordInfos;
	// std::array<bool, util::EnumValue(render::Domain::EnumCount)> domainImplementationFlags = { false };

	AfterglowComputeCommandBuffer computeCommandBuffer;
	ComputeRecordDependencies computeRecordInfos;

	ImDrawData* uiDrawData = nullptr;
};

AfterglowCommandManager::Impl::Impl(AfterglowPassManager& inPassManager) :
	passManager(inPassManager),
	commandPool(passManager.device()),
	drawCommandBuffer(commandPool),
	computeCommandBuffer(commandPool) {
}

inline AfterglowDrawCommandBuffer::RecordInfo* AfterglowCommandManager::Impl::aquireDrawRecordInfo(
	AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs
) {
	if (!verifyMaterialDomain(matResource)) {
		return nullptr;
	}
	auto& matLayout = matResource.materialLayout();
	auto& pipeline = matLayout.pipeline();
	render::Domain domain = matLayout.material().domain();
	uint32_t subpassIndex = matLayout.subpassIndex();
	
	if (!drawRecordInfos[util::EnumValue(domain)]) {
		DEBUG_CLASS_WARNING("Failed to record the draw, due to this material domain is not implemented in PassManager.");
		return nullptr;
	}
	auto& subpassDrawRecordInfos = (*drawRecordInfos[util::EnumValue(domain)])[subpassIndex];
	return &subpassDrawRecordInfos.emplace_back(&pipeline, &setRefs, AfterglowDrawCommandBuffer::RecordInfo{}).recordInfo;
}

inline void AfterglowCommandManager::Impl::drawCustomPassSets(uint32_t domainIndex) {
	auto* domainCustomPassSets = passManager.domainCustomPassSets(render::Domain(domainIndex));
	if (!domainCustomPassSets) {
		return;
	}
	for (auto& customPassSet : *domainCustomPassSets) {
		customPassSet->submitCommands(drawCommandBuffer);
	}
}

inline void AfterglowCommandManager::Impl::applyPassDrawCommands(AfterglowPassInterface& pass, PerPassRecordDependencies& passRecordInfos, int32_t imageIndex) {
	drawCommandBuffer.beginRenderPass(pass, imageIndex);

	bool isFirstSubpass = true;
	for (auto& subpassRecordInfos : passRecordInfos) {
		if (!isFirstSubpass) {
			drawCommandBuffer.nextSubpass();
		}
		isFirstSubpass = false;
		for (auto& [pipeline, setRefs, recordInfo] : subpassRecordInfos) {
			drawCommandBuffer.setupPipeline(*pipeline);
			drawCommandBuffer.setupDescriptorSets(*setRefs);
			drawCommandBuffer.draw(recordInfo);
		}
	}

	// Draw UI
	if (passManager.isFinalPass(pass)) {
		ImGui_ImplVulkan_RenderDrawData(uiDrawData, drawCommandBuffer.current());
	}

	drawCommandBuffer.endRenderPass();
	drawCommandBuffer.barrier(pass);
}

inline void AfterglowCommandManager::Impl::applyDrawCommands(int32_t imageIndex) {
	drawCommandBuffer.reset(commandPool.device().currentFrameIndex());
	drawCommandBuffer.beginRecord();

	for (uint32_t index = 0; index < drawRecordInfos.size(); ++index) {
		auto& passRecordInfos = drawRecordInfos[index];
		if (!passRecordInfos) {
			continue;
		}
		// Apply per pass commands.
		auto* pass = passManager.findPass(render::Domain(index));
		applyPassDrawCommands(*pass, *passRecordInfos, imageIndex);

		// Apply custom pass set commands only if input domain exists.
		drawCustomPassSets(index);

		// Clear record infos.
		for (auto& subpassRecordInfos : *passRecordInfos) {
			subpassRecordInfos.clear();
		}
	}

	drawCommandBuffer.endRecord();
}

inline void AfterglowCommandManager::Impl::applyComputeCommands() {
	computeCommandBuffer.reset(commandPool.device().currentFrameIndex());
	computeCommandBuffer.beginRecord();

	for (auto& info : computeRecordInfos) {
		// Update preparing computeinfo, indirect etc..
		if (info.preparing) {
			// They both keep same descriptorSets, so setup once only.
			computeCommandBuffer.setupDescriptorSets(*info.setReferences, info.pipeline->pipelineLayout());
			computeCommandBuffer.setupPipeline(*info.preparing->pipeline);
			computeCommandBuffer.dispatch(info.preparing->recordInfo);
			computeCommandBuffer.barrier();
			computeCommandBuffer.setupPipeline(*info.pipeline);
		}
		else {
			computeCommandBuffer.setupPipeline(*info.pipeline);
			computeCommandBuffer.setupDescriptorSets(*info.setReferences);
		}
		// Primary compute commands.
		computeCommandBuffer.dispatch(info.recordInfo);
	}
	computeRecordInfos.clear();

	computeCommandBuffer.endRecord();
}

inline bool AfterglowCommandManager::Impl::verifyMaterialDomain(AfterglowMaterialResource& matResource) const noexcept {
	if (matResource.materialLayout().material().customPassName().empty()) {
		return true;
	}
	return false;
}

AfterglowCommandManager::AfterglowCommandManager(AfterglowPassManager& passManager) : 
	_impl(std::make_unique<Impl>(passManager)) {
}

AfterglowCommandPool& AfterglowCommandManager::commandPool() noexcept {
	return _impl->commandPool;
}

VkCommandBuffer* AfterglowCommandManager::drawCommandBuffers() noexcept {
	return &_impl->drawCommandBuffer.current();
}

VkCommandBuffer* AfterglowCommandManager::computeCommandBuffers() noexcept {
	return &_impl->computeCommandBuffer.current();
}

bool AfterglowCommandManager::recordDraw(
	AfterglowMaterialResource& matResource, 
	AfterglowDescriptorSetReferences& setRefs,
	AfterglowVertexBufferHandle& vertexBufferHandle,
	AfterglowIndexBuffer* indexBuffer,
	AfterglowStorageBuffer* indirectBuffer,
	uint32_t instanceCount
) {
	auto* recordInfo = _impl->aquireDrawRecordInfo(matResource, setRefs);
	if (!recordInfo) {
		return false;
	}
	
	recordInfo->vertexBuffer = vertexBufferHandle.buffer;
	// TODO: Same indexBuffer for different vertexBuffers
	recordInfo->vertexCount = vertexBufferHandle.vertexCount;
	recordInfo->instanceCount = instanceCount;

	if (indexBuffer) {
		recordInfo->indexBuffer = *indexBuffer;
		recordInfo->indexCount = indexBuffer->indexCount();
	}

	if (indirectBuffer) {
		recordInfo->indirectBuffer = *indirectBuffer;
	}

	return true;
}

bool AfterglowCommandManager::recordDraw(
	AfterglowMaterialResource& matResource,
	AfterglowDescriptorSetReferences& setRefs,
	const AfterglowSSBOInfo& vertexSSBOInfo,
	AfterglowStorageBuffer& vertexData,
	const AfterglowSSBOInfo* indexSSBOInfo,
	AfterglowStorageBuffer* indexData, 
	AfterglowStorageBuffer* indirectBuffer
) {
	auto* recordInfo = _impl->aquireDrawRecordInfo(matResource, setRefs);
	if (!recordInfo) {
		return false;
	}
	recordInfo->vertexBuffer = vertexData;
	recordInfo->vertexCount = static_cast<uint32_t>(vertexSSBOInfo.numElements());
	recordInfo->instanceCount = 1;

	if (indexSSBOInfo && indexData) {
		recordInfo->indexBuffer = *indexData;
		recordInfo->indexCount = static_cast<uint32_t>(indexSSBOInfo->numUnpackedIndices());
	}

	if (indirectBuffer) {
		recordInfo->indirectBuffer = *indirectBuffer;
	}

	return true;
}

// TODO: GPU Instancing
void AfterglowCommandManager::applyDrawCommands(int32_t imageIndex) {
	_impl->applyDrawCommands(imageIndex);
}

void AfterglowCommandManager::recordCompute(AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs) {
	auto frameIndex = _impl->commandPool.device().currentFrameIndex();
	auto& matLayout = matResource.materialLayout();
	auto& computeTask = matLayout.material().computeTask();

	// SSBO initialization from compute shader.
	// Order dependency buffer.
	if (computeTask.dispatchStatus(frameIndex) == AfterglowComputeTask::DispatchStatus::None) {
		auto& ssboInitComputePipelines = matLayout.ssboInitComputePipelines();
		for (AfterglowComputePipeline& pipeline : ssboInitComputePipelines) {
			_impl->computeRecordInfos.emplace_back(
				&pipeline, 
				&setRefs, 
				AfterglowComputeCommandBuffer::RecordInfo{ computeTask.dispatchGroup() }
			);

		}
		computeTask.setDispatchStatus(frameIndex, AfterglowComputeTask::DispatchStatus::Initialized);
	}
	// Dispatch regular comptue shader.
	else {
		// @note for {}
		auto& info = _impl->computeRecordInfos.emplace_back(
			&matLayout.computePipeline(),
			&setRefs,
			AfterglowComputeCommandBuffer::RecordInfo{ computeTask.dispatchGroup() }
		);
		// Find indirect buffer reset pipeline.
		auto* indirectResetPipeline = matLayout.indirectResetPipeline();
		if (indirectResetPipeline) {
			info.preparing = std::make_unique<Impl::PreparingComputeDependency>(
				indirectResetPipeline, AfterglowComputeCommandBuffer::RecordInfo{ {.x = 1, .y = 1, .z = 1} }
			);
		}
	}
}

void AfterglowCommandManager::applyComputeCommands() {
	_impl->applyComputeCommands();
}

void AfterglowCommandManager::recordUIDraw(ImDrawData* uiDrawData) {
	_impl->uiDrawData = uiDrawData;
}

void AfterglowCommandManager::installFixedPasses() {
	for (uint32_t index = 0; index < util::EnumValue(render::Domain::EnumCount); ++index) {
		auto* pass = _impl->passManager.findPass(render::Domain(index));
		if (pass) {
			_impl->drawRecordInfos[index] = std::make_unique<Impl::PerPassRecordDependencies>();
			_impl->drawRecordInfos[index]->resize(pass->subpassContext().subpassCount());
		}
		else {
			_impl->drawRecordInfos[index].reset();
		}
	}
}
