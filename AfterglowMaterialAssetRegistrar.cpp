#include "AfterglowMaterialAssetRegistrar.h"

#include "AfterglowMaterialManager.h"
#include "AfterglowAssetMonitor.h"
#include "AfterglowMaterialAsset.h"
#include "AfterglowMaterialInstanceAsset.h"
#include "AfterglowMaterialLayout.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowComputeTask.h"


// TODO: Remove build files from SVN

struct AfterglowMaterialAssetRegistrar::Impl {
	using UpdateShaderAssetReferencesFunc = void(Impl::*)(const std::string&, const std::string&);

	Impl(AfterglowMaterialManager& materialManagerRef, AfterglowAssetMonitor& assetMonitorRef);

	inline void initAssetMonitorCallbacks();

	static void modifiedMateiralAssetCallback(Impl& context, const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos);
	static void modifiedMateiralInstanceAssetCallback(Impl& context, const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos);
	static void modifiedShaderAssetCallback(Impl& context, const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos);

	inline void createMaterialFromAsset(const AfterglowMaterialAsset& materialAsset);

	// When the material reference == 0, remove shader asset.
	inline void increaseShaderAssetReference(const std::string& assetPath, const std::string& materialName);
	inline void decreaseShaderAssetReference(const std::string& assetPath, const std::string& materialName);

	inline void updateShaderAssetReferences(UpdateShaderAssetReferencesFunc func,  AfterglowMaterial& material, const std::string& materialName);

	AfterglowMaterialManager& materialManager;
	AfterglowAssetMonitor& assetMonitor;

	inline static constexpr const char* materialNameTag = "materialName";
	inline static constexpr const char* materialInstanceNameTag = "materialInstanceName";
};


AfterglowMaterialAssetRegistrar::AfterglowMaterialAssetRegistrar(AfterglowMaterialManager& materialManager, AfterglowAssetMonitor& assetMonitor) :
	_impl(std::make_unique<Impl>(materialManager, assetMonitor)) {
}

AfterglowMaterialAssetRegistrar::~AfterglowMaterialAssetRegistrar() {

}

std::string AfterglowMaterialAssetRegistrar::registerMaterialAsset(const std::string& materialPath) {
	std::string materialName = _impl->materialManager.errorMaterialName();
	try {
		AfterglowMaterialAsset materialAsset(materialPath);
		materialName = materialAsset.materialName();
		_impl->assetMonitor.registerAsset(
			AfterglowAssetMonitor::AssetType::Material,
			materialPath,
			{ {Impl::materialNameTag, materialName } }
		);
		_impl->createMaterialFromAsset(materialAsset);
	}
	catch (const std::runtime_error& error) {
		// Handle initialize failed.
		_impl->assetMonitor.registerAsset(
			AfterglowAssetMonitor::AssetType::Material,
			materialPath,
			{ {Impl::materialNameTag, materialPath} }
		);
		DEBUG_CLASS_ERROR(std::format(
			"Material path: {}\n Some errors were occurred when creating material asset: {}",
			materialPath, error.what()
		));
	}
	return materialName;
}

std::string AfterglowMaterialAssetRegistrar::registerMaterialInstanceAsset(const std::string& materialInstancePath) {
	auto& materialManager = _impl->materialManager;
	std::string materialInstanceName = materialManager.errorMaterialInstanceName();
	std::string parentMaterialName = materialManager.errorMaterialName();
	try {
		AfterglowMaterialInstanceAsset materialInstanceAsset(materialInstancePath);
		materialInstanceName = materialInstanceAsset.materialnstanceName();
		parentMaterialName = materialInstanceAsset.parentMaterialName();
		_impl->assetMonitor.registerAsset(
			AfterglowAssetMonitor::AssetType::MaterialInstance,
			materialInstancePath,
			{ {"materialInstanceName", materialInstanceName}, {Impl::materialNameTag, parentMaterialName } }
		);
		auto* materialLayout = materialManager.materialLayout(parentMaterialName);
		if (!materialLayout) {
			DEBUG_CLASS_ERROR("Parent material of material instance asset is not exists.");
			return materialManager.errorMaterialInstanceName();
		}
		auto& materialInstance = materialManager.createMaterialInstance(
			materialInstanceName, parentMaterialName
		);
		materialInstanceAsset.fill(materialInstance);
	}
	catch (const std::exception& assetException) {
		DEBUG_CLASS_ERROR(std::string("Some errors occurred when creating material instance asset: \n") + assetException.what());
	}
	return materialInstanceName;
}

