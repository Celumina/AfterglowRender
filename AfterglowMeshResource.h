#pragma once

#include "AfterglowSharedMeshPool.h"
#include "AfterglowStaticMeshComponent.h"
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

	bool activated() const;
	void setActivated(bool activated);

	void bindStaticMesh(AfterglowStaticMeshComponent& staticMesh);
	AfterglowStaticMeshComponent& staticMesh() const;

	
	// std::pair<AfterglowIndexBuffer&, AfterglowVertexBuffer&> addIndexVertexBuffer(AfterglowDevice& device);

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
	// Update this value to verify static mesh is exists. 
	// Set to false in update() every frame, and set to true in registerStaticMesh().
	// Keep this value true to remain a mesh in rendering.
	bool _activated = true;
	Mode _mode = Mode::Custom;

	// Storage data
	std::unique_ptr<MeshBuffer> _meshBuffer = nullptr;

	// Reference data
	std::unique_ptr<AfterglowMeshReference> _meshReference = nullptr;

	ubo::MeshUniform _meshUniform = {};

	AfterglowStaticMeshComponent* _staticMeshHandle = nullptr;
};

