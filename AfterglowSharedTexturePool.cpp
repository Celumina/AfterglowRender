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

const img::Info& AfterglowTextureReference::info() {
	verifyValue();
	return _value->info;
}

AfterglowTextureImage& AfterglowTextureReference::texture() {
	verifyValue();
	return _value->buffer;
}

AfterglowSharedTexturePool::AfterglowSharedTexturePool(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) : 
	AfterglowSharedResourcePool(commandPool, graphicsQueue) {
}

AfterglowTextureReference AfterglowSharedTexturePool::texture(const img::AssetInfo& assetInfo) {
	// TODO: mesh pool also do that check?
	img::AssetInfo safeInfo = assetInfo;
	if (!std::filesystem::exists(safeInfo.path)) {
		safeInfo.path = img::defaultTextureInfo.path;
	}

	auto textureIterator = _resources.find(safeInfo);
	Resource* texture = nullptr;
	if (textureIterator == _resources.end()) {
		texture = createTexture(safeInfo);
	}
	else {
		texture = &(textureIterator->second);
	}

	return AfterglowTextureReference{ safeInfo, _resources, texture->count};
}

AfterglowSharedTexturePool::Resource* AfterglowSharedTexturePool::createTexture(const img::AssetInfo& assetInfo) {
	auto textureIterator = _resources.emplace(assetInfo, Resource{}).first;
	auto& texture = textureIterator->second;
	
	texture.count.setDecreaseCallback(
		[this, textureIterator](AfterglowReferenceCount::Count count) {
			if (count <= 0) {
				DEBUG_CLASS_INFO("Texture was destroyed: " + textureIterator->first.path);
				_resources.erase(textureIterator);
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