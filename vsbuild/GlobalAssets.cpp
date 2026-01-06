#include "GlobalAssets.h"
#include <stdexcept>

img::AssetInfo shader::GlobalSetBindingTextureInfo(shader::GlobalSetBindingIndex bindingIndex) {
	switch (bindingIndex) {
	case shader::GlobalSetBindingIndex::ambientTexture:
		return {
			.colorSpace = img::ColorSpace::SRGB, 
			.path = "Assets/Shared/Textures/DefaultSky.png"
		};
	case shader::GlobalSetBindingIndex::diffuseEnergyTexture:
		return {
			.colorSpace = img::ColorSpace::Linear,
			.path = "Assets/Shared/Textures/EnergyConservation/Diffuse_Energy.EXR"
		};
	}
	throw std::runtime_error("TexturePath of this binding index is not defined.");
}
