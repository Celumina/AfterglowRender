#pragma once

#include <map>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

#include "AfterglowAssetMonitor.h"
#include "AfterglowMaterialAsset.h"

#include "AfterglowMaterialResource.h"
#include "AfterglowDescriptorSetWriter.h"
#include "AfterglowDescriptorSetReferences.h"

#include "AfterglowSynchronizer.h"
#include "UniformBufferObjects.h"

class AfterglowMaterialManager : public AfterglowObject {
public:
	using LockGuard = std::lock_guard<std::mutex>;
	using UniqueLock = std::unique_lock<std::mutex>;

	using MaterialLayouts = std::unordered_map<std::string, AfterglowMaterialLayout>;
	using MaterialResources = std::unordered_map<std::string, AfterglowMaterialResource>;

	struct PerObjectSetContext {
		const ubo::MeshUniform* meshUniform = nullptr;
		std::array<AfterglowUniformBuffer::AsElement, cfg::maxFrameInFlight> inFlightBuffers;
		// This set allocate own mesh buffer only
		std::array<AfterglowDescriptorSets::AsElement, cfg::maxFrameInFlight> inFlightSets;
		std::array<AfterglowDescriptorSetReferences, cfg::maxFrameInFlight> inFlightSetReferences;
		std::array<bool, cfg::maxFrameInFlight> inFlightMaterialChangedFlags;
		bool activated = false;
	};

	// Use to apply commands.
	struct ComputePerObjectSetContext {
		std::array<AfterglowDescriptorSetReferences, cfg::maxFrameInFlight> inFlightComputeSetReferences;
	};

	struct GlobalSetContext {
		ubo::GlobalUniform globalUniform {};
		std::vector<AfterglowMaterialResource::TextureResource> textureResources;
		std::array<AfterglowUniformBuffer::AsElement, cfg::maxFrameInFlight> inFlightBuffers;
		std::array<AfterglowDescriptorSets::AsElement, cfg::maxFrameInFlight> inFlightSets;
		// For compute shader to avoid synchronization conflition.
		std::array<AfterglowDescriptorSets::AsElement, cfg::maxFrameInFlight> inFlightComputeSets;
	};

	// One material instance support multi objects.
	using PerObjectSetContextArray = std::vector<PerObjectSetContext>;
	using MaterialPerObjectSetContexts = std::unordered_map<AfterglowMaterialResource*, PerObjectSetContextArray>;
	// Compute material usually have not material instance.
	using ComputeMaterialPerObjectSetContexts = std::unordered_map<AfterglowMaterialResource*, ComputePerObjectSetContext>;

	using DatedMaterialLayouts = std::unordered_set<AfterglowMaterialLayout*>;
	using DatedMaterialResources = std::unordered_set<AfterglowMaterialResource*>;
	using DatedPerObjectSetContexts = std::unordered_map<AfterglowMaterialResource*, PerObjectSetContextArray*>;

	AfterglowMaterialManager(
		AfterglowCommandPool& commandPool, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowRenderPass& renderPass, 
		AfterglowAssetMonitor& assetMonitor
	);

	AfterglowDevice& device();
	AfterglowDescriptorSetWriter& descriptorSetWriter();

	// @return: created material name of this asset.
	std::string registerMaterialAsset(const std::string& materialPath);
	// @return: created material instance name of this asset.
	std::string registerMaterialInstanceAsset(const std::string& materialInstancePath);

	void unregisterMaterialAsset(const std::string& materialPath);
	void unregisterMaterialInstanceAsset(const std::string& materialInstancePath);

	/**
	* @brief: Create material by name, if name exists, replace old material by new one.
	* @return: Material handle;
	* @thread_safety
	*/
	AfterglowMaterial& createMaterial(const std::string& name, const AfterglowMaterial& sourceMaterial = AfterglowMaterial::emptyMaterial());

	/**
	* @brief: Create materialInstance by name, if name exists, replace old material by new one.
	* @desc: If parent not in this manager, manager will create a empty same named material for the instance.
	* @return: Material Insrtance handle;
	* @thread_safety
	*/
	AfterglowMaterialInstance& createMaterialInstance(const std::string& name, const std::string& parentMaterialName);

	// @return: Material handle;
	AfterglowMaterial* material(const std::string& name);

	// @return: MaterialInstance  handle;
	AfterglowMaterialInstance* materialInstance(const std::string& name);

	// @return: MaterialResource  handle;
	AfterglowMaterialResource* materialResource(const std::string& name);
	const AfterglowMaterialResource* materialResource(const std::string& name) const;

