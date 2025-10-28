#pragma once
#include "VertexStructs.h"
#include "AfterglowBuffer.h"

class AfterglowCommandPool;
class AfterglowGraphicsQueue;

class AfterglowIndexBuffer : public AfterglowBuffer<AfterglowIndexBuffer> {
public:
	using Index = vert::StandardIndex;
	using IndexArray = std::vector<Index>;
	using Size = uint32_t;

	AfterglowIndexBuffer(AfterglowDevice& device);

	struct Info {
		Size count;
	};

	// Only call once.
	//void initIndices(uint64_t indexCount);
	void bind(std::weak_ptr<IndexArray> data);

	Size indexCount() const;

	// These functions require binded data were exists.
	// Change index before fill memory to staging buffer is valid.
	void setIndex(Size pos, Index value);
	std::weak_ptr<const IndexArray> indexData() const;
	
	// Creating a staging buffer and use it copy memery to GPU, then clear stagingbuffer.
	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

protected:
	uint64_t byteSize() override;

private:
	inline void initIndicesImplimentation();

	inline std::shared_ptr<IndexArray> safeIndices();

	std::weak_ptr<IndexArray> _indices;
	Info _indexInfo;
};

