#pragma once

#include "AfterglowBuffer.h"

class AfterglowUniformBuffer : public AfterglowBuffer<AfterglowUniformBuffer> {
public:
	AfterglowUniformBuffer(AfterglowDevice& device, const void* uniform, uint64_t uniformSize);

	void updateMemory();
	uint64_t byteSize() override;

	const void* sourceData() const;

private:
	const void* _uniform;
	uint64_t _uniformSize;
	void* _mapped;
};

