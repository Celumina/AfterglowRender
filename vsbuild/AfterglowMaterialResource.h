#pragma once

#include "AfterglowMaterialLayout.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowSharedTexturePool.h"
#include "AfterglowStorageBuffer.h"
#include "AfterglowStorageImage.h"
#include "AfterglowUniformBuffer.h"
#include "AfterglowDescriptorSets.h"

class AfterglowSSBOInfo;
class AfterglowDescriptorSetWriter;
class AfterglowDescriptorPool;

class AfterglowMaterialResource {
public:
	using InFlightDescriptorSets = std::array<AfterglowDescriptorSets::AsElement, cfg::maxFrameInFlight>;
	// If texture is not longer exists in material instance, then clear its texture buffer. 
	struct TextureResource {
		TextureResource() = default;
		TextureResource(uint32_t inBindingIndex, std::unique_ptr<AfterglowTextureReference>&& inTextureRef) : 
			bindingIndex(inBindingIndex), textureRef(std::forward<std::unique_ptr<AfterglowTextureReference>>(inTextureRef)) {}

		uint32_t bindingIndex = 0;
		std::array<bool, cfg::maxFrameInFlight> inFlightModifiedFlags{};
		std::unique_ptr<AfterglowTextureReference> textureRef;
	};

	struct StorageBufferResource {
		uint32_t bindingIndex = 0;
		// Using iamge or buffer depend on the ssboInfo.textureFormat
		AfterglowStorageBuffer::AsElement buffer;
		AfterglowStorageImage::AsElement image;
	};

	// Here use std::vector for frame in flight ssbos readwrite buffer.
	using FrameStorageBufferResources = std::vector<StorageBufferResource>;
	using StorageBufferResources = std::unordered_map<std::string, FrameStorageBufferResources>;

	// Resources
	struct StageResource {
		std::vector<AfterglowMaterial::Scalar> uniforms;
		std::array<AfterglowUniformBuffer::AsElement, cfg::maxFrameInFlight> uniformBuffers;
		std::unordered_map<std::string, TextureResource> textureResources;
		StorageBufferResources storageBufferResources;
	};

	struct SpecifiedSSBOResources {
		// For ComputeTask specialize usage reources refs.
		FrameStorageBufferResources* inFlightIndexInputs = nullptr;
		FrameStorageBufferResources* inFlightVertexInputs = nullptr;
		FrameStorageBufferResources* inFlightIndirects = nullptr;
	};

	using StageResources = std::unordered_map<shader::Stage, StageResource>;

	AfterglowMaterialResource(
		AfterglowMaterialLayout& materialLayout, 
		AfterglowDescriptorSetWriter& descriptorSetWriter, 
		AfterglowDescriptorPool& descriptorPool, 
		AfterglowSharedTexturePool& texturePool
	);

	AfterglowDevice& device() noexcept;

	void setMateiralInstance(const AfterglowMaterialInstance& materialInstance) noexcept;

	AfterglowMaterialInstance& materialInstance() noexcept;
	const AfterglowMaterialInstance& materialInstance() const noexcept;

	AfterglowMaterialLayout& materialLayout() noexcept;
	const AfterglowMaterialLayout& materialLayout() const noexcept;

	AfterglowDescriptorSets& descriptorSets();
	const AfterglowDescriptorSets& descriptorSets() const;

	InFlightDescriptorSets& inFlightDescriptorSets() noexcept;
	const InFlightDescriptorSets& inFlightDescriptorSets() const noexcept;

	AfterglowStorageBuffer* indexInputStorageBuffer() noexcept; 
	const AfterglowStorageBuffer* indexInputStorageBuffer() const noexcept;

	AfterglowStorageBuffer* vertexInputStorageBuffer() noexcept;
	const AfterglowStorageBuffer* vertexInputStorageBuffer() const noexcept;

	AfterglowStorageBuffer* indirectStorageBuffer() noexcept;
	const AfterglowStorageBuffer* indirectStorageBuffer() const noexcept;

	// @brief: Reload resources, costly, less call.
	// @deprecated: sepreated into updateUniforms(), updateTextures() and submit.
	// void update(uint32_t frameIndex);

	// @brief: If material layout was changed, call this function.
	void reloadMaterialLayout();

	/**
	* @desc: Find all Compute Stages resource and return the first which is satisfied the ssboName.
	* @return: Target frame storage buffer resource.
	*/
	StorageBufferResource* findStorageBufferResource(const std::string& ssboName, uint32_t frameIndex, uint32_t ssboIndex);

	void updateUniforms(uint32_t frameIndex);
	void updateTextures(uint32_t frameIndex);
	void submitDescriptorSets(uint32_t frameIndex);

private:
	inline TextureResource* aquireTextureResource(shader::Stage stage, const std::string name);

	inline void submitStorageBuffers();

	inline void synchronizeTextures();
	// When the material layout reloaded, do it.
	inline void reregisterUnmodifiedTextures();
	inline void reloadModifiedTextures(uint32_t frameIndex);

	inline void synchronizeStorageBuffers();
	inline void registerFrameStorageBuffers(
		uint32_t frameIndex, 
		StorageBufferResources& ssboResources, 
		AfterglowDescriptorSetLayout& setLayout, 
		VkDescriptorSet& set
	);

	inline AfterglowStorageBuffer* currentFrameStorageBuffer(FrameStorageBufferResources* inFlightResources) noexcept;

	// @param frameIndex: target frame index for descriptor set application. 
	// @param setIndex: [0 <= setIndex < cfg::maxFrameInFlight] : readonly; [setIndex == cfg::maxFrameInFlight] : writeonly;
	static inline uint32_t frameStorageResourceIndex(uint32_t frameIndex, uint32_t ssboIndex, bool multipleSSBOs) noexcept;

	static inline VkFormat computeTextureFormat(const AfterglowSSBOInfo& ssboInfo) noexcept;
	static inline VkExtent3D computeTextureExtent(const AfterglowSSBOInfo& ssboInfo) noexcept;


	AfterglowDescriptorSetWriter& _descriptorSetWriter;
	AfterglowMaterialLayout& _materialLayout;

	AfterglowMaterialInstance _materialInstance;
	InFlightDescriptorSets _inFlightDescriptorSets;
	// This raw array including meshSetLayout
	StageResources _stageResources;
	
	AfterglowDescriptorPool& _descriptorPool;
	AfterglowSharedTexturePool& _texturePool;

	bool _shouldReregisterTextures;

	std::unique_ptr<SpecifiedSSBOResources> _specifiedSSBOResources;
};

