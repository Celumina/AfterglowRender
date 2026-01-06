#pragma once

#include "AfterglowCommandBuffer.h"
#include "AfterglowComputePipeline.h"
#include "ComputeDefinitions.h"

class AfterglowComputeCommandBuffer : public AfterglowCommandBuffer<AfterglowComputeCommandBuffer> {
public:
	struct RecordInfo {
		compute::DispatchGroup dispatchGroup{};
	};

	AfterglowComputeCommandBuffer(AfterglowCommandPool& commandPool);

	void beginRecord();
	void setupPipeline(AfterglowComputePipeline& computePipeline);
	void setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs);
	void setupDescriptorSets(const AfterglowDescriptorSetReferences& setRefs, AfterglowPipelineLayout& pipelineLayout);
	void dispatch(const RecordInfo& recordInfo);
	void barrier();
	void endRecord();

private:
	VkMemoryBarrier _barrier{};

	AfterglowComputePipeline* _currentPipeline = nullptr;
	const AfterglowDescriptorSetReferences* _currentSetRefs = nullptr;
};

