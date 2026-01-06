#include "ComputeDefinitions.h"
#include "AfterglowUtilities.h"
#include "DebugUtilities.h"

std::string compute::HLSLTypeName(SSBOTextureMode textureMode) noexcept {
	switch (textureMode) {
	case compute::SSBOTextureMode::RGBA8Uint: return "uint4";
	case compute::SSBOTextureMode::RGBA8Unorm: return "unorm float4";
	case compute::SSBOTextureMode::RGBA8Snorm: return "snorm float4";
	case compute::SSBOTextureMode::RG16Float: return "min16float2"; 
	case compute::SSBOTextureMode::RGBA16Float: return "min16float4";
	case compute::SSBOTextureMode::R32Float: return "float";
	case compute::SSBOTextureMode::RG32Float: return "float2";
	case compute::SSBOTextureMode::RGB32Float: return "float3";
	case compute::SSBOTextureMode::RGBA32Float: return "float4";
	default:
		DEBUG_WARNING("[compute::hlslTypeName] Texture mode is unused, it will return a void type name.");
		return "";
	}
}

uint32_t compute::TexelByteSize(SSBOTextureMode textureMode) noexcept {
	
	switch (textureMode) {
	case compute::SSBOTextureMode::RGBA8Uint: return 4;
	case compute::SSBOTextureMode::RGBA8Unorm: return 4;
	case compute::SSBOTextureMode::RGBA8Snorm: return 4;
	case compute::SSBOTextureMode::RG16Float: return 4;
	case compute::SSBOTextureMode::RGBA16Float: return 8;
	case compute::SSBOTextureMode::R32Float: return 4;
	case compute::SSBOTextureMode::RG32Float: return 8;
	case compute::SSBOTextureMode::RGB32Float: return 12;
	case compute::SSBOTextureMode::RGBA32Float: return 16;
	default: 
		DEBUG_WARNING("[compute::TexelByteSize] Parameter textureMode is not a valid textue format. ");
		return 0;
	}
}

