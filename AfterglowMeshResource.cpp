#include "AfterglowMeshResource.h"

AfterglowMeshResource::AfterglowMeshResource(Mode mode) : _mode(mode) {
	if (mode == Mode::Custom) {
		_meshBuffer = std::make_unique<MeshBuffer>();
	}
}

bool AfterglowMeshResource::activated() const {
	return _activated;
}

void AfterglowMeshResource::setActivated(bool activated) {
	_activated = activated;
}

void AfterglowMeshResource::bindStaticMesh(AfterglowStaticMeshComponent& staticMesh) {
	_staticMeshHandle = &staticMesh;
}

AfterglowStaticMeshComponent& AfterglowMeshResource::staticMesh() const {
	return *_staticMeshHandle;
}

void AfterglowMeshResource::bindIndexBuffers(AfterglowIndexBuffer::Array& indexBuffers) {
	_meshBuffer->indexBuffers = &indexBuffers;
}

void AfterglowMeshResource::bindVertexBufferHandles(std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles) {
	_meshBuffer->vertexBufferHandles = &vertexBufferHandles;
}

void AfterglowMeshResource::setMeshReference(const AfterglowMeshReference& reference) {
	_meshReference = std::make_unique<AfterglowMeshReference>(reference);
}

const AfterglowMeshReference& AfterglowMeshResource::meshReference() const {
	return *_meshReference;
}

AfterglowIndexBuffer::Array& AfterglowMeshResource::indexBuffers() {
	if (_mode == Mode::Custom) {
		return *_meshBuffer->indexBuffers;
	}
	else if (_mode == Mode::SharedPool) {
		return _meshReference->indexBuffers();
	}
	throw std::runtime_error("[AfterglowMeshResource] Unknown mode.");
}

std::vector<AfterglowVertexBufferHandle>& AfterglowMeshResource::vertexBufferHandles() {
	if (_mode == Mode::Custom) {
		return *_meshBuffer->vertexBufferHandles;
	}
	else if (_mode == Mode::SharedPool) {
		return _meshReference->vertexBufferHandles();
	}
	throw std::runtime_error("[AfterglowMeshResource] Unknown mode.");
}

ubo::MeshUniform& AfterglowMeshResource::meshUniform() {
	return _meshUniform;
}

const ubo::MeshUniform& AfterglowMeshResource::meshUniform() const {
	return _meshUniform;
}
