#include "AfterglowSharedTexturePool.h"

#include <filesystem>
#include "AfterglowImageAsset.h"
#include "GlobalAssets.h"

AfterglowTextureReference::AfterglowTextureReference(const img::AssetInfo& assetInfo, AfterglowResourceReference::Resources& textures, AfterglowReferenceCount& count) :
	AfterglowResourceReference(assetInfo, textures, count) {
}

AfterglowTextureReference::AfterglowTextureReference(const AfterglowTextureReference& other) :
	AfterglowResourceReference(other) {
}

const img::Info& AfterglowTextureReference::info() const noexcept {
	// verifyValue();
	return _value->info;
}

AfterglowTextureImage& AfterglowTextureReference::texture() noexcept {
	// verifyValue();
	return _value->buffer;
}

AfterglowSharedTexturePool::AfterglowSharedTexturePool(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowSynchronizer& synchronizer) :
	AfterglowSharedResourcePool(commandPool, graphicsQueue, synchronizer) {
}

AfterglowTextureReference AfterglowSharedTexturePool::texture(const img::AssetInfo& assetInfo) {
	img::AssetInfo assetInfoCopy = assetInfo;

	auto textureIterator = _resources.find(assetInfoCopy);
	Resource* texture = nullptr;
	if (textureIterator == _resources.end()) {
		texture = createTexture(assetInfoCopy);
	}
	else {
		texture = &(textureIterator->second);
	}

	return AfterglowTextureReference{ std::move(assetInfoCopy), _resources, texture->count };
}

AfterglowTextureReference AfterglowSharedTexturePool::texture(img::AssetInfo&& rval) {
	img::AssetInfo assetInfo{ std::forward<img::AssetInfo>(rval) };
	auto textureIterator = _resources.find(assetInfo);
	Resource* texture = nullptr;
	if (textureIterator == _resources.end()) {
		texture = createTexture(assetInfo);
	}
	else {
		texture = &(textureIterator->second);
	}

	return AfterglowTextureReference{ std::move(assetInfo), _resources, texture->count };
}

AfterglowSharedTexturePool::Resource* AfterglowSharedTexturePool::createTexture(img::AssetInfo& assetInfo) {
	// TODO: mesh pool also do that check?
	if (!std::filesystem::exists(assetInfo.path)) {
		DEBUG_CLASS_ERROR(std::format("Path texture is not exists: \"{}\", it was replaced to default texture. ", assetInfo.path));
		assetInfo.path = img::defaultTextureInfo.path;
		auto textureIterator = _resources.find(assetInfo);
		if (textureIterator != _resources.end()) {
			return &textureIterator->second;
		}
	}

	auto textureIterator = _resources.emplace(assetInfo, Resource{}).first;
	auto& texture = textureIterator->second;
	
	texture.count.setDecreaseCallback(
		[this, textureIterator](AfterglowReferenceCount::Count count) {
			if (count <= 0) {
				DEBUG_CLASS_INFO("Texture was destroyed: " + textureIterator->first.path);
				// _resources.erase(textureIterator);
				removeResource(&textureIterator->first);
			}
		}
	);

	AfterglowImageAsset imageAsset(assetInfo);
	texture.info = imageAsset.info();
	auto& buffer = texture.buffer;
	buffer.recreate(commandPool().device());
	(*buffer).bind(imageAsset.info(), imageAsset.data());
	(*buffer).submit(commandPool(), graphicsQueue());

	return &texture;
}