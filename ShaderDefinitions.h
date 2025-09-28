#pragma once
#include <stdint.h>
#include "Inreflect.h"

namespace shader {
	enum class SetIndex : uint32_t {
		Global = 0,
		PerObject = 1, 
		MaterialVertex = 2,
		MaterialFragment = 3,
		MaterialShared = 4,

		Compute = 5, 
		ComputeVertex = 6,
		ComputeFragment = 7, 

		EnumCount
	};

	INR_CLASS(SetIndex) {
		INR_ATTRS(
			INR_ENUM(Global), 
			INR_ENUM(PerObject),
			INR_ENUM(MaterialVertex),
			INR_ENUM(MaterialFragment),
			INR_ENUM(MaterialShared), 
			INR_ENUM(Compute), 
			INR_ENUM(ComputeVertex),
			INR_ENUM(ComputeFragment)
		);
	};

	constexpr auto MaterialSetIndexBegin = std::underlying_type_t<SetIndex>(SetIndex::MaterialVertex);
	constexpr auto MaterialSetIndexEnd = std::underlying_type_t<SetIndex>(SetIndex::MaterialShared) + 1;
	constexpr auto ComputeSetIndexEnd = std::underlying_type_t<SetIndex>(SetIndex::ComputeFragment) + 1;

	// ShaderStage keep same value with ShaderDescriptorSet
	enum class Stage : uint32_t {
		Undefined = 0, 
		Vertex = SetIndex::MaterialVertex,
		Fragment = SetIndex::MaterialFragment,
		Shared = SetIndex::MaterialShared,

		Compute = SetIndex::Compute, 
		ComputeVertex = SetIndex::ComputeVertex,
		ComputeFragment = SetIndex::ComputeFragment,

		EnumCount
	};

	INR_CLASS(Stage) {
		INR_ATTRS (
			INR_ENUM(Vertex), 
			INR_ENUM(Fragment), 
			INR_ENUM(Shared), 
			INR_ENUM(Compute), 
			INR_ENUM(ComputeVertex), 
			INR_ENUM(ComputeFragment)
		);
	};

	constexpr auto MaterialStageBegin = std::underlying_type_t<Stage>(Stage::Vertex);

	enum class GlobalSetBindingResource : uint32_t {
		Uniform, 
		Sampler, 
		Texture, 

		EnumCount
	};

	INR_CLASS(GlobalSetBindingResource) {
		INR_ATTRS(
			INR_ENUM(Uniform),
			INR_ENUM(Sampler), 
			INR_ENUM(Texture)
		);
	};

	// Remaind that these enum name suffix is important, 
	// MaterialManager load these enums to build global set automatically.
	// Supported Suffix (GlobalSetBindingResource):
	// Uniform
	// Sampler
	// Texture
	enum class GlobalSetBindingIndex : uint32_t {
		GlobalUniform = 0,

		// Global textures
		// Here enum use begin lower case due to these names will automatically reflect to shader variables which comform with lower case standard.
		ambientTexture = 1, 

		// TODO: Shared samplers

		EnumCount

		// Attachment texture bindings put in here.
		// binding[EnumCount] ~ binding[n]
	};

	INR_CLASS(GlobalSetBindingIndex) {
		INR_ATTRS (
			INR_ENUM(GlobalUniform), 
			INR_ENUM(ambientTexture)
		);
	};

	enum class  PerObjectSetBindingIndex : uint32_t {
		MeshUniform = 0, 
		
		EnumCount
	};

	uint32_t AttachmentTextureBindingIndex(uint32_t inputAttachmentLocalIndex);
}
