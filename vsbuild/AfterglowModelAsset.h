#pragma once
#include <vector>
#include <string>
#include <memory>
#include "VertexStructs.h"
#include "AssetDefinitions.h"

class AfterglowModelAsset {
public:
	AfterglowModelAsset(const std::string& path);
	// TODO: Custom vertex layout support from here.
	AfterglowModelAsset(const model::AssetInfo& assetInfo);
	// Necessary, because pImpl unique_ptr require a explicit destructor.
	~AfterglowModelAsset();

	uint32_t numMeshes();
	std::weak_ptr<vert::IndexArray> indices(uint32_t meshIndex);
	std::weak_ptr<vert::VertexData> vertexData(uint32_t meshIndex);

	void printModelInfo();

	template<typename CallbackType, uint32_t index = 0>
	static auto AsType(model::ImportFlag importFlags, CallbackType&& callback);

private:
	inline void initialize();

	// pImpl method.
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

template<typename CallbackType, uint32_t index>
inline auto AfterglowModelAsset::AsType(model::ImportFlag importFlags, CallbackType&& callback) {
	model::ImportFlag vertexBits = importFlags & model::ImportFlag::VertexBitsMask;
	switch (vertexBits) {
	case model::ImportFlag::VertexPTCT0: return callback.template operator() < vert::VertexPTCT0 > ();
	case model::ImportFlag::VertexPNCT0: return callback.template operator() < vert::VertexPNCT0 > ();
	case model::ImportFlag::VertexPCT0: return callback.template operator() < vert::VertexPCT0 > ();
	case model::ImportFlag::VertexPNTT0: return callback.template operator() < vert::VertexPNTT0 > ();
	case model::ImportFlag::VertexPTT0: return callback.template operator() < vert::VertexPTT0 > ();
	case model::ImportFlag::VertexPNT0: return callback.template operator() < vert::VertexPNT0 > ();
	case model::ImportFlag::VertexPT0: return callback.template operator() < vert::VertexPT0 > ();
	case model::ImportFlag::VertexPNTC: return callback.template operator() < vert::VertexPNTC > ();
	case model::ImportFlag::VertexPTC: return callback.template operator() < vert::VertexPTC > ();
	case model::ImportFlag::VertexPNC: return callback.template operator() < vert::VertexPNC > ();
	case model::ImportFlag::VertexPNT: return callback.template operator() < vert::VertexPNT > ();
	case model::ImportFlag::VertexPC: return callback.template operator() < vert::VertexPC > ();
	case model::ImportFlag::VertexPT: return callback.template operator() < vert::VertexPT > ();
	case model::ImportFlag::VertexPN: return callback.template operator() < vert::VertexPN > ();
	case model::ImportFlag::VertexP: return callback.template operator() < vert::VertexP > ();
	default: return callback.template operator() < vert::StandardVertex > ();
	}
}
