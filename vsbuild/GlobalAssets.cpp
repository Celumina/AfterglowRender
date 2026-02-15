#include "GlobalAssets.h"
#include "ExceptionUtilities.h"

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
	EXCEPT_RUNTIME("TexturePath of this binding index is not defined.");
}
