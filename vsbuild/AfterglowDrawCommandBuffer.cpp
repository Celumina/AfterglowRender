#include "AfterglowDrawCommandBuffer.h"
#include "AfterglowPipeline.h"
#include "AfterglowFramebuffer.h"
#include "AfterglowPassInterface.h"
#include "AfterglowDescriptorSetReferences.h"
#include "RenderConfigurations.h"


AfterglowDrawCommandBuffer::AfterglowDrawCommandBuffer(AfterglowCommandPool& commandPool) : 
	AfterglowCommandBuffer(commandPool) {
}

void AfterglowDrawCommandBuffer::beginRecord() {
	updateCurrentCommandBuffer();

	VkCommandBufferBeginInfo commandBufferBegin{};
	commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBegin.flags = 0;
	commandBufferBegin.pInheritanceInfo = nullptr;

	// Here call operator Type  will make sure command buffer be created.
	if (vkBeginCommandBuffer(_currentCommandBuffer, &commandBufferBegin) != VK_SUCCESS) {
		throw runtimeError("Failed to begin recording command buffer.");
	}
}

void AfterglowDrawCommandBuffer::beginRenderPass(PassBeginInfo& beginInfo) {
	// # 0 cmd
	// VK_SUBPASS_CONTENTS_INLINE: for primary command buffers.
	// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: for secondary command buffers.
	vkCmdBeginRenderPass(_currentCommandBuffer, &beginInfo.renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	// TODO: Given a function to reset viewport and scissor for downsampling.
	// # 4 cmd
	vkCmdSetViewport(_currentCommandBuffer, 0, 1, &beginInfo.viewport);

	// # 5 cmd
	// vkCmdSetScissor(commandBuffer, firstScissorIndex, scissorCount, scissorHandle);
	vkCmdSetScissor(_currentCommandBuffer, 0, 1, &beginInfo.scissor);
}

void AfterglowDrawCommandBuffer::beginRenderPass(AfterglowPassInterface& pass, uint32_t imageIndex) {
	auto& subpassContext = pass.subpassContext();
	if (pass.framebuffers().size() <= 1) {
		imageIndex = 0;
	}
	auto& framebuffer = pass.framebuffer(imageIndex);

	AfterglowDrawCommandBuffer::PassBeginInfo beginInfo{};
	beginInfo.renderPassBegin.renderPass = pass.renderPass();
	beginInfo.renderPassBegin.framebuffer = framebuffer;
	beginInfo.renderPassBegin.renderArea.extent = framebuffer.extent();
	beginInfo.renderPassBegin.clearValueCount = subpassContext.clearValueCount();
	beginInfo.renderPassBegin.pClearValues = subpassContext.clearValues().data();
	beginInfo.viewport.width = static_cast<float>(framebuffer.extent().width);
	beginInfo.viewport.height = static_cast<float>(framebuffer.extent().height);
	beginInfo.scissor.extent = framebuffer.extent();

	beginRenderPass(beginInfo);
}

void AfterglowDrawCommandBuffer::setResolution(float width, float height) {
	VkViewport viewport {
		.x = 0.0f, 
		.y = 0.0f, 
		.width = width, 
		.height = height, 
		.minDepth = cfg::reverseDepth ? 1.0f : 0.0f,
		.maxDepth = cfg::reverseDepth ? 0.0f : 1.0f
	};
	VkRect2D scissor {
		.offset = { 0, 0 }, 
		.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
	};
	vkCmdSetViewport(_currentCommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(_currentCommandBuffer, 0, 1, &scissor);
}

void AfterglowDrawCommandBuffer::setupPipeline(AfterglowPipeline& pipeline) {
	if (_currentPipeline != &pipeline) {
		_currentPipeline = &pipeline;
		// The second parameter specifies if the pipeline object is a graphics or compute pipeline.
		vkCmdBindPipeline(_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}
}

void AfterglowDrawCommandBuffer::setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs) {
	if (!_currentPipeline) {
		throw runtimeError("setupPipeline() should be called before setupDescriptorSets().");
	}
	
	if (_currentSetRefs != &setRefs) {
		_currentSetRefs = &setRefs;
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
}

void AfterglowDrawCommandBuffer::draw(const RecordInfo& recordInfo) {
	static constexpr std::array<VkDeviceSize, 1> vertexoffsets = { 0 };
	
	vkCmdBindVertexBuffers(
		_currentCommandBuffer, 0, 1, &recordInfo.vertexBuffer, vertexoffsets.data()
	);

	// If indexBuffer exists, draw indexed.
	if (recordInfo.indexBuffer) {
		vkCmdBindIndexBuffer(_currentCommandBuffer, recordInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		if (recordInfo.indirectBuffer) {
			// @note: support indexed indiret draw only
			// 16 is the storage stuct memory alignment size
			constexpr uint32_t strideSize = static_cast<uint32_t>(util::Align(sizeof(VkDrawIndexedIndirectCommand), 16));
			vkCmdDrawIndexedIndirect(_currentCommandBuffer, recordInfo.indirectBuffer, 0, recordInfo.indirectCommandCount, strideSize);
		}
		else {
			// Usage: vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstanceIndex);
			vkCmdDrawIndexed(_currentCommandBuffer, recordInfo.indexCount, recordInfo.instanceCount, 0, 0, 0);
		}
	}
	// Otherwise draw directly.
	else {
		if (recordInfo.indirectBuffer) {
			constexpr uint32_t strideSize = static_cast<uint32_t>(util::Align(sizeof(VkDrawIndirectCommand), 16));
			vkCmdDrawIndirect(_currentCommandBuffer, recordInfo.vertexBuffer, 0, recordInfo.indirectCommandCount, strideSize);
		}
		else {
			// Usage: vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertexIndex, firstInstanceIndex);
			vkCmdDraw(_currentCommandBuffer, recordInfo.vertexCount, recordInfo.instanceCount, 0, 0);
		}
	}
}

void AfterglowDrawCommandBuffer::nextSubpass() {
	vkCmdNextSubpass(_currentCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
}

void AfterglowDrawCommandBuffer::endRenderPass() {
	// # 8 cmd
	vkCmdEndRenderPass(_currentCommandBuffer);
}

void AfterglowDrawCommandBuffer::endRecord() {
	// # 9 cmd
	if (vkEndCommandBuffer(_currentCommandBuffer) != VK_SUCCESS) {
		throw runtimeError("Failed to record command buffer.");
	}
}

void AfterglowDrawCommandBuffer::barrier(
	const std::vector<VkImageMemoryBarrier>* barriers, 
	VkPipelineStageFlags srcPipelineStage, 
	VkPipelineStageFlags destPipelineStage
) {
	if (!barriers || barriers->empty()) {
		return;
	}
	vkCmdPipelineBarrier(
		_currentCommandBuffer,
		srcPipelineStage, 
		destPipelineStage,
		0, 
		0, nullptr, 
		0, nullptr, 
		barriers->size(), barriers->data()
	);
}

void AfterglowDrawCommandBuffer::barrier(AfterglowPassInterface& pass) {
	barrier(pass.exportBarriers(), pass.exportBarrierSrcPipelineStage());
}

AfterglowDrawCommandBuffer::PassBeginInfo::PassBeginInfo() {
	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBegin.clearValueCount = 0;
	renderPassBegin.pClearValues = nullptr;
	//  The render area defines where shader loads and stores will take place.
	renderPassBegin.renderArea.offset = { 0, 0 };

	viewport.x = 0.0f;
	viewport.y = 0.0f;

	viewport.minDepth = cfg::reverseDepth ? 1.0f : 0.0f;
	viewport.maxDepth = cfg::reverseDepth ? 0.0f : 1.0f;

	scissor.offset = { 0, 0 };
}