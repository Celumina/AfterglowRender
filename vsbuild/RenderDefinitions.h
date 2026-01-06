#pragma once

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

namespace render {
	// Render subpasses in this order.
	enum class Domain : uint32_t {
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

	enum class Topology : uint32_t {
		Undefined, 

		PointList,
		LineList,
		LineStrip, 
		TriangleList, 
		TriangleStrip, 
		PatchList, 

		EnumCount
	};

	enum class InputAttachmentType : uint32_t {
		Color, 
		Depth, 
		DepthStencil
	};
	std::string HLSLTexturePixelTypeName(InputAttachmentType inputAttachment);

	struct InputAttachmentInfo {
		// attachment which is refered.
		Domain domain;
		uint32_t attachmentIndex;		
		InputAttachmentType type;
		bool isMultiSample;
		std::string name;
	};

	using InputAttachmentInfos = std::vector<InputAttachmentInfo>;
}