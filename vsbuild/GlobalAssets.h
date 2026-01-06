#pragma once
#include "AssetDefinitions.h"
#include "ShaderDefinitions.h"

namespace img {
	static const AssetInfo defaultTextureInfo = {
		.colorSpace = ColorSpace::SRGB,
		.path = "Assets/Shared/Textures/White.png"
	};
}

namespace shader {
	img::AssetInfo GlobalSetBindingTextureInfo(shader::GlobalSetBindingIndex bindingIndex);

	constexpr const char* indirectResetShaderPath = "Shaders/IndirectResetInstanceCount_CS.hlsl";
}

namespace font {
	constexpr const char* defaultFontPath = "Assets/Shared/Fonts/calibrib.ttf";
}
