#include "AfterglowCommandManager.h"
#include <backends/imgui_impl_vulkan.h>

#include "AfterglowDrawCommandBuffer.h"
#include "AfterglowComputeCommandBuffer.h"
#include "AfterglowFramebuffer.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowIndexBuffer.h"

#include "AfterglowComputePipeline.h"
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

	using DrawRecordDependencies = std::array<std::vector<RecordDependency<AfterglowPipeline, AfterglowDrawCommandBuffer::RecordInfo>>, util::EnumValue(render::Domain::EnumCount)>;
	using ComputeRecordDependencies = std::vector<ComputeRecordDependency>;

	Impl(AfterglowRenderPass& renderPass);

	inline AfterglowDrawCommandBuffer::RecordInfo* aquireDrawRecordInfo(
		AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs
	);

	/**
	* @brief: Init ComputePipelineRecordInfoContext if it's not exists.
	* @return: pipelineIndex.
	*/ 
	// inline uint32_t computePipelineIndex(AfterglowComputePipeline& pipeline);

	inline void applyDrawCommands(AfterglowFramebuffer& frameBuffer);
	inline void applyComputeCommands();

	AfterglowRenderPass& renderPass;
	AfterglowCommandPool commandPool;

	AfterglowDrawCommandBuffer drawCommandBuffer;
	DrawRecordDependencies drawRecordInfos;
	std::array<bool, util::EnumValue(render::Domain::EnumCount)> domainImplementationFlags = { false };

	AfterglowComputeCommandBuffer computeCommandBuffer;
	ComputeRecordDependencies computeRecordInfos;

	ImDrawData* uiDrawData = nullptr;
};

AfterglowCommandManager::Impl::Impl(AfterglowRenderPass& renderPass) :
	renderPass(renderPass),
	commandPool(renderPass.device()),
	drawCommandBuffer(commandPool),
	computeCommandBuffer(commandPool) {
	for (uint32_t index = 0; index < util::EnumValue(render::Domain::EnumCount); ++index) {
		if (renderPass.subpassContext().subpassExists(static_cast<render::Domain>(index))) {
			domainImplementationFlags[index] = true;
			// _drawRecordInfos[static_cast<render::Domain>(index)] = {};
		}
	}
}

inline AfterglowDrawCommandBuffer::RecordInfo* AfterglowCommandManager::Impl::aquireDrawRecordInfo(
	AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs) {
	auto& pipeline = matResource.materialLayout().pipeline();
	auto domain = matResource.materialLayout().material().domain();
	if (!renderPass.subpassContext().subpassExists(domain)) {
		DEBUG_CLASS_WARNING("Failed to record the draw, due to this material domain is not declared in RenderPass.");
		return nullptr;
	}
	// return &drawRecordInfos[util::EnumValue(domain)].descSetRecordInfos(pipeline)[&setRefs].emplace_back();
	return &drawRecordInfos[util::EnumValue(domain)].emplace_back(&pipeline, &setRefs, AfterglowDrawCommandBuffer::RecordInfo{}).recordInfo;
}

inline void AfterglowCommandManager::Impl::applyDrawCommands(AfterglowFramebuffer& frameBuffer) {
	drawCommandBuffer.reset(commandPool.device().currentFrameIndex());

	auto& subpassContext = renderPass.subpassContext();

	VkExtent2D extent = renderPass.swapchain().extent();
	AfterglowDrawCommandBuffer::BeginInfo beginInfo;
	beginInfo.renderPassBegin.renderPass = renderPass;
	beginInfo.renderPassBegin.framebuffer = frameBuffer;
	beginInfo.renderPassBegin.renderArea.extent = extent;
	beginInfo.renderPassBegin.clearValueCount = subpassContext.clearValueCount();
	beginInfo.renderPassBegin.pClearValues = subpassContext.clearValues().data();
	beginInfo.viewport.width = static_cast<float>(extent.width);
	beginInfo.viewport.height = static_cast<float>(extent.height);
	beginInfo.scissor.extent = extent;
	drawCommandBuffer.beginRecord(beginInfo);

	bool isFirstDomain = true;
	for (uint32_t domainIndex = 0; domainIndex < drawRecordInfos.size(); ++domainIndex) {
		if (!domainImplementationFlags[domainIndex]) {
			continue;
		}
		if (!isFirstDomain) {
			drawCommandBuffer.nextSubpassRecord();
		}
		isFirstDomain = false;

		for (auto& [pipeline, setRefs, recordInfo] : drawRecordInfos[domainIndex]) {
			drawCommandBuffer.setupPipeline(*pipeline);
			drawCommandBuffer.setupDescriptorSets(*setRefs);
			drawCommandBuffer.dispatch(recordInfo);
		}
	}
	// Draw UI
	ImGui_ImplVulkan_RenderDrawData(uiDrawData, drawCommandBuffer.current());

	drawCommandBuffer.endRecord();

	for (auto& elem : drawRecordInfos) {
		elem.clear();
	}
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

AfterglowCommandManager::AfterglowCommandManager(AfterglowRenderPass& renderPass) : _impl(std::make_unique<Impl>(renderPass)) {
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
	uint32_t instanceCount) {

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
	AfterglowStorageBuffer* indirectBuffer) {

	auto* recordInfo = _impl->aquireDrawRecordInfo(matResource, setRefs);
	if (!recordInfo) {
		return false;
	}
	recordInfo->vertexBuffer = vertexData;
	recordInfo->vertexCount = vertexSSBOInfo.numElements();
	recordInfo->instanceCount = 1;

	if (indexSSBOInfo && indexData) {
		recordInfo->indexBuffer = *indexData;
		recordInfo->indexCount = indexSSBOInfo->numUnpackedIndices();
	}

	if (indirectBuffer) {
		recordInfo->indirectBuffer = *indirectBuffer;
	}

	return true;
}

// TODO: GPU Instancing
void AfterglowCommandManager::applyDrawCommands(AfterglowFramebuffer& frameBuffer) {
	_impl->applyDrawCommands(frameBuffer);
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