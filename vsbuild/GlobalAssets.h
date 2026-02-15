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
	using Text = const char*;

	img::AssetInfo GlobalSetBindingTextureInfo(shader::GlobalSetBindingIndex bindingIndex);
	
	// VS: Vertex Shader, FS: Fragment Shader,  CS: Compute Shader.
	constexpr Text errorVSPath = "Shaders/Error_VS.hlsl";
	constexpr Text errorFSPath = "Shaders/Error_FS.hlsl";
	constexpr Text errorCSPath = "Shaders/Error_CS.hlsl";
	constexpr Text defaultForwardVSPath = "Shaders/Unlit_VS.hlsl";
	constexpr Text defaultForwardFSPath = "Shaders/Unlit_FS.hlsl";
	constexpr Text emptyPostprocessVSPath = "Shaders/EmptyPostProcess_VS.hlsl";
	constexpr Text emptyPostprocessFSPath = "Shaders/EmptyPostProcess_FS.hlsl";

	constexpr Text indirectResetCSPath = "Shaders/IndirectResetInstanceCount_CS.hlsl";
}

namespace font {
	constexpr const char* defaultFontPath = "Assets/Shared/Fonts/calibrib.ttf";
}
