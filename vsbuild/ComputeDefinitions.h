#pragma once
#include <stdint.h>
#include "Inreflect.h"

namespace compute {
	enum class DispatchFrequency : uint16_t {
		Never,         // Submit to command buffer manually
		Once,          // Submit once when compute resource is initialized, or you can set it on manually.
		PerFrame       // Submit every update
		// TODO:  Fixed update rate
	};

	INR_CLASS(DispatchFrequency) {
		INR_ATTRS (
			INR_ENUM(Never), 
			INR_ENUM(Once), 
			INR_ENUM(PerFrame)
		);
	};

	enum class SSBOAccessMode : uint16_t {
		Undefined, 
		ReadOnly,
		ReadWrite, 

		EnumCount
	};

	enum class SSBOInitMode : uint16_t {
		Zero, 
		StructuredData,       // TODO: Custom structured data by definition.
		Model,                // TODO
		Texture,              // TODO
		InternalFunction,     // Temporary solution.
		ComputeShader,        // Init SSBO from another compute shader.

		EnumCount
	};

	enum class SSBOUsage : uint16_t {
		Buffer,
		// IndexCount for square = ((sideSize - 1) ^ 2) * 6, sideSize - 1 is cellSideSize.
		IndexInput, 
		VertexInput,
		Instancing,
		Indirect, 

		EnumCount
	};

	enum class SSBOTextureDimension : uint16_t {
		Undefined, 
		Texture1D, 
		Texture2D, 
		Texture3D, 

		EnumCount
	}; 

	enum class SSBOTextureSampleMode : uint16_t {
		LinearRepeat, 
		LinearClamp, 
		// TODO: More sample modes support if it's required.

		EnumCount
	};

	enum class SSBOTextureMode : uint16_t {
		Unused, 

		// 4 bytes is the minimum structed layout element size.
		// 8 bits per channel
		RGBA8Uint,
		RGBA8Unorm,
		RGBA8Snorm, 

		// 16 bits per channel
		RG16Float, 
		RGBA16Float, 

		// 32 bits per channel
		R32Float, 
		RG32Float, 
		RGB32Float,
		RGBA32Float,

		EnumCount
	};

	INR_CLASS(SSBOTextureMode) {
		INR_ATTRS (
			INR_ENUM(Unused), 
			INR_ENUM(RGBA8Uint),
			INR_ENUM(RGBA8Unorm),
			INR_ENUM(RGBA8Snorm),
			INR_ENUM(RG16Float),
			INR_ENUM(RGBA16Float),
			INR_ENUM(R32Float),
			INR_ENUM(RG32Float),
			INR_ENUM(RGB32Float),
			INR_ENUM(RGBA32Float)
		);
	};

	std::string HLSLTypeName(SSBOTextureMode textureMode) noexcept;
	uint32_t TexelByteSize(SSBOTextureMode textureMode) noexcept;

	struct DispatchGroup {
		uint32_t x = 1;
		uint32_t y = 1;
		uint32_t z = 1;
	};

	constexpr const char* indirectSSBOName = "IndirectBuffer";
}