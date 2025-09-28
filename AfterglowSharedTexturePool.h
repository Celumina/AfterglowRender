#pragma once
#include "AfterglowSharedResourcePool.h"
#include "AfterglowTextureImage.h"
#include "AssetDefinitions.h"


struct AfterglowTexturePoolResource {
	img::Info info;
	AfterglowTextureImage::AsElement buffer;
	AfterglowReferenceCount count;
};


class AfterglowTextureReference : public AfterglowResourceReference<img::AssetInfo, AfterglowTexturePoolResource> {
public:
	AfterglowTextureReference(const img::AssetInfo& assetInfo, AfterglowResourceReference::Resources& textures, AfterglowReferenceCount& count);
	AfterglowTextureReference(const AfterglowTextureReference& other);

	const img::Info& info();
	AfterglowTextureImage& texture();
};


class AfterglowSharedTexturePool : public AfterglowSharedResourcePool<AfterglowTextureReference> {
public: 
	AfterglowSharedTexturePool(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	// @brief: Get ref of texture resource, if resource not exists, it will create texture from file automatically.
	AfterglowTextureReference texture(const img::AssetInfo& assetInfo);

private:
	Resource* createTexture(const img::AssetInfo& assetInfo);
};
