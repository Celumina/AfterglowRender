#pragma once
#include "AssetDefinitions.h"
#include "ShaderDefinitions.h"

namespace img {
	static const AssetInfo defaultTextureInfo = {
		.format = Format::RGBA, 
		.colorSpace = ColorSpace::SRGB,
		.path = "Assets/Shared/Textures/White.png"
	};


}

namespace shader {
	img::AssetInfo GlobalSetBindingTextureInfo(shader::GlobalSetBindingIndex bindingIndex);
}

namespace font {
	static constexpr const char* defaultFontPath = "Assets/Shared/Fonts/calibrib.ttf";
}
