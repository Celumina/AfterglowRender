#pragma once
#include "AfterglowMaterialLayout.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowDescriptorSetWriter.h"
#include "AfterglowSharedTexturePool.h"
#include "AfterglowStorageBuffer.h"

// TODO: Storage Buffer support.

class AfterglowMaterialResource {
public:
	using InFlightDescriptorSets = std::array<AfterglowDescriptorSets::AsElement, cfg::maxFrameInFlight>;
	// If texture is not longer exists in material instance, then clear its texture buffer. 
	struct TextureResource {
		uint32_t bindingIndex;
		std::unique_ptr<AfterglowTextureReference> textureRef;
	};

	struct StorageBufferResource {
		uint32_t bindingIndex;
		AfterglowStorageBuffer::AsElement buffer;
	};

	// Resources
	struct StageResource {
		// Here use std::vector for frame in flight ssbos.
		using StorageBufferResources = std::unordered_map<std::string, std::vector<StorageBufferResource>>;

		std::vector<AfterglowMaterial::Scalar> uniforms;
		std::array<AfterglowUniformBuffer::AsElement, cfg::maxFrameInFlight> uniformBuffers;
		std::unordered_map<std::string, TextureResource> textureResources;
		StorageBufferResources storageBufferResources;
	};

	using StageResources = std::unordered_map<shader::Stage, StageResource>;

	AfterglowMaterialResource(
		AfterglowMaterialLayout& materialLayout, 
		AfterglowDescriptorSetWriter& descriptorSetWriter, 
		AfterglowSharedTexturePool& texturePool
	);

	AfterglowDevice& device();

	void setMateiralInstance(const AfterglowMaterialInstance& materialInstance);

	AfterglowMaterialInstance& materialInstance();
	const AfterglowMaterialInstance& materialInstance() const;

	AfterglowMaterialLayout& materialLayout();
	const AfterglowMaterialLayout& materialLayout() const;


	AfterglowDescriptorSets& descriptorSets();
	const AfterglowDescriptorSets& descriptorSets() const;

	InFlightDescriptorSets& inFlightDescriptorSets();
	const InFlightDescriptorSets& inFlightDescriptorSets() const;

	AfterglowStorageBuffer* vertexInputStorageBuffer();
	const AfterglowStorageBuffer* vertexInputStorageBuffer() const;

	// @brief: Reload resources, costly, less call.
	void update();

	// @brief: If material layout was changed, call this function.
	void reloadMaterialLayout(AfterglowDescriptorPool& descriptorPool);

private:
	inline TextureResource* aquireTextureResource(shader::Stage stage, const std::string name);

	inline void submitUniforms();
	inline void submitTextures();
	inline void submitStorageBuffers();

	inline void synchronizeTextures();
	// When the material layout reloaded, do it.
	inline void reregisterUnmodifiedTextures();
	inline void reloadModifiedTextures();

	inline void synchronizeStorageBuffers();
	inline void registerFrameStorageBuffers(
		uint32_t frameIndex, 
		StageResource::StorageBufferResources& ssboResources, 
		AfterglowDescriptorSetLayout& setLayout, 
		VkDescriptorSet& set
	);

	AfterglowDescriptorSetWriter& _descriptorSetWriter;
	AfterglowMaterialLayout& _materialLayout;

	AfterglowMaterialInstance _materialInstance;
	InFlightDescriptorSets _inFlightDescriptorSets;
	// This raw array including meshSetLayout
	StageResources _stageResources;
	
	AfterglowSharedTexturePool& _texturePool;

	bool _shouldReregisterTextures;
};

