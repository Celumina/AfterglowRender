#pragma once

#include <imgui.h>

#include "AfterglowCommandPool.h"


struct ImDrawData;
class AfterglowSSBOInfo;
class AfterglowRenderPass;
class AfterglowMaterialResource;
class AfterglowDescriptorSetReferences;
class AfterglowIndexBuffer;
class AfterglowStorageBuffer;
class AfterglowFramebuffer;
struct AfterglowVertexBufferHandle;

class AfterglowCommandManager : public AfterglowObject {
public:
	AfterglowCommandManager(AfterglowRenderPass& renderPass);

	AfterglowCommandPool& commandPool() noexcept;

	// Command buffer element address, and all elements layout are continious.
	// But just one commandBuffer is required here.
	VkCommandBuffer* drawCommandBuffers() noexcept;
	VkCommandBuffer* computeCommandBuffers() noexcept;

	// @brief: Record a StaticMesh!
	// @return: record successfullys.
	bool recordDraw(
		AfterglowMaterialResource& matResource, 
		AfterglowDescriptorSetReferences& setRefs, 
		AfterglowVertexBufferHandle& vertexBufferHandle, 
		AfterglowIndexBuffer* indexBuffer = nullptr, 
		AfterglowStorageBuffer* indirectBuffer = nullptr, 
		uint32_t instanceCount = 1
	);

	// For compute vertex input.
	// @param indexSSBOInfo: [optional] for indexed draw.
	// @param indexData: [optional] for indexed draw.
	bool recordDraw(
		AfterglowMaterialResource& matResource,
		AfterglowDescriptorSetReferences& setRefs,
		const AfterglowSSBOInfo& vertexSSBOInfo, 
		AfterglowStorageBuffer& vertexData,
		const AfterglowSSBOInfo* indexSSBOInfo = nullptr,
		AfterglowStorageBuffer* indexData = nullptr, 
		AfterglowStorageBuffer* indirectBuffer = nullptr
	);

	// @brief: Apply all commands to device. Call it every ticks.
	void applyDrawCommands(AfterglowFramebuffer& frameBuffer);

	void recordCompute(
		AfterglowMaterialResource& matResource,
		AfterglowDescriptorSetReferences& setRefs
	);

	// @brief: Apply all commands to device. Call it every ticks.
	void applyComputeCommands();

	void recordUIDraw(ImDrawData* uiDrawData);

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

