#pragma once
#include "AfterglowProxyArray.h"
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
	void record(const RecordInfo& recordInfo);
	void endRecord();

private:
	AfterglowComputePipeline* _currentPipeline = nullptr;

};

