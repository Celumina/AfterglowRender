#include "AfterglowImageAsset.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct AfterglowImageAsset::Context {
	std::string path;
	std::shared_ptr<void> data;
	img::Info info;
};

AfterglowImageAsset::AfterglowImageAsset(const std::string& path, img::Format format, img::ColorSpace colorSpace) :
	_context(std::make_unique<Context>()) {
	_context->path = path;
	_context->info.format = format;
	_context->info.colorSpace = colorSpace;
	initImage();
}

AfterglowImageAsset::AfterglowImageAsset(const img::AssetInfo& assetInfo) :
	_context(std::make_unique<Context>()) {
	_context->path = assetInfo.path;
	_context->info.format = assetInfo.format;
	_context->info.colorSpace = assetInfo.colorSpace;
	initImage();
}

AfterglowImageAsset::~AfterglowImageAsset() {
}

img::Info AfterglowImageAsset::info() {
	return _context->info;
}

std::weak_ptr<void> AfterglowImageAsset::data() {
	return _context->data;
}

void AfterglowImageAsset::freeImageData(void* data) {
	stbi_image_free(data);
	DEBUG_INFO("[AfterglowImageAsset] Image data was freed.");
}

void AfterglowImageAsset::initImage() {
	_context->data.reset(
		stbi_load(
		_context->path.data(), 
		&_context->info.width, 
		&_context->info.height, 
		&_context->info.channels, 
		static_cast<int>(_context->info.format)
	), 
		freeImageData
	);

	DEBUG_INFO("[AfterglowImageAsset] Image data was loaded.");

	// Here imageFormat enum value equal to channel count.
	_context->info.size = static_cast<uint64_t>(_context->info.width) * _context->info.height * static_cast<int>(_context->info.format);
	if (!_context->data) {
		DEBUG_CLASS_ERROR(std::format("Failed to load image file: {}", _context->path));
		throw std::runtime_error("[AfterglowImageAsset] Failed to load image file.");
	}
}