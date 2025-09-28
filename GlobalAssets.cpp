#include "GlobalAssets.h"
#include <stdexcept>

img::AssetInfo shader::GlobalSetBindingTextureInfo(shader::GlobalSetBindingIndex bindingIndex) {
	switch (bindingIndex) {
	case shader::GlobalSetBindingIndex::ambientTexture:
		return {
			.format = img::Format::RGBA, 
			.colorSpace = img::ColorSpace::SRGB, 
			.path = "Assets/Shared/Textures/DefaultSky.png"
		};
	}
	throw std::runtime_error("TexturePath of this binding index is not defined.");
}
