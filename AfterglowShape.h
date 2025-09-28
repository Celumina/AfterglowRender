#pragma once

#include "AfterglowIndexBuffer.h"
#include "AfterglowVertexBuffer.h"

template<vert::VertexType Type>
class AfterglowShape : public AfterglowObject {
public:
	using Vertex = Type;
	using VertexBuffer = AfterglowVertexBufferTemplate<Type>;
	using IndexData = AfterglowIndexBuffer::IndexArray;
	using VertexData = typename VertexBuffer::VertexArray;

	struct Resource {
		std::shared_ptr<IndexData> indexData;
		std::shared_ptr<VertexData> vertexData;
	};

	AfterglowIndexBuffer::Array& indexBuffers();
	std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles();

protected:
	using AddShapeCallback = void(IndexData&, VertexData&);

	AfterglowShape(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	void addShape(AddShapeCallback callback);

private:
	AfterglowCommandPool& _commandPool;
	AfterglowGraphicsQueue& _graphicsQueue;

	std::vector<Resource> _resources;

	AfterglowIndexBuffer::Array _indexBuffers;
	VertexBuffer::Array _vertexBuffers;
	std::vector<AfterglowVertexBufferHandle> _vertexBufferHandles;
};


namespace shape {
	class NDCRectangle;

	template<typename Type>
	concept ShapeType = 
		requires { typename Type::Vertex; }
		&& std::is_base_of_v<AfterglowShape<typename Type::Vertex>, Type>;
};


class shape::NDCRectangle : public AfterglowShape<vert::VertexPT0> {
public:
	NDCRectangle(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);
	// Polynormial desctruction test.
	~NDCRectangle() {DEBUG_CLASS_INFO("Shape was destructed."); };
};


template<vert::VertexType Type>
AfterglowShape<Type>::AfterglowShape(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) :
	_commandPool(commandPool),
	_graphicsQueue(graphicsQueue) {
}

template<vert::VertexType Type>
inline void AfterglowShape<Type>::addShape(AddShapeCallback callback) {
	auto& indexBuffer = *_indexBuffers.emplace_back(AfterglowIndexBuffer::makeElement(_commandPool.device()));
	auto& vertexBuffer = *_vertexBuffers.emplace_back(VertexBuffer::makeElement(_commandPool.device()));
	auto& resource = _resources.emplace_back(std::make_shared<IndexData>(), std::make_shared<VertexData>());

	callback(*resource.indexData, *resource.vertexData);

	indexBuffer.bind(resource.indexData);
	vertexBuffer.bind(resource.vertexData);
	indexBuffer.submit(_commandPool, _graphicsQueue);
	vertexBuffer.submit(_commandPool, _graphicsQueue);

	_vertexBufferHandles.push_back(vertexBuffer.handle());
}

template<vert::VertexType Type>
AfterglowIndexBuffer::Array& AfterglowShape<Type>::indexBuffers() {
	return _indexBuffers;
}

template<vert::VertexType Type>
std::vector<AfterglowVertexBufferHandle>& AfterglowShape<Type>::vertexBufferHandles() {
	return _vertexBufferHandles;
}