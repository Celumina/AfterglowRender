#include "AfterglowImageAsset.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct AfterglowImageAsset::Impl {
	std::string path;
	std::shared_ptr<void> data;
	img::Info info;
};

AfterglowImageAsset::AfterglowImageAsset(const std::string& path, img::Format format, img::ColorSpace colorSpace) :
	_impl(std::make_unique<Impl>()) {
	_impl->path = path;
	_impl->info.format = format;
	_impl->info.colorSpace = colorSpace;
	initImage();
}

AfterglowImageAsset::AfterglowImageAsset(const img::AssetInfo& assetInfo) :
	_impl(std::make_unique<Impl>()) {
	_impl->path = assetInfo.path;
	_impl->info.format = assetInfo.format;
	_impl->info.colorSpace = assetInfo.colorSpace;
	initImage();
}

AfterglowImageAsset::~AfterglowImageAsset() {
}

img::Info AfterglowImageAsset::info() {
	return _impl->info;
}

std::weak_ptr<void> AfterglowImageAsset::data() {
	return _impl->data;
}

void AfterglowImageAsset::freeImageData(void* data) {
	stbi_image_free(data);
	DEBUG_INFO("[AfterglowImageAsset] Image data was freed.");
}

void AfterglowImageAsset::initImage() {
	_impl->data.reset(
		stbi_load(
		_impl->path.data(), 
		&_impl->info.width, 
		&_impl->info.height, 
		&_impl->info.channels, 
		static_cast<int>(_impl->info.format)
	), 
		freeImageData
	);

	DEBUG_INFO("[AfterglowImageAsset] Image data was loaded.");

	// Here imageFormat enum value equal to channel count.
	_impl->info.size = static_cast<uint64_t>(_impl->info.width) * _impl->info.height * static_cast<int>(_impl->info.format);
	if (!_impl->data) {
		DEBUG_CLASS_ERROR(std::format("Failed to load image file: {}", _impl->path));
		throw std::runtime_error("[AfterglowImageAsset] Failed to load image file.");
	}
}