#pragma once
#include "AfterglowSharedResourcePool.h"
#include "AfterglowTextureImage.h"
#include "AssetDefinitions.h"


struct AfterglowTexturePoolResource : public AfterglowSharedPoolResource {
	img::Info info;
	AfterglowTextureImage::AsElement buffer;
};


class AfterglowTextureReference : public AfterglowResourceReference<img::AssetInfo, AfterglowTexturePoolResource> {
public:
	AfterglowTextureReference(const img::AssetInfo& assetInfo, AfterglowResourceReference::Resources& textures, AfterglowReferenceCount& count);
	AfterglowTextureReference(const AfterglowTextureReference& other);

	const img::Info& info() const noexcept;
	AfterglowTextureImage& texture() noexcept;
};


class AfterglowSharedTexturePool : public AfterglowSharedResourcePool<AfterglowTextureReference> {
public: 
	AfterglowSharedTexturePool(
		AfterglowCommandPool& commandPool, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowSynchronizer& synchronizer
	);

	// @brief: Get ref of texture resource, if resource not exists, it will create texture from file automatically.
	AfterglowTextureReference texture(const img::AssetInfo& assetInfo);
	AfterglowTextureReference texture(img::AssetInfo&& rval);
	
	// TODO: 
	// AfterglowSampler& sharedSampler();

private:
	Resource* createTexture(img::AssetInfo& assetInfo);

	// TODO: 
	// AfterglowSampler::AsElement _sharedSampler;
};