void AfterglowMaterialAssetRegistrar::unregisterMaterialAsset(const std::string& materialPath) {
	_impl->assetMonitor.unregisterAsset(materialPath);
	// Try to clear relative material layout from manager
	try {
		AfterglowMaterialAsset materialAsset(materialPath);
		std::string materialName = materialAsset.materialName();
		auto& material = materialAsset.material();
		// unregister relative shaders
		_impl->decreaseShaderAssetReference(material.fragmentShaderPath(), materialName);
		_impl->decreaseShaderAssetReference(material.vertexShaderPath(), materialName);
		if (material.hasComputeTask()) {
			_impl->decreaseShaderAssetReference(material.computeTask().computeShaderPath(), materialName);
			// Unregister initComputeShader assets.
			for (const auto& ssboInfo : material.computeTask().ssboInfos()) {
				if (ssboInfo.initMode != compute::SSBOInitMode::ComputeShader) {
					continue;
				}
				_impl->decreaseShaderAssetReference(ssboInfo.initResource, materialName);
			}
		}
		_impl->materialManager.removeMaterial(materialAsset.materialName());
		// Here relatived material instance assets are remained. Until unregisterMaterialInstance() is invoked manually.
	}
	catch (const std::exception& assetException) { 
		DEBUG_CLASS_ERROR(std::string("Some errors occurred when unregistering the material asset: \n") + assetException.what());
	}
}

void AfterglowMaterialAssetRegistrar::unregisterMaterialInstanceAsset(const std::string& materialInstancePath) {
	_impl->assetMonitor.unregisterAsset(materialInstancePath);
	try {
		AfterglowMaterialInstanceAsset materialInstanceAsset(materialInstancePath);
		_impl->materialManager.removeMaterialInstance(materialInstanceAsset.materialnstanceName());
	}
	catch (const std::exception& assetException) {
		DEBUG_CLASS_ERROR(std::string("Some errors occurred when unregistering the material instance asset: \n") + assetException.what());
	}
}

AfterglowMaterialAssetRegistrar::Impl::Impl(AfterglowMaterialManager& materialManagerRef, AfterglowAssetMonitor& assetMonitorRef) : 
	materialManager(materialManagerRef), assetMonitor(assetMonitorRef) {
	initAssetMonitorCallbacks();
}

inline void AfterglowMaterialAssetRegistrar::Impl::initAssetMonitorCallbacks() {
	// Catch error, print a debug info , don't let exception leave.
	assetMonitor.registerModifiedCallback(
		AfterglowAssetMonitor::AssetType::Material,
		[this](const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos) {
			modifiedMateiralAssetCallback(*this, modifiedPath, tagInfos);
		}
	);

	assetMonitor.registerModifiedCallback(
		AfterglowAssetMonitor::AssetType::MaterialInstance,
		[this](const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos) {
			modifiedMateiralInstanceAssetCallback(*this, modifiedPath, tagInfos);
		}
	);

	assetMonitor.registerModifiedCallback(
		AfterglowAssetMonitor::AssetType::Shader,
		[this](const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos) {
			modifiedShaderAssetCallback(*this, modifiedPath, tagInfos);
		}
	);
}

inline void AfterglowMaterialAssetRegistrar::Impl::createMaterialFromAsset(const AfterglowMaterialAsset& materialAsset) {
	std::string materialName = materialAsset.materialName();
	materialManager.createMaterial(materialName, materialAsset.material(), materialAsset);
	auto& materialLayout = *materialManager.materialLayout(materialName);
	auto& material = materialLayout.material();
	updateShaderAssetReferences(&Impl::increaseShaderAssetReference, material, materialName);
}