	// @brief: If static mesh have a invalid material instance name, use this material instance.
	static AfterglowMaterialAsset& errorMaterialAsset();
	static const std::string&  errorMaterialInstanceName();

	/**
	* @brief: Remove material and its instances.
	* @return: Ture if remove successfully.
	* @thread_safety
	*/
	bool removeMaterial(const std::string& name);

	/**
	* @return: Ture if remove successfully.
	* @thread_safety
	*/
	bool removeMaterialInstance(const std::string& name);

	/**
	* @brief: Apply material info to descriptors manually.
	* @param: name MaterialContext's name.
	* @return: true if update succefully.
	*/
	bool submitMaterial(const std::string& name);

	/**
	* @brief: Apply material info to descriptors manually.
	* @param: name MaterialInstanceContext's name.
	* @return: true if update succefully.
	*/
	bool submitMaterialInstance(const std::string& name);

	/**
	* @brief: set and update mesh uniform to material instance.
	* @note: If material instance is not exists, it will initialize form material automatically.
	* @param: name MaterialInstanceContext's name.
	* @return: true if materialInstanceContext exists and set successfully.
	*/
	bool submitMeshUniform(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform);

	// @brief: write descritptor sets to device.
	void update(img::WriteInfoArray& imageWriteInfos, AfterglowSynchronizer& synchronizer);
	// @brief: update compute descriptor sets and pipelines.
	void updateCompute();

	AfterglowMaterialResource& errorMaterialResource();

	ubo::GlobalUniform& globalUniform();

	// @brief: Per object set data.
	AfterglowDescriptorSetReferences* descriptorSetReferences(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform);
	// @brief: For compute record.
	AfterglowDescriptorSetReferences* computeDescriptorSetReferences(const std::string& materialName, const ubo::MeshUniform& meshUniform);


	UniqueLock lock() const;

private:
	inline AfterglowMaterialInstance& createMaterialInstanceWithoutLock(const std::string& name, const std::string& parentMaterialName);
	inline bool instantializeMaterial(const std::string& name);

	inline void initGlobalDescriptorSet();

	inline void initAssetMonitorCallbacks();
	inline void createMaterialFromAsset(const AfterglowMaterialAsset& materialAsset);

	// Call it when that material submit.
	inline void reloadMaterialResources(AfterglowMaterialLayout& matLayout);

	inline void applyMaterialLayout(AfterglowMaterialLayout& matLayout);
	inline void applyMaterialResource(AfterglowMaterialResource& matResource);
	inline void applyExternalSetContext(AfterglowMaterialResource& matResource, PerObjectSetContextArray& perObjectSetContexts);
	inline void applyGlobalSetContext(img::WriteInfoArray& imageWriteInfos);

	inline void applyErrorShaders(AfterglowMaterialLayout& matLayout);
	inline void appendGlobalSetTextureResource(shader::GlobalSetBindingIndex textureBindingIndex);

	inline PerObjectSetContextArray* perObjectSetContexts(AfterglowMaterialResource* matResource);

	inline void applyShaders(AfterglowMaterialLayout& matLayout, AfterglowMaterialAsset& matAsset);

	// Static Pool Size
	// TODO: add a new pool for dynamic pool size.
	// Place front to make sure descriptor set destruct later than descriptor sets.
	AfterglowDescriptorPool::AsElement _descriptorPool;
	AfterglowSharedTexturePool _texturePool;

	// TODO: Different subpass as material domain.
	AfterglowRenderPass& _renderPass;
	AfterglowAssetMonitor& _assetMonitor;

	MaterialLayouts _materialLayouts;
	MaterialResources _materialResources;

	// Global Set Layout
	AfterglowDescriptorSetLayout::AsElement _globalDescriptorSetLayout;
	// Global Uniform Resources
	GlobalSetContext _globalSetContext;

	// PerObject Set Layout
	AfterglowDescriptorSetLayout::AsElement _perObjectDescriptorSetLayout;
	// Mesh Uniform Resources
	MaterialPerObjectSetContexts _materialPerObjectSetContexts;
	ComputeMaterialPerObjectSetContexts _computeMaterialPerObjectSetContexts;

	AfterglowDescriptorSetWriter _descriptorSetWriter;

	DatedMaterialLayouts _datedMaterialLayouts;
	DatedMaterialResources _datedMaterialResources;
	DatedPerObjectSetContexts _datedPerObjectSetContexts;

	mutable std::mutex _mutex;

	// Error material.
	static constexpr const char* _errorMaterialName = "__ERROR__";
};

