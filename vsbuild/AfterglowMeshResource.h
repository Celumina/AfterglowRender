#pragma once

#include "AfterglowSharedMeshPool.h"
#include "UniformBufferObjects.h"

class AfterglowMeshResource {
public:
	// Supports manager buffer itself or ref from shared mesh pool.
	enum class Mode {
		Custom, 
		SharedPool
	};

	struct MeshBuffer {
		AfterglowIndexBuffer::Array* indexBuffers = nullptr;
		std::vector<AfterglowVertexBufferHandle>* vertexBufferHandles = nullptr;
	};

	AfterglowMeshResource(Mode mode);

	void bindIndexBuffers(AfterglowIndexBuffer::Array& indexBuffers);
	void bindVertexBufferHandles(std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles);

	// @desc: For Reference mode. dispatch mesh buffer from 
	void setMeshReference(const AfterglowMeshReference& reference);
	const AfterglowMeshReference& meshReference() const;

	AfterglowIndexBuffer::Array& indexBuffers();
	std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles();

	ubo::MeshUniform& meshUniform();
	const ubo::MeshUniform& meshUniform() const;

private:
	// Storage data
	std::unique_ptr<MeshBuffer> _meshBuffer = nullptr;
	// Reference data
	std::unique_ptr<AfterglowMeshReference> _meshReference = nullptr;
	ubo::MeshUniform _meshUniform = {};

	Mode _mode = Mode::Custom;
};

