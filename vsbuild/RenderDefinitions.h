#pragma once

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "Inreflect.h"

/**
* RenderPass and RenderPipeline definitions.
*/

namespace render {
	// Render subpasses in this order.
	enum class Domain : uint16_t {
		Undefined, 

		DepthPrepass,
		Shadow,
		DeferredGeometry,
		Decal,
		DeferredLighting,
		Forward,
		Transparency,
		PostProcess,
		UserInterface,

		EnumCount	// Explicit enum count mark.
	};

	INR_CLASS(Domain) {
		INR_ATTRS(
			INR_ENUM(Undefined),
			INR_ENUM(DepthPrepass), 
			INR_ENUM(Shadow), 
			INR_ENUM(DeferredGeometry), 
			INR_ENUM(Decal), 
			INR_ENUM(DeferredLighting), 
			INR_ENUM(Forward), 
			INR_ENUM(Transparency), 
			INR_ENUM(PostProcess), 
			INR_ENUM(UserInterface)
		);
	};

	enum class Topology : uint16_t {
		Undefined, 

		PointList,
		LineList,
		LineStrip, 
		TriangleList, 
		TriangleStrip, 
		PatchList, 

		EnumCount
	};

	enum class CullMode : uint16_t {
		None,  // Two sided
		Front, 
		Back,  // Default
		FrontBack, 

		EnumCount
	};

	enum class StencilOperation : uint16_t {
		Keep, 
		Zero, 
		Replace, 
		IncrementClamp, // + 1
		DecrementClamp, // - 1
		Invert, 
		IncrementWrap, 
		DecrementWrap, 

		EnumCount
	};

	enum class CompareOperation : uint16_t {
		AlwaysFalse,
		Less, 
		Equal, 
		LessEqual, 
		Greater, 
		NotEqual, 
		GreaterEqual, 
		AlwaysTrue,

		EnumCount
	};

	struct StencilInfo {
		uint8_t stencilValue = 0;
		uint8_t compareMask = 0xFF;
		uint8_t writeMask = 0xFF;
		CompareOperation compareOperation = CompareOperation::AlwaysTrue;
		StencilOperation failOperation = StencilOperation::Keep;
		StencilOperation passOperation = StencilOperation::Keep;
		StencilOperation depthFailOperation = StencilOperation::Keep;
	};

	struct FaceStencilInfos {
		StencilInfo front{};
		StencilInfo back{};
	};

	enum class AttachmentType : uint16_t {
		Color, 
		Depth, 
		DepthStencil, 

		EnumCount
	};
	std::string HLSLTexturePixelTypeName(AttachmentType inputAttachment);

	struct InputAttachmentInfo {
		// attachment which is refered.
		uint32_t attachmentIndex;
		AttachmentType type;
		bool isMultiSample;
		std::string name;
	};

	using InputAttachmentInfos = std::vector<InputAttachmentInfo>;

	// Genernal pass export attachmenet names
	constexpr const char* sceneColorMSTextureName = "sceneColorMSTexture"; // MS: Multiple sample
	constexpr const char* sceneColorTextureName = "sceneColorTexture";
	constexpr const char* depthTextureName = "depthTexture";
}