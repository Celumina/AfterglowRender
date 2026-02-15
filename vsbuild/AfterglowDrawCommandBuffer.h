#pragma once
#include "AfterglowCommandBuffer.h"

class AfterglowPipeline;
class AfterglowPassInterface;
class AfterglowDescriptorSetReferences;

class AfterglowDrawCommandBuffer : public AfterglowCommandBuffer<AfterglowDrawCommandBuffer> {
public:
	struct PassBeginInfo {
		PassBeginInfo();

		VkRenderPassBeginInfo renderPassBegin{};

		// Viewport
		VkViewport viewport{};
		VkRect2D scissor{};
	};

	// One index buffer with one vertex buffer, forget the one-to-multi model, its overengineered.
	struct RecordInfo {
		// Vertex Buffer
		VkBuffer vertexBuffer = nullptr;
		// [Optional] Index Buffer
		VkBuffer indexBuffer = nullptr;

		// [Optional] Indirect Buffer 
		VkBuffer indirectBuffer = nullptr;
		// TODO: aquire indirectCommandCount from compute task.
		uint32_t indirectCommandCount = 1;

		// Vertex Info
		uint32_t indexCount = 0;
		uint32_t vertexCount = 0;
		uint32_t instanceCount = 1;
	};

	AfterglowDrawCommandBuffer(AfterglowCommandPool& commandPool);

	// Call layers example: 
	// beginRecord
	//	beginRenderPass
	//		-> setupPipeline 
	//			-> setupDescriptorSets 
	//				-> record 
	//				-> ... 
	//			-> setupDescriptorSet 
	//				-> record 
	//	endRenderPass
	// 	beginRenderPass
	//		-> setupPipeline 
	//			-> setupDescriptorSet 
	//				-> record 
	// -> nextSubpassRecord
	//		-> setupPipeline 
	//			-> setupDescriptorSets 
	//				-> record 
	//				-> ... 
	//	endRenderPass
	// endRecord
	void beginRecord();
	void beginRenderPass(PassBeginInfo& beginInfo);
	/**
	* @brief: Create pass begin info from pass automatically.
	* @param imageIndex: for multiple framebuffers case.
	*/
	// 
	void beginRenderPass(AfterglowPassInterface& pass, uint32_t imageIndex = 0);
	void setResolution(float width, float height);
	// Relative with material.
	void setupPipeline(AfterglowPipeline& pipeline);
	// Relative with material instance.
	void setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs);
	// Relative with drawcall mesh.
	void draw(const RecordInfo& recordInfo);
	void nextSubpass();
	void endRenderPass();
	void endRecord();

	void barrier(
		const std::vector<VkImageMemoryBarrier>* barriers, 
		VkPipelineStageFlags srcPipelineStage, 
		VkPipelineStageFlags destPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	void barrier(AfterglowPassInterface& pass);

private:
	AfterglowPipeline* _currentPipeline = nullptr;
	const AfterglowDescriptorSetReferences* _currentSetRefs = nullptr;
};

