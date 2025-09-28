#pragma once
#include "AfterglowCommandBuffer.h"

class AfterglowDrawCommandBuffer : public AfterglowCommandBuffer<AfterglowDrawCommandBuffer> {
public:
	using VertexBufferArray = std::vector<VkBuffer>;
	using VertexBufferOffsetArray = std::vector<VkDeviceSize>;

	struct BeginInfo {
		BeginInfo();

		VkCommandBufferBeginInfo commandBufferBegin{};
		VkRenderPassBeginInfo renderPassBegin{};

		// Viewport
		VkViewport viewport{};
		VkRect2D scissor{};
	};

	struct RecordInfo {
		// Index Buffer
		VkBuffer indexBuffer = nullptr;

		// Vertex Buffer
		VertexBufferArray vertexBuffers;
		VertexBufferOffsetArray vertexBufferOffsets;

		// Vertex Info
		uint32_t indexCount = 0;
		uint32_t vertexCount = 0;
		uint32_t instanceCount = 0;
	};

	AfterglowDrawCommandBuffer(AfterglowCommandPool& commandPool);

	// Call layers example: 
	// begin 
	//		-> setupPipeline 
	//			-> setupDescriptorSets 
	//				-> record 
	//				-> ... 
	//			-> setupDescriptorSet 
	//				-> record 
	//		-> setupPipeline 
	//			-> setupDescriptorSet 
	//				-> record 
	// -> nextSubpassRecord
	//		-> setupPipeline 
	//			-> setupDescriptorSets 
	//				-> record 
	//				-> ... 
	// end
	void beginRecord(BeginInfo& beginInfo);
	// Relative with material.
	void setupPipeline(AfterglowPipeline& pipeline);
	// Relative with material instance.
	void setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs);
	// Relative with drawcall mesh.
	void record(const RecordInfo& recordInfo);
	void nextSubpassRecord();
	void endRecord();

private:
	AfterglowPipeline* _currentPipeline = nullptr;
};

