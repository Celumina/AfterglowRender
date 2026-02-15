#pragma once

#include <memory>
#include <mutex>


#include "UniformBufferObjects.h"
#include "AfterglowImage.h"
#include "AfterglowUtilities.h"
#include "AfterglowMaterialInstance.h"
#include "AfterglowPassInterface.h"


class AfterglowSynchronizer;
class AfterglowMaterialAsset;
class AfterglowCommandPool;
class AfterglowGraphicsQueue;
class AfterglowPassManager;
class AfterglowAssetMonitor;
class AfterglowDevice;
class AfterglowDescriptorPool;
class AfterglowDescriptorSetWriter;
class AfterglowMaterialLayout;
class AfterglowMaterialResource;
class AfterglowDescriptorSetReferences;
class AfterglowSharedTexturePool;

class AfterglowMaterialManager : public AfterglowObject {
public:
	using LockGuard = std::lock_guard<std::mutex>;

	AfterglowMaterialManager(
		AfterglowCommandPool& commandPool, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowPassManager& passManager, 
		AfterglowAssetMonitor& assetMonitor, 
		AfterglowSynchronizer& synchronizer
	);
	~AfterglowMaterialManager();

	void initGlobalDescriptorSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages);

	AfterglowDevice& device() noexcept;
	AfterglowPassManager& passManager() noexcept;
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

	/**
	* @brief: Remove material and its instances.
	* @thread_safety
	*/
	void removeMaterial(const std::string& name);

	/**
	* @return: Ture if remove successfully.
	* @thread_safety
	*/
	void removeMaterialInstance(const std::string& name);

	// @return: Material handle;
	AfterglowMaterial* material(const std::string& name);
	// @return: Parent material from material instance name.
	AfterglowMaterial* findMaterialByInstanceName(const std::string& name);

	// @return: MaterialLayout handle;
	AfterglowMaterialLayout* materialLayout(const std::string& name);
	const AfterglowMaterialLayout* materialLayout(const std::string& name) const;

	// @return: MaterialInstance handle;
	AfterglowMaterialInstance* materialInstance(const std::string& name);

	// @return: MaterialResource handle;
	AfterglowMaterialResource* materialResource(const std::string& name);
	const AfterglowMaterialResource* materialResource(const std::string& name) const;


	/**
	* @brief: Apply material info to descriptors manually.
	* @param: name MaterialContext's name.
	* @return: true if update succefully.
	* @thread-safety
	*/
	bool submitMaterial(const std::string& name);

	/**
	* @brief: Apply material info to descriptors manually.
	* @param: name MaterialInstanceContext's name.
	* @return: true if update succefully.
	* @thred-safety
	*/
	bool submitMaterialInstance(const std::string& name);
	/**
	* @brief: Submit uniform buffer of material resource (exclude textures)
	* @thread-safety
	*/ 
	bool submitMaterialInstanceUniformParams(const std::string& name);
	/**
	* @brief: Submit textures of material resource (exclude uniform buffer)
	* @thread-safety
	*/
	bool submitMaterialInstanceTextureParams(const std::string& name);

	/**
	* @brief: set and update mesh uniform to material instance.
	* @note: If material instance is not exists, it will initialize form material automatically.
	* @param: name from material (instance).
	* @return: true if materialInstanceContext exists and set successfully.
	*/
	bool submitMeshUniform(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform);

	// @brief: write descritptor sets to device.
	void updateMaterials(
		render::PassUnorderedMap<img::ImageReferences>& allPassImages, 
		bool swapchainImageSetOutdated
	);
	// @brief: Update it when the GPU is not in flight.
	void updateResources();

	AfterglowMaterialResource& errorMaterialResource();

	ubo::GlobalUniform& globalUniform() noexcept;

	// @brief: Per object set data.
	AfterglowDescriptorSetReferences* descriptorSetReferences(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform);

	// @warning: These function do not check validation of material layout and material resource. 
	void applyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset, bool useGlobalResources = true);
	void applyErrorShaders(AfterglowMaterialLayout& matLayout);
	void safeApplyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset) noexcept;

	AfterglowSharedTexturePool& texturePool() noexcept;

	template<typename FuncType, typename ...ParamTypes>
	void lockedAccess(FuncType&& func, ParamTypes&& ...params);

	// @warning: Make sure it be invoked in render thread only, and never invoke it between reset() and queue submit().
	void waitGPU() const;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;

	mutable std::mutex _mutex;
};

template<typename FuncType, typename ...ParamTypes>
inline void AfterglowMaterialManager::lockedAccess(FuncType&& func, ParamTypes && ...params) {
	LockGuard lockGuard{ _mutex };
	func(std::forward<ParamTypes>(params)...);
}
