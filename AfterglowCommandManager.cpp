#include "AfterglowCommandManager.h"

AfterglowCommandManager::AfterglowCommandManager(AfterglowRenderPass& renderPass) :
	_renderPass(renderPass), 
	_commandPool(renderPass.device()), 
	_drawCommandBuffer(_commandPool), 
	_computeCommandBuffer(_commandPool) {
	for (const auto& attachmentInfo : _renderPass.subpassContext().inputAttachmentInfos()) {
		_drawRecordInfos[attachmentInfo.domain] = {};
	}
}

AfterglowCommandPool& AfterglowCommandManager::commandPool() {
	return _commandPool;
}

VkCommandBuffer* AfterglowCommandManager::drawCommandBuffers() {
	return &_drawCommandBuffer.current();
}

VkCommandBuffer* AfterglowCommandManager::computeCommandBuffers() {
	return &_computeCommandBuffer.current();
}

bool AfterglowCommandManager::recordDraw(
	AfterglowMaterialResource& matResource, 
	AfterglowDescriptorSetReferences& setRefs,
	AfterglowVertexBufferHandle& vertexBufferHandle,
	util::MutableOptionalRef<AfterglowIndexBuffer> indexBuffer, 
	uint32_t  instanceCount) {

	auto* recordInfo = createDrawRecordInfo(matResource, setRefs);
	if (!recordInfo) {
		return false;
	}
	
	recordInfo->vertexBuffers.push_back(vertexBufferHandle.buffer);
	// TODO: Same indexBuffer for different vertexBuffers
	recordInfo->vertexBufferOffsets.push_back(0);
	recordInfo->vertexCount = vertexBufferHandle.vertexCount;
	recordInfo->instanceCount = instanceCount;

	if (indexBuffer) {
		recordInfo->indexBuffer = indexBuffer->get();
		recordInfo->indexCount = indexBuffer->get().indexCount();
	}
	return true;
}

bool AfterglowCommandManager::recordDraw(
	AfterglowMaterialResource& matResource,
	AfterglowDescriptorSetReferences& setRefs,
	const AfterglowSSBOInfo& vertexSSBOInfo,
	AfterglowStorageBuffer& vertexData,
	util::OptionalRef<AfterglowSSBOInfo> indexSSBOInfo,
	util::MutableOptionalRef<AfterglowStorageBuffer> indexData) {

	auto* recordInfo = createDrawRecordInfo(matResource, setRefs);
	if (!recordInfo) {
		return false;
	}
	recordInfo->vertexBuffers.push_back(vertexData);
	recordInfo->vertexBufferOffsets.push_back(0);
	recordInfo->vertexCount = vertexSSBOInfo.numElements;
	recordInfo->instanceCount = 1;

	if (indexSSBOInfo && indexData) {
		recordInfo->indexBuffer = indexData->get();
		recordInfo->indexCount = indexSSBOInfo->get().numElements;
	}
	return true;
}

// TODO: GPU Instancing
void AfterglowCommandManager::applyDrawCommands(AfterglowFramebuffer& frameBuffer) {	
	_drawCommandBuffer.reset(_commandPool.device().currentFrameIndex());

	auto& subpassContext = _renderPass.subpassContext();

	VkExtent2D extent = _renderPass.swapchain().extent();
	AfterglowDrawCommandBuffer::BeginInfo beginInfo;
	beginInfo.renderPassBegin.renderPass = _renderPass;
	beginInfo.renderPassBegin.framebuffer = frameBuffer;
	beginInfo.renderPassBegin.renderArea.extent = extent;
	beginInfo.renderPassBegin.clearValueCount = subpassContext.clearValueCount();
	beginInfo.renderPassBegin.pClearValues = subpassContext.clearValues().data();
	beginInfo.viewport.width = static_cast<float>(extent.width);
	beginInfo.viewport.height = static_cast<float>(extent.height);
	beginInfo.scissor.extent = extent;
	_drawCommandBuffer.beginRecord(beginInfo);

	bool isFirstDomain = true;
	for (auto& [domain, pipelineSetRecordInfos] : _drawRecordInfos) {
		if (!isFirstDomain) {
			_drawCommandBuffer.nextSubpassRecord();
		}
		isFirstDomain = false;
		for (auto& [pipeline, setRecordInfos] : pipelineSetRecordInfos) {
			_drawCommandBuffer.setupPipeline(*pipeline);
			for (auto& [setRefs, recordInfos] : setRecordInfos) {
				_drawCommandBuffer.setupDescriptorSets(*setRefs);
				for (auto& recordInfo : recordInfos) {
					_drawCommandBuffer.record(recordInfo);
				}
			}
		}
	}
	_drawCommandBuffer.endRecord();
	for (auto& [domain, pipelineSetRecordInfos] : _drawRecordInfos) {
		pipelineSetRecordInfos.clear();
	}
}

void AfterglowCommandManager::recordCompute(AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs) {
	auto& matLayout = matResource.materialLayout();\

	// SSBO initialization from compute shader.
	if (matLayout.shouldInitSSBOsTrigger()) {
		auto& ssboInitComputePipelines = matLayout.ssboInitComputePipelines();
		for (auto& pipeline : ssboInitComputePipelines) {
			_computeRecordInfos[&*pipeline][&setRefs].emplace_back(
				matLayout.material().computeTask().dispatchGroup()
			);
		}
	}
	// Dispatch regular comptue shader.
	else {
		_computeRecordInfos[&matLayout.computePipeline()][&setRefs].emplace_back(
			matLayout.material().computeTask().dispatchGroup()
		);
	}
}

void AfterglowCommandManager::applyComputeCommands() {
	_computeCommandBuffer.reset(_commandPool.device().currentFrameIndex());
	_computeCommandBuffer.beginRecord();
	for(auto& [pipeline, setRecordInfos] : _computeRecordInfos) {
		//if (!pipeline) {
		//	DEBUG_CLASS_WARNING("Compute pipeline is not created.");
		//	continue;
		//}
		_computeCommandBuffer.setupPipeline(*pipeline);
		for (auto& [setRefs, recordInfos] : setRecordInfos) {
			//if(setRefs->size() == 0) {
			//	DEBUG_CLASS_WARNING("Compute set references is empty.");
			//	continue;
			//}
			_computeCommandBuffer.setupDescriptorSets(*setRefs);
			for (auto& recordInfo : recordInfos) {
				_computeCommandBuffer.record(recordInfo);
			}
		}
	}
	_computeCommandBuffer.endRecord();
	_computeRecordInfos.clear();
}

inline AfterglowDrawCommandBuffer::RecordInfo* AfterglowCommandManager::createDrawRecordInfo(
	AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs) {
	auto& pipeline = matResource.materialLayout().pipeline();
	auto domain = matResource.materialLayout().material().domain();
	if (!_renderPass.subpassContext().subpassExists(domain)) {
		DEBUG_CLASS_WARNING("Failed to record the draw, due to this material domain is not declare in RenderPass.");
		return nullptr;
	}
	return &_drawRecordInfos[domain][&pipeline][&setRefs].emplace_back();
}
