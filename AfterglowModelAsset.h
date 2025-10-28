#pragma once
#include <vector>
#include <string>
#include <memory>
#include "VertexStructs.h"
#include "AssetDefinitions.h"

class AfterglowModelAsset {
public:
	using IndexArray = std::vector<vert::StandardIndex>;
	using VertexArray = std::vector<vert::StandardVertex>;

	AfterglowModelAsset(const std::string& path);
	// TODO: Custom vertex layout support from here.
	AfterglowModelAsset(const model::AssetInfo& assetInfo);
	// Necessary, because pImpl unique_ptr require a explicit destructor.
	~AfterglowModelAsset();

	uint32_t numMeshes();
	std::weak_ptr<IndexArray> indices(uint32_t meshIndex);
	std::weak_ptr<VertexArray> vertices(uint32_t meshIndex);

	void printModelInfo();

private:
	inline void initialize();

	// pImpl method.
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