void AfterglowMaterialAssetRegistrar::Impl::modifiedMateiralAssetCallback(Impl& context, const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos) {
	try {
		AfterglowMaterialAsset materialAsset(modifiedPath);
		std::string newMaterialName = materialAsset.materialName();

		// Decreasing old shaders
		auto& oldMaterialName = tagInfos[materialNameTag];
		auto* oldMaterial = context.materialManager.material(oldMaterialName);
		if (oldMaterial) {
			context.updateShaderAssetReferences(&Impl::decreaseShaderAssetReference, *oldMaterial, oldMaterialName);
		}

		// If material name was changed, remove old material.
		if (newMaterialName != oldMaterialName) {
			context.materialManager.removeMaterial(oldMaterialName);
			oldMaterialName = newMaterialName;
		}
		context.createMaterialFromAsset(materialAsset);
	}
	catch (const std::exception& assetException) {
		DEBUG_TYPE_ERROR(Impl, std::format(
			"Some errors were occurred when creating material asset: {} \n", assetException.what()
		));
	}
}

void AfterglowMaterialAssetRegistrar::Impl::modifiedMateiralInstanceAssetCallback(Impl& context, const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos) {
	auto& materialManager = context.materialManager;
	try {
		AfterglowMaterialInstanceAsset materialInstanceAsset(modifiedPath);
		std::string newMaterialInstanceName = materialInstanceAsset.materialnstanceName();
		std::string newMaterialName = materialInstanceAsset.parentMaterialName();
		auto* matLayout = materialManager.materialLayout(newMaterialName);
		// It seems use different matlayout with body
		// If material instance name was changed, remove old material instance.
		if (newMaterialInstanceName != tagInfos["materialInstanceName"] || newMaterialName != tagInfos["materialName"]) {
			// if new parent material is valid, create new one.
			if (matLayout) {
				materialManager.removeMaterialInstance(tagInfos["materialInstanceName"]);
				materialManager.createMaterialInstance(newMaterialInstanceName, newMaterialName);
				// It's materialLayout have not changed, so this function will not be triggered automatically.
				// So we call it manually.
				materialManager.materialResource(newMaterialInstanceName)->reloadMaterialLayout(
					materialManager.descriptorPool()
				);
				tagInfos["materialInstanceName"] = newMaterialInstanceName;
				tagInfos[materialNameTag] = newMaterialName;
			}
			else {
				DEBUG_TYPE_ERROR(Impl, std::format(
					"Failed to recreate material instance from asset, due to it's parent material name is not found: {}\n", newMaterialName
				));
			}
		}

		auto* matResource = materialManager.materialResource(newMaterialInstanceName);
		if (!matResource) {
			return;
		}
		// fill() will not change old parameter settings. so reset it to make sure removed parametes can be applied.
		matResource->materialInstance().reset();
		materialInstanceAsset.fill(matResource->materialInstance());
		// reload layout due to shader may change and it never reload resources automatically (for performance).
		matResource->reloadMaterialLayout(materialManager.descriptorPool());
		materialManager.submitMaterialInstance(newMaterialInstanceName);
	}
	catch (const std::exception& assetException) {
		DEBUG_TYPE_ERROR(Impl, std::format(
			"Some errors occurred when creating material instance asset: {}\n", assetException.what())
		);
	}
}

