#include "AfterglowMeshResource.h"
#include "ExceptionUtilities.h"

AfterglowMeshResource::AfterglowMeshResource(Mode mode) : _mode(mode) {
	if (mode == Mode::Custom) {
		_meshBuffer = std::make_unique<MeshBuffer>();
	}
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
	EXCEPT_CLASS_RUNTIME("Unknown mode.");
}

std::vector<AfterglowVertexBufferHandle>& AfterglowMeshResource::vertexBufferHandles() {
	if (_mode == Mode::Custom) {
		return *_meshBuffer->vertexBufferHandles;
	}
	else if (_mode == Mode::SharedPool) {
		return _meshReference->vertexBufferHandles();
	}
	EXCEPT_CLASS_RUNTIME("Unknown mode.");
}

ubo::MeshUniform& AfterglowMeshResource::meshUniform() {
	return _meshUniform;
}

const ubo::MeshUniform& AfterglowMeshResource::meshUniform() const {
	return _meshUniform;
}

const model::AABB* AfterglowMeshResource::aabb() const {
	if (!_meshReference) {
		// EXCEPT_CLASS_RUNTIME("MeshReference not found, aabb is supported in SharedPool mode only.");
		return nullptr;
	}
	return &_meshReference->aabb();
}
