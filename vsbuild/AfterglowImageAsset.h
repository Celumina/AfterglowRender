#pragma once

#include "AfterglowImage.h"

class AfterglowImageAsset {
public:
	AfterglowImageAsset(const std::string& path, img::ColorSpace colorSpace = img::ColorSpace::SRGB);
	AfterglowImageAsset(const img::AssetInfo& assetInfo);
	~AfterglowImageAsset();

	const img::Info& info() const noexcept;
	std::weak_ptr<img::DataArray> data() noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

