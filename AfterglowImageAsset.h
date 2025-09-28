#pragma once

#include "AfterglowImage.h"

class AfterglowImageAsset {
public:
	AfterglowImageAsset(const std::string& path, img::Format format, img::ColorSpace colorSpace = img::ColorSpace::SRGB);
	AfterglowImageAsset(const img::AssetInfo& assetInfo);
	~AfterglowImageAsset();

	img::Info info();
	std::weak_ptr<void> data();

private:
	static void freeImageData(void* data);

	void initImage();
	struct Context;
	std::unique_ptr<Context> _context;
};

