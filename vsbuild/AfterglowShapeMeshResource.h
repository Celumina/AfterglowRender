#pragma once

#include "AfterglowShape.h"
#include "AfterglowMeshResource.h"

class AfterglowCommandPool;
class AfterglowGraphicsQueue;

class AfterglowShapeMeshResource : public AfterglowMeshResource {
public:
	AfterglowShapeMeshResource();

	template<shape::ShapeType Type>
	void initializeShape(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

private: 
	std::unique_ptr<AfterglowObject> _shape;
};

template<shape::ShapeType Type>
inline void AfterglowShapeMeshResource::initializeShape(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) {
	_shape = std::make_unique<Type>(commandPool, graphicsQueue);
	Type& shape = *reinterpret_cast<Type*>(_shape.get());
	bindIndexBuffers(shape.indexBuffers());
	bindVertexBufferHandles(shape.vertexBufferHandles());
}
