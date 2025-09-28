#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "Inreflect.h"

namespace ubo {
	struct GlobalUniform {
		// TODO: PointLightStructure
		alignas(16) glm::vec4 dirLightDirection;
		alignas(16) glm::vec4 dirLightColor;
		alignas(16) glm::vec4 cameraPosition;
		alignas(16) glm::vec4 cameraVector;
		alignas(8) glm::vec2 screenResolution;
		alignas(8) glm::vec2 invScreenResolution;
		alignas(8) glm::vec2 cursorPosition;		// Based on screen window relative pixel, origin[0, 0] at top-left.
		alignas(4) float time;
		alignas(4) float deltaTime;	
		alignas(4) float cameraNear;
		alignas(4) float cameraFar;
		alignas(4) float __padding0;
		alignas(4) float __padding1;
	};

	INR_CLASS(GlobalUniform) {
		INR_ATTRS (
			INR_ATTR(dirLightDirection), 
			INR_ATTR(dirLightColor), 
			INR_ATTR(cameraPosition),
			INR_ATTR(cameraVector),
			INR_ATTR(screenResolution),
			INR_ATTR(invScreenResolution),
			INR_ATTR(cursorPosition),
			INR_ATTR(time), 
			INR_ATTR(deltaTime), 
			INR_ATTR(cameraNear), 
			INR_ATTR(cameraFar) 
		);
	};


	struct MeshUniform {
		// Column-major matrices
		alignas(16) glm::mat4 model;			// Model to World maxtrix
		alignas(16) glm::mat4 view;				// World to Viewport matrix
		alignas(16) glm::mat4 projection;	// Viewport to Clip Space matrix
		alignas(16) glm::mat4 invTransModel; // model.inverse().transpose()
		alignas(16) glm::mat4 invView;
		alignas(16) glm::mat4 invProjection;
		alignas(4) uint32_t objectID;
		alignas(4) float __padding0;
		alignas(4) float __padding1;
		alignas(4) float __padding2;
	};

	INR_CLASS(MeshUniform) {
		INR_ATTRS (
			INR_ATTR(model), 
			INR_ATTR(view), 
			INR_ATTR(projection), 
			INR_ATTR(invTransModel), 
			INR_ATTR(invView), 
			INR_ATTR(invProjection), 
			INR_ATTR(objectID)
		);
	};

	struct MetaInfo {
		std::string type;
		std::string name;
	};

	using MetaInfos = std::vector<MetaInfo>;

	// These Info use for shader declaration info.
	template<typename Type> constexpr auto HLSLTypeName() = delete;
	template<> constexpr auto HLSLTypeName<glm::vec4> () {return "float4"; };
	template<> constexpr auto HLSLTypeName<glm::vec3>() { return "float3"; };
	template<> constexpr auto HLSLTypeName<glm::vec2>() { return "float2"; };
	template<> constexpr auto HLSLTypeName<float>() { return "float"; };
	template<> constexpr auto HLSLTypeName<glm::mat4>() { return "float4x4"; };
	template<> constexpr auto HLSLTypeName<glm::mat3>() { return "float3x3"; };
	template<> constexpr auto HLSLTypeName<glm::mat2>() { return "float2x2"; };
	template<> constexpr auto HLSLTypeName<uint32_t>() { return "uint"; };
	template<> constexpr auto HLSLTypeName<int32_t>() { return "int"; };
}
