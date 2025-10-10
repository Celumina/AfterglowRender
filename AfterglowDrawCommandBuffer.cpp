#include "AfterglowDrawCommandBuffer.h"
#include "AfterglowComputeCommandBuffer.h"


AfterglowDrawCommandBuffer::AfterglowDrawCommandBuffer(AfterglowCommandPool& commandPool) : 
	AfterglowCommandBuffer(commandPool) {
}

void AfterglowDrawCommandBuffer::beginRecord(BeginInfo& beginInfo) {
	updateCurrentCommandBuffer();
	// Here call operator Type  will make sure command buffer be created.
	if (vkBeginCommandBuffer(_currentCommandBuffer, &beginInfo.commandBufferBegin) != VK_SUCCESS) {
		throw runtimeError("Failed to begin recording command buffer.");
	}

	// # 0 cmd
	// VK_SUBPASS_CONTENTS_INLINE: for primary command buffers.
	// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: for secondary command buffers.
	vkCmdBeginRenderPass(_currentCommandBuffer, &beginInfo.renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	// # 4 cmd
	vkCmdSetViewport(_currentCommandBuffer, 0, 1, &beginInfo.viewport);

	// # 5 cmd
	// vkCmdSetScissor(commandBuffer, firstScissorIndex, scissorCount, scissorHandle);
	vkCmdSetScissor(_currentCommandBuffer, 0, 1, &beginInfo.scissor);
}

void AfterglowDrawCommandBuffer::setupPipeline(AfterglowPipeline& pipeline) {
	_currentPipeline = &pipeline;
	// # 1 cmd
	// The second parameter specifies if the pipeline object is a graphics or compute pipeline.
	vkCmdBindPipeline(_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void AfterglowDrawCommandBuffer::setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs) {
	if (!_currentPipeline) {
		throw runtimeError("setupPipeline() should be called before setupDescriptorSets().");
	}
	// # 6 cmd
// vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, pipelineLayout, firstSetIndex, setCount, pSets, dynamicOffsetCount, pDynamicOffset);
	vkCmdBindDescriptorSets(
		_currentCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_currentPipeline->pipelineLayout(),
		0,
		setRefs.size(),
		setRefs.address(),
		0,
		nullptr
	);
}

void AfterglowDrawCommandBuffer::record(const RecordInfo& recordInfo) {
	static constexpr std::array<VkDeviceSize, 1> vertexoffsets = { 0 };
	
	vkCmdBindVertexBuffers(
		_currentCommandBuffer, 0, 1, &recordInfo.vertexBuffer, vertexoffsets.data()
	);

	// If indexBuffer exists, draw indexed.
	if (recordInfo.indexBuffer) {
		vkCmdBindIndexBuffer(_currentCommandBuffer, recordInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Usage: vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstanceIndex);
		vkCmdDrawIndexed(_currentCommandBuffer, recordInfo.indexCount, recordInfo.instanceCount, 0, 0, 0);
	}
	// Otherwise draw directly.
	else {
		// Usage: vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertexIndex, firstInstanceIndex);
		vkCmdDraw(_currentCommandBuffer, recordInfo.vertexCount, recordInfo.instanceCount, 0, 0);
	}
}

void AfterglowDrawCommandBuffer::nextSubpassRecord() {
	vkCmdNextSubpass(_currentCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
}

void AfterglowDrawCommandBuffer::endRecord() {
	// # 8 cmd
	vkCmdEndRenderPass(_currentCommandBuffer);

	// # 9 cmd
	if (vkEndCommandBuffer(_currentCommandBuffer) != VK_SUCCESS) {
		throw runtimeError("Failed to record command buffer.");
	}
}

AfterglowDrawCommandBuffer::BeginInfo::BeginInfo() {
	commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBegin.flags = 0;
	commandBufferBegin.pInheritanceInfo = nullptr;

	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBegin.clearValueCount = 0;
	renderPassBegin.pClearValues = nullptr;
	//  The render area defines where shader loads and stores will take place.
	renderPassBegin.renderArea.offset = { 0, 0 };

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	scissor.offset = { 0, 0 };
}