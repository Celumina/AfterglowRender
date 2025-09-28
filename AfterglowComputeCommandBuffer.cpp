#include "AfterglowComputeCommandBuffer.h"

AfterglowComputeCommandBuffer::AfterglowComputeCommandBuffer(AfterglowCommandPool& commandPool) : 
	AfterglowCommandBuffer(commandPool) {
}

void AfterglowComputeCommandBuffer::beginRecord() {
	updateCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	if (vkBeginCommandBuffer(_currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
		throw runtimeError("Failed to begin recording compute command buffer. ");
	}
}

void AfterglowComputeCommandBuffer::setupPipeline(AfterglowComputePipeline& computePipeline) {
	_currentPipeline = &computePipeline;
	vkCmdBindPipeline(_currentCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *_currentPipeline);
}

void AfterglowComputeCommandBuffer::setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs) {
	if (!_currentPipeline) {
		throw runtimeError("setupPipeline() should be called before setupDescriptorSets().");
	}

	vkCmdBindDescriptorSets(
		_currentCommandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		_currentPipeline->pipelineLayout(),
		0,
		setRefs.size(),
		setRefs.address(),
		0,
		nullptr
	);
}

void AfterglowComputeCommandBuffer::record(const RecordInfo& recordInfo) {
	vkCmdDispatch(
		_currentCommandBuffer, 
		recordInfo.dispatchGroup.x, 
		recordInfo.dispatchGroup.y, 
		recordInfo.dispatchGroup.z
	);
}

void AfterglowComputeCommandBuffer::endRecord() {
	if (vkEndCommandBuffer(_currentCommandBuffer) != VK_SUCCESS) {
		throw runtimeError("Failed to record compute command buffer.");
	}
}
