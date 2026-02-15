#include "AfterglowImageAsset.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

/** enum alias name
* enum class {
*	A, 
*	B = A, 
*	...
* };
*/


#include <omp.h>
#include <OpenImageIO/imageio.h>
#include "AfterglowUtilities.h"
#include "ExceptionUtilities.h"

struct AfterglowImageAsset::Impl {
	void initImage();

	// @desc: load 2D rgb image into rgba standard data array
	inline bool loadRGBImageDataWithPadding(OIIO::ImageInput& imageInput);
	inline img::Format imageFormat(const OIIO::ImageSpec& imageSpec) const;
	
	std::string path;
	std::shared_ptr<img::DataArray> data;
	img::Info info;
};

AfterglowImageAsset::AfterglowImageAsset(const std::string& path, img::ColorSpace colorSpace) :
	_impl(std::make_unique<Impl>()) {
	_impl->path = path;
	// _impl->info.format = format;
	_impl->info.colorSpace = colorSpace;
	_impl->initImage();
}

AfterglowImageAsset::AfterglowImageAsset(const img::AssetInfo& assetInfo) :
	_impl(std::make_unique<Impl>()) {
	_impl->path = assetInfo.path;
	// _impl->info.format = assetInfo.format;
	_impl->info.colorSpace = assetInfo.colorSpace;
	_impl->initImage();
}

AfterglowImageAsset::~AfterglowImageAsset() {
}

const img::Info& AfterglowImageAsset::info() const noexcept {
	return _impl->info;
}

std::weak_ptr<img::DataArray> AfterglowImageAsset::data() noexcept {
	return _impl->data;
}

void AfterglowImageAsset::Impl::initImage() {
	bool loadSuccess = false;

	// @deprecated: Doesn't work for me.
	// OIIO::attribute("png:linear_premult", 0);

	// Avoid the implicit pre-multiplied alpha
	OIIO::ImageSpec config;
	config["oiio:UnassociatedAlpha"] = 1;

	auto file = OIIO::ImageInput::open(path, &config);
	if (!file) {
		EXCEPT_CLASS_RUNTIME(std::format("Failed to load image file: {}", path));
	}

	auto& spec = file->spec();
	info.width = spec.width;
	info.height = spec.height;
	info.depth = spec.depth;
	// RGB is not widely supported in GPU
	info.channels = img::Channel(spec.nchannels) == img::Channel::RGB ? img::Channel::RGBA : img::Channel(spec.nchannels);

	info.format = imageFormat(spec.format);
	info.size = static_cast<uint64_t>(info.width)
		* info.height
		* info.depth
		* util::EnumValue(info.channels)
		* img::FormatByteSize(info.format);

	data = std::make_shared<img::DataArray>(info.size, 0xFF);

	// TODO: Subimage for cubemap?
	// TODO: Generate miplevels and cache them here?

	// Padding value depend on the data array initialization.
	if (spec.nchannels == util::EnumValue(img::Channel::RGB)) {
		// DEBUG_COST_BEGIN(std::format("IMG_RGB: {}", path));
		loadSuccess = loadRGBImageDataWithPadding(*file);
		// DEBUG_COST_END;
	}
	else {
		// DEBUG_COST_BEGIN(std::format("IMG_OTHER: {}", path));
		loadSuccess = file->read_image(0, 0, 0, -1, spec.format, data->data());
		// DEBUG_COST_END;
	}
	file->close();

	if (!loadSuccess) {
		EXCEPT_CLASS_RUNTIME(std::format("Faild to read image: \"{}\", due to {}.", path, file->geterror()));
	}
}

inline bool AfterglowImageAsset::Impl::loadRGBImageDataWithPadding(OIIO::ImageInput& imageInput) {
	auto& spec = imageInput.spec();
	img::DataArray rgbData(spec.image_bytes());
	if (!imageInput.read_image(0, 0, 0, -1, spec.format, rgbData.data())) {
		return false;
	}

	// Ensure the data is initialized.
	auto& rgbaData = *data;

	#pragma omp parallel for
	for (int y = 0; y < spec.height; y++) {
		for (int x = 0; x < spec.width; x++) {
			size_t rgbIndex = (static_cast<size_t>(y) * spec.width + x) * spec.nchannels;
			size_t rgbaIndex = (static_cast<size_t>(y) * spec.width + x) * util::EnumValue(img::Channel::RGBA);
			// Copy RGB channels
			rgbaData[rgbaIndex] = rgbData[rgbIndex];
			rgbaData[rgbaIndex + 1] = rgbData[rgbIndex + 1];
			rgbaData[rgbaIndex + 2] = rgbData[rgbIndex + 2];
		}
	}

	return true;
}

inline img::Format AfterglowImageAsset::Impl::imageFormat(const OIIO::ImageSpec& imageSpec) const {
	switch (imageSpec.format.basetype) {
	case(OIIO::TypeDesc::UINT8): return img::Format::UnsignedInt8;
	case(OIIO::TypeDesc::INT8): return img::Format::Int8;
	case(OIIO::TypeDesc::UINT16): return img::Format::UnsignedInt16;
	case(OIIO::TypeDesc::INT16): return img::Format::Int16;
	case(OIIO::TypeDesc::HALF): return img::Format::Half;
	case(OIIO::TypeDesc::UINT32): return img::Format::UnsignedInt32;
	case(OIIO::TypeDesc::INT32): return img::Format::Int32;
	case(OIIO::TypeDesc::FLOAT): return img::Format::Float;
	case(OIIO::TypeDesc::UINT64): return img::Format::UnsignedInt64;
	case(OIIO::TypeDesc::INT64): return img::Format::Int64;
	case(OIIO::TypeDesc::DOUBLE): return img::Format::Double;
	default:
		EXCEPT_CLASS_RUNTIME("Unknown image format from OpenImageIO.");
	}
}
