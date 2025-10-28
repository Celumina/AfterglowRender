#pragma once

#include <imgui.h>

#include "AfterglowDrawCommandBuffer.h"
#include "AfterglowComputeCommandBuffer.h"
#include "AfterglowFramebuffer.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowIndexBuffer.h"

#include "AfterglowComputePipeline.h"
#include "ComputeDefinitions.h"

struct AfterglowSSBOInfo;

class AfterglowCommandManager : public AfterglowObject {
public:
	// TODO: Here "vector" has not any specification, try to add something.
	template<typename RecordInfoType>
	using SetRecordInfos = std::unordered_map<AfterglowDescriptorSetReferences*, std::vector<RecordInfoType>>;

	template<typename PipelineType, typename RecordInfo>
	using PipelineSetRecordInfos = std::unordered_map<PipelineType*, SetRecordInfos<RecordInfo>>;

	// For DrawCommandBuffer: 
	using PipelineSetDrawRecordInfos = PipelineSetRecordInfos<AfterglowPipeline, AfterglowDrawCommandBuffer::RecordInfo>;
	using DomainPipelineSetDrawRecordInfos = std::map<render::Domain, PipelineSetDrawRecordInfos>;

	// For ComputeCommandBuffer: 
	using PipelineSetComputeRecordInfos = PipelineSetRecordInfos<AfterglowComputePipeline, AfterglowComputeCommandBuffer::RecordInfo>;

	AfterglowCommandManager(AfterglowRenderPass& renderPass);

	AfterglowCommandPool& commandPool();

	// Command buffer element address, and all elements layout are continious.
	// But just one commandBuffer is required here.
	VkCommandBuffer* drawCommandBuffers();
	VkCommandBuffer* computeCommandBuffers();

	// @brief: Record a StaticMesh!
	// @return: record successfullys.
	bool recordDraw(
		AfterglowMaterialResource& matResource, 
		AfterglowDescriptorSetReferences& setRefs, 
		AfterglowVertexBufferHandle& vertexBufferHandle, 
		util::MutableOptionalRef<AfterglowIndexBuffer> indexBuffer = std::nullopt, 
		uint32_t instanceCount = 1
	);

	// For compute vertex input.
	// TODO: VkBuffer can be aquired from ssbo directly.
	// TODO: SSBO as index data param.
	bool recordDraw(
		AfterglowMaterialResource& matResource,
		AfterglowDescriptorSetReferences& setRefs,
		const AfterglowSSBOInfo& vertexSSBOInfo, 
		AfterglowStorageBuffer& vertexData,
		util::OptionalRef<AfterglowSSBOInfo> indexSSBOInfo = std::nullopt,
		util::MutableOptionalRef<AfterglowStorageBuffer> indexData = std::nullopt
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
	inline AfterglowDrawCommandBuffer::RecordInfo* aquireDrawRecordInfo(
		AfterglowMaterialResource& matResource, AfterglowDescriptorSetReferences& setRefs
	);

	AfterglowRenderPass& _renderPass;
	AfterglowCommandPool _commandPool;

	AfterglowDrawCommandBuffer _drawCommandBuffer;
	// TODO: Update modified meehes only, don't clear every update.
	// TODO: A easy implementation:
	//  Adding a dirty flag and set to false every apply. 
	//  If renderer is not update the dirty flag, clear this recordInfo instead of submit it.
	DomainPipelineSetDrawRecordInfos _drawRecordInfos;
	
	AfterglowComputeCommandBuffer _computeCommandBuffer;
	// TODO: Update modified computeTasks only, don't clear every update.
	PipelineSetComputeRecordInfos _computeRecordInfos;

	ImDrawData* _uiDrawData = nullptr;
};