void AfterglowMaterialAssetRegistrar::Impl::modifiedShaderAssetCallback(Impl& context, const std::string& modifiedPath, AfterglowAssetMonitor::TagInfos& tagInfos) {
	auto& materialManager = context.materialManager;
	// Copy the tagInfos due to shader tagInfos will be changed by decreaseShaderAssetReference().
	AfterglowAssetMonitor::TagInfos copiedTagInfos = tagInfos;
	for (const auto& [materialName, verificationInfo] : copiedTagInfos) {
		if (verificationInfo != Impl::materialNameTag) {
			continue;
		}
		auto* matLayout = materialManager.materialLayout(materialName);
		// If material not found, decrease the shader asset reference.
		if (!matLayout) {
			// context.decreaseShaderAssetReference(modifiedPath, materialName);
			continue;
		}
		auto& material = matLayout->material();
		// Here redundant decreaseShaderAssetReference seem are not needed any more.
		// If material shader changed, unregister shader asset.
		// If this shader is not a material shader, unregister it.
		//if (modifiedPath != material.vertexShaderPath()
		//	&& modifiedPath != material.fragmentShaderPath()) {
		//	if (!material.hasComputeTask()) {
		//		context.decreaseShaderAssetReference(modifiedPath, materialName);
		//		continue;
		//	}
		//	auto& computeTask = material.computeTask();
		//	if (modifiedPath != computeTask.computeShaderPath()
		//		&& !computeTask.isInitComputeShader(modifiedPath)) {
		//		context.decreaseShaderAssetReference(modifiedPath, materialName);
		//		continue;
		//	}
		//}
		auto materialAsset = AfterglowMaterialAsset(material);
		try {
			// Recompile both of stage shaders because of before that could be error shaders, which have different input-output variables.
			materialManager.applyShaders(*matLayout, materialAsset);
		}
		catch (std::runtime_error& error) {
			// If failed to compile shaders, use error material instead.
			materialManager.applyErrorShaders(*matLayout);
			DEBUG_TYPE_ERROR(Impl, std::format(
				"Failed to update material, some shader compilation errors were occurred: {}", error.what()
			));
		}
		// When shader changed, only pipeline need to rebuild, resources are same.
		// So do not call mark matLayout to dated, dated will also reload its matResources.
		// markDated(matLayout);
		matLayout->updatePipelines();
	}
}

inline void AfterglowMaterialAssetRegistrar::Impl::increaseShaderAssetReference(const std::string& assetPath, const std::string& materialName) {
	auto* assetInfo = assetMonitor.registeredAssetInfo(assetPath);
	if (!assetInfo) {
		assetInfo = assetMonitor.registerAsset(AfterglowAssetMonitor::AssetType::Shader, assetPath);
	}
	if (!assetInfo) {
		throw std::runtime_error("[AfterglowMaterialAssetRegistrar] Shader file is not exists.");
	}
	// Reverse tag and context, due to many material refs are required.
	assetInfo->tagInfos[materialName] = materialNameTag;
}

inline void AfterglowMaterialAssetRegistrar::Impl::decreaseShaderAssetReference(const std::string& assetPath, const std::string& materialName) {
	auto* assetInfo = assetMonitor.registeredAssetInfo(assetPath);
	if (!assetInfo) {
		DEBUG_CLASS_WARNING("The shader asset was not registered.");
		return;
	}
	auto iterator = assetInfo->tagInfos.find(materialName);
	if (iterator == assetInfo->tagInfos.end() || iterator->second != materialNameTag) {
		DEBUG_CLASS_WARNING("The shader asset reference tag is not exists.");
		return;
	}
	assetInfo->tagInfos.erase(iterator);
	if (assetInfo->tagInfos.empty()) {
		assetMonitor.unregisterAsset(assetPath);
	}
}

inline void AfterglowMaterialAssetRegistrar::Impl::updateShaderAssetReferences(UpdateShaderAssetReferencesFunc func, AfterglowMaterial& material, const std::string& materialName) {
	(this->*func)(material.vertexShaderPath(), materialName);
	(this->*func)(material.fragmentShaderPath(), materialName);
	if (material.hasComputeTask()) {
		(this->*func)(material.computeTask().computeShaderPath(), materialName);
		// Register initComputeShader assets.
		for (const auto& ssboInfo : material.computeTask().ssboInfos()) {
			if (ssboInfo.initMode != compute::SSBOInitMode::ComputeShader) {
				continue;
			}
			(this->*func)(ssboInfo.initResource, materialName);
		}
	}
}
