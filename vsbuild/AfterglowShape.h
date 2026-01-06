#pragma once

#include "AfterglowIndexBuffer.h"
#include "AfterglowVertexBuffer.h"
#include "AssetDefinitions.h"

template<vert::VertexType Type>
class AfterglowShape : public AfterglowObject {
public:
	using Vertex = Type;
	using VertexBuffer = AfterglowVertexBufferTemplate<Type>;
	using IndexArray = AfterglowIndexBuffer::IndexArray;

	struct VertexArray {
		inline void resize(size_t numVertices) { data->resize(sizeof(Vertex) * numVertices); }
		Vertex& operator[](size_t vertexIndex) { return *reinterpret_cast<Vertex*>(&((*data)[sizeof(Vertex) * vertexIndex])); }

		std::shared_ptr<vert::VertexData> data;
	};

	struct Resource {
		std::shared_ptr<IndexArray> indexData;
		VertexArray vertexData;
	};

	AfterglowIndexBuffer::Array& indexBuffers();
	std::vector<AfterglowVertexBufferHandle>& vertexBufferHandles();

protected:
	// @param: Indices and vertices
	using AddShapeCallback = void(IndexArray&, VertexArray&);

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
	auto& resource = _resources.emplace_back(std::make_shared<IndexArray>(), VertexArray{std::make_shared<vert::VertexData>()});

	callback(*resource.indexData, resource.vertexData);

	indexBuffer.bind(resource.indexData);
	vertexBuffer.bind(resource.vertexData.data);
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