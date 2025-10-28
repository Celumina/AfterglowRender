#pragma once

#include <memory>
#include <mutex>


#include "UniformBufferObjects.h"
#include "AfterglowImage.h"
#include "AfterglowUtilities.h"
#include "AfterglowMaterialInstance.h"


class AfterglowSynchronizer;
class AfterglowMaterialAsset;
class AfterglowCommandPool;
class AfterglowGraphicsQueue;
class AfterglowRenderPass;
class AfterglowAssetMonitor;
class AfterglowDevice;
class AfterglowDescriptorPool;
class AfterglowDescriptorSetWriter;
class AfterglowMaterialLayout;
class AfterglowMaterialResource;
class AfterglowDescriptorSetReferences;

class AfterglowMaterialManager : public AfterglowObject {
public:
	using LockGuard = std::lock_guard<std::mutex>;

	AfterglowMaterialManager(
		AfterglowCommandPool& commandPool, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowRenderPass& renderPass, 
		AfterglowAssetMonitor& assetMonitor
	);
	~AfterglowMaterialManager();

	AfterglowDevice& device() noexcept;
	AfterglowDescriptorPool& descriptorPool();
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
	* @param materialAsset [optional]: For existing material to reapply shaders;
	* @thread_safety
	*/
	AfterglowMaterial& createMaterial(
		const std::string& name, 
		util::OptionalRef<AfterglowMaterial> sourceMaterial = std::nullopt, 
		util::OptionalRef<AfterglowMaterialAsset> materialAsset = std::nullopt
	);

	/**
	* @brief: Create materialInstance by name, if name exists, replace old material by new one.
	* @desc: If parent not in this manager, manager will create a empty same named material for the instance.
	* @return: Material Insrtance handle;
	* @thread_safety
	*/
	AfterglowMaterialInstance& createMaterialInstance(const std::string& name, const std::string& parentMaterialName);

	// @return: Material handle;
	AfterglowMaterial* material(const std::string& name);

	// @return: MaterialLayout handle;
	AfterglowMaterialLayout* materialLayout(const std::string& name);
	const AfterglowMaterialLayout* materialLayout(const std::string& name) const;

	// @return: MaterialInstance handle;
	AfterglowMaterialInstance* materialInstance(const std::string& name);

	// @return: MaterialResource handle;
	AfterglowMaterialResource* materialResource(const std::string& name);
	const AfterglowMaterialResource* materialResource(const std::string& name) const;

	// @brief: If static mesh have a invalid material instance name, use this material instance.
	static AfterglowMaterialAsset& errorMaterialAsset();
	static const std::string& errorMaterialName();
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

	ubo::GlobalUniform& globalUniform() noexcept;

	// @brief: Per object set data.
	AfterglowDescriptorSetReferences* descriptorSetReferences(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform);
	// @brief: For compute record.
	AfterglowDescriptorSetReferences* computeDescriptorSetReferences(const std::string& materialName, const ubo::MeshUniform& meshUniform);

	void applyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset);
	void applyErrorShaders(AfterglowMaterialLayout& matLayout);

	std::mutex& mutex();

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;

	mutable std::mutex _mutex;

	// Error material.
	static constexpr const char* _errorMaterialName = "__ERROR__";
};

