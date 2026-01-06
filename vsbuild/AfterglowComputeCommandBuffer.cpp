#include "AfterglowComputeCommandBuffer.h"
#include "AfterglowShaderAsset.h"

AfterglowComputeCommandBuffer::AfterglowComputeCommandBuffer(AfterglowCommandPool& commandPool) : 
	AfterglowCommandBuffer(commandPool) {
	_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
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
	if (_currentPipeline != &computePipeline) {
		_currentPipeline = &computePipeline;
		vkCmdBindPipeline(_currentCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *_currentPipeline);
	}
}

void AfterglowComputeCommandBuffer::setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs) {
	if (!_currentPipeline) {
		throw runtimeError("setupPipeline() should be called before setupDescriptorSets().");
	}

	if (_currentSetRefs != &setRefs) {
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
}

void AfterglowComputeCommandBuffer::setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs, AfterglowPipelineLayout& pipelineLayout) {
	if (_currentSetRefs != &setRefs) {
		vkCmdBindDescriptorSets(
			_currentCommandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipelineLayout,
			0,
			setRefs.size(),
			setRefs.address(),
			0,
			nullptr
		);
	}
}

void AfterglowComputeCommandBuffer::dispatch(const RecordInfo& recordInfo) {
	vkCmdDispatch(
		_currentCommandBuffer, 
		recordInfo.dispatchGroup.x, 
		recordInfo.dispatchGroup.y, 
		recordInfo.dispatchGroup.z
	);
}

void AfterglowComputeCommandBuffer::barrier() {
	vkCmdPipelineBarrier(
		_currentCommandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		1, &_barrier,
		0, nullptr,
		0, nullptr
	);
}

void AfterglowComputeCommandBuffer::endRecord() {
	if (vkEndCommandBuffer(_currentCommandBuffer) != VK_SUCCESS) {
		throw runtimeError("Failed to record compute command buffer.");
	}
}
