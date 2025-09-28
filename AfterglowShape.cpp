#include "AfterglowShape.h"

shape::NDCRectangle::NDCRectangle(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) : 
	AfterglowShape(commandPool, graphicsQueue) {

	addShape([](IndexData& indexData, VertexData& vertexData){
		indexData.resize(6);
		indexData[0] = 2;
		indexData[1] = 1;
		indexData[2] = 0;
		indexData[3] = 0;
		indexData[4] = 3;
		indexData[5] = 2;

		vertexData.resize(4);
		vertexData[0].set<vert::Position>({ -1.0f, -1.0f });
		vertexData[1].set<vert::Position>({ 1.0f, -1.0f });
		vertexData[2].set<vert::Position>({ 1.0f, 1.0f });
		vertexData[3].set<vert::Position>({ -1.0f, 1.0f });

		vertexData[0].set<vert::TexCoord0>({ 0.0f, 0.0f });
		vertexData[1].set<vert::TexCoord0>({ 1.0f, 0.0f });
		vertexData[2].set<vert::TexCoord0>({ 1.0f, 1.0f });
		vertexData[3].set<vert::TexCoord0>({ 0.0f, 1.0f });
	});

}
