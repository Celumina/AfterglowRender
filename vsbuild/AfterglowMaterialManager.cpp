#include "AfterglowMaterialManager.h"
#include <utility>

#include <map>
#include <unordered_set>
#include <unordered_map>

#include "GlobalAssets.h"
#include "AfterglowMaterialUtilities.h"
#include "AfterglowSynchronizer.h"
#include "AfterglowMaterialAsset.h"
#include "AfterglowMaterialAssetRegistrar.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowDescriptorSetWriter.h"
#include "AfterglowDescriptorSetReferences.h"
#include "AfterglowComputeTask.h"
#include "AfterglowPassManager.h"


struct AfterglowMaterialManager::Impl {
	using MaterialLayouts = std::unordered_map<std::string, AfterglowMaterialLayout>;
	using MaterialResources = std::unordered_map<std::string, AfterglowMaterialResource>;
	using InFlightDescriptorSets = std::array<AfterglowDescriptorSets::AsElement, cfg::maxFrameInFlight>;

	struct PerObjectSetContext {
		const ubo::MeshUniform* meshUniform = nullptr;
		std::array<AfterglowUniformBuffer::AsElement, cfg::maxFrameInFlight> inFlightUniformBuffers;
		// This set allocate own mesh buffer only
		InFlightDescriptorSets inFlightSets;
		std::array<AfterglowDescriptorSetReferences, cfg::maxFrameInFlight> inFlightSetReferences;
		std::array<bool, cfg::maxFrameInFlight> inFlightMaterialChangedFlags;
		bool activated = false;
	};

	struct GlobalSetContext {
		ubo::GlobalUniform globalUniform{};
		std::array<AfterglowUniformBuffer::AsElement, cfg::maxFrameInFlight> inFlightUniformBuffers;
		// Global texture assets from GlobalAsset.h
		std::vector<AfterglowMaterialResource::TextureResource> globalTextureResources;
		InFlightDescriptorSets globalInFlightSets; // Uniform binding and global texture bindings.
		render::PassUnorderedMap<InFlightDescriptorSets> allPassInFlightSets; // Pass import and input attachment bindings.
	};

	enum class MaterialResourceUpdateFlag {
		None = 0, 
		Uniform = 1 << 0, 
		Texture = 1 << 1, 
		UniformTexture = Uniform | Texture, 

		EnumCount
	};

	// One material instance support multi objects.
	using PerObjectSetContextArray = std::vector<PerObjectSetContext>;
	using MaterialPerObjectSetContexts = std::unordered_map<AfterglowMaterialResource*, PerObjectSetContextArray>;

	using DatedMaterialLayouts = std::unordered_set<AfterglowMaterialLayout*>;
	using DatedMaterialResources = std::array<std::unordered_map<AfterglowMaterialResource*, MaterialResourceUpdateFlag>, cfg::maxFrameInFlight>;
	// TODO: Here refs from container are DANGEROUS due to PerObjectSetContextArray use std::vector.
	using DatedPerObjectSetContexts = std::unordered_map<AfterglowMaterialResource*, PerObjectSetContextArray*>;

	// ComputeTask external ssbo context
	struct ComputeExternalSSBOContext {
		AfterglowDescriptorSetLayout::AsElement setLayout;
		InFlightDescriptorSets inflightSets;
		// <ssboName, assosicatedMaterialResource>
		std::vector<AfterglowMaterialResource*> associatedMaterialResources;
		AfterglowComputeTask::SSBOInfoRefs associatedSSBOInfos;
	};
	using ComputeExternalSSBOContexts = std::unordered_map<AfterglowMaterialLayout*, ComputeExternalSSBOContext>;

	Impl(
		AfterglowMaterialManager& inManager, 
		AfterglowCommandPool& inCommandPool,
		AfterglowGraphicsQueue& inGraphicsQueue,
		AfterglowPassManager& inPassManager,
		AfterglowAssetMonitor& inAssetMonitor, 
		AfterglowSynchronizer& inSynchronizer
	);

	inline AfterglowMaterialInstance& createMaterialInstanceWithoutLock(const std::string& name, const std::string& parentMaterialName);
	inline bool removeMaterialInstanceWithoutLock(const std::string& name);
	// @brief: Actual remove material implementation.
	inline bool removeMaterialWithoutLock(const std::string& name);

	inline bool instantializeMaterial(const std::string& name);

	inline void initGlobalDescriptorSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages);
	inline void initPassDescriptorSet(AfterglowPassInterface& pass);
	// @warning: Invoke only before the draw begin due to it's not handling the frameIndex case.
	inline void initAllPassImageSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages);

	// Call it when that material submit.
	inline void reloadMaterialResources(AfterglowMaterialLayout& matLayout);

	inline void applyMaterialLayout(AfterglowMaterialLayout& matLayout);
	inline void applyMaterialResource(AfterglowMaterialResource& matResource, MaterialResourceUpdateFlag updateFlag, uint32_t frameIndex);
	inline void applyPerObjectGlobalSetContext(AfterglowMaterialResource& matResource, PerObjectSetContextArray& perObjectSetContexts, uint32_t frameIndex);
	inline void applyGlobalUniformSet(uint32_t frameIndex);

	inline void applyPassImageSets(AfterglowPassInterface& pass, img::ImageReferences& passImages, uint32_t frameIndex);
	inline void applySwapchainPassImageSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages, uint32_t frameIndex);

	inline void appendGlobalSetTextureResource(shader::GlobalSetBindingIndex textureBindingIndex);

	inline PerObjectSetContextArray* perObjectSetContexts(AfterglowMaterialResource* matResource);

	// @brief: If current layout is compute task, append it and its associated context as dated.
	inline void appendDatedComputeExternalSSBOContext(AfterglowMaterialLayout& matLayout);
	inline void applyComputeExternalSSBOContext(AfterglowMaterialLayout& matLayout);
	inline void applyComputeExternalSSBOSetReference(AfterglowMaterialLayout& matLayout, AfterglowDescriptorSetReferences& setRefs, uint32_t frameIndex);

	inline bool submitMaterialInstanceWithoutLock(const std::string& name, MaterialResourceUpdateFlag updateFlag);
	inline void markAsDated(AfterglowMaterialResource& matResource, MaterialResourceUpdateFlag flag = MaterialResourceUpdateFlag::UniformTexture);

	// Static Pool Size
	// TODO: add a new pool for dynamic pool size.
	// Place front to make sure descriptor set destruct later than descriptor sets.
	AfterglowDescriptorPool::AsElement descriptorPool;
	AfterglowSharedTexturePool texturePool;
	AfterglowPassManager& passManager;
	AfterglowSynchronizer& synchronizer;

	AfterglowMaterialAssetRegistrar assetRegistrar;

	MaterialLayouts materialLayouts;
	MaterialResources materialResources;

	// Global Set Layouts
	AfterglowDescriptorSetLayout::AsElement globalDescriptorSetLayout;
	render::PassUnorderedMap<AfterglowDescriptorSetLayout::AsElement> allPassDescriptorSetLayouts;
	std::array<bool, cfg::maxFrameInFlight> inFlightSwapchainImageSetOutdatedFlags{};
	// Global Uniform Resources
	GlobalSetContext globalSetContext;

	// PerObject Set Layout
	AfterglowDescriptorSetLayout::AsElement perObjectDescriptorSetLayout;
	// Mesh Uniform Resources
	MaterialPerObjectSetContexts materialPerObjectSetContexts;

	AfterglowDescriptorSetWriter descriptorSetWriter;

	DatedMaterialLayouts datedMaterialLayouts;
	DatedMaterialResources datedMaterialResources;
	DatedPerObjectSetContexts datedPerObjectSetContexts;
	std::vector<PerObjectSetContextArray*> perObjectSetContextRemovingCache;
	std::vector<std::string> materialRemovingCache;
	std::vector<std::string> materialInstanceRemovingCache;

	ComputeExternalSSBOContexts computeExternalSSBOContexts;
	DatedMaterialLayouts datedComputeExternalSSBOContextKeys;

	AfterglowMaterialManager& manager;
};

AfterglowMaterialManager::Impl::Impl(
	AfterglowMaterialManager& inManager,
	AfterglowCommandPool& inCommandPool, 
	AfterglowGraphicsQueue& inGraphicsQueue, 
	AfterglowPassManager& inPassManager, 
	AfterglowAssetMonitor& inAssetMonitor, 
	AfterglowSynchronizer& inSynchronizer) :
	texturePool(inCommandPool, inGraphicsQueue, inSynchronizer),
	passManager(inPassManager),
	assetRegistrar(inManager, inAssetMonitor),
	synchronizer(inSynchronizer), 
	perObjectDescriptorSetLayout(AfterglowDescriptorSetLayout::makeElement(inCommandPool.device())),
	descriptorPool(AfterglowDescriptorPool::makeElement(inCommandPool.device())),
	descriptorSetWriter(inCommandPool.device()), 
	manager(inManager) {
	// TODO: Check remaining set size every update, if have not enough size, reset pool and dated all material resources(remember reload layout ).
	(*descriptorPool).extendUniformPoolSize(cfg::uniformDescriptorSize);
	(*descriptorPool).extendImageSamplerPoolSize(cfg::samplerDescriptorSize);
	(*descriptorPool).setMaxDescritporSets(cfg::descriptorSetSize);

	// @note: Apply manually due to late initialize issue.
	//initGlobalDescriptorSets();

	// Initialize PerOjbect set binding[0] : mesh uniform
	// All stage support, hardcoded yet.
	(*perObjectDescriptorSetLayout).appendBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
	);
}

inline AfterglowMaterialInstance& AfterglowMaterialManager::Impl::createMaterialInstanceWithoutLock(const std::string& name, const std::string& parentMaterialName) {
	AfterglowMaterialLayout* matLayout = nullptr;
	AfterglowMaterialResource* matResource = nullptr;
	if (materialLayouts.find(parentMaterialName) == materialLayouts.end()) {
		matLayout = &materialLayouts.emplace(
			parentMaterialName,
			AfterglowMaterialLayout{}
		).first->second;
		datedMaterialLayouts.insert(matLayout);
	}
	else {
		matLayout = &materialLayouts.at(parentMaterialName);
	}
	if (materialResources.find(name) == materialResources.end()) {
		matResource = &materialResources.emplace(
			name,
			AfterglowMaterialResource{ *matLayout, descriptorSetWriter, descriptorPool, texturePool }
 		).first->second;
	}
	else {
		matResource = &materialResources.at(name);
	}
	markAsDated(*matResource);
	return matResource->materialInstance();
}

inline bool AfterglowMaterialManager::Impl::removeMaterialInstanceWithoutLock(const std::string& name) {
	auto matResourceIterator = materialResources.find(name);
	// TODO: Encapsurate these create and remove param into a RAII class.
	if (matResourceIterator != materialResources.end()) {
		auto& matResource = matResourceIterator->second;
		for (uint32_t frameIndex = 0; frameIndex < cfg::maxFrameInFlight; ++frameIndex) {
			datedMaterialResources[frameIndex].erase(&matResource);
		}
		datedPerObjectSetContexts.erase(&matResource);
		materialPerObjectSetContexts.erase(&matResource);
		materialResources.erase(matResourceIterator);
		return true;
	}
	return false;
}

inline bool AfterglowMaterialManager::Impl::instantializeMaterial(const std::string& name) {
	auto matLayoutIterator = materialLayouts.find(name);
	if (matLayoutIterator == materialLayouts.end()) {
		DEBUG_CLASS_ERROR(std::format("Material not found: \"{}\"", name));
		return false;
	}
	// If material exists but have not material instance, try to create one which has same name with material. 
	DEBUG_CLASS_INFO("Material instance not found, the same name instance will be created from material: " + name);
	createMaterialInstanceWithoutLock(name, name);
	return true;
}

inline void AfterglowMaterialManager::Impl::initGlobalDescriptorSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages) {
	/** Pass SetLayout bindings:
	*	0            : GlobalUniform
	*	A part       : GlobalTextures / GlobalSamplers
	*/

	globalDescriptorSetLayout.recreate(texturePool.commandPool().device());
	// SetLayout bindings: Global Uniform buffer object and Global Textures.
	// Additional bindingInfo from enum
	std::vector<shader::GlobalSetBindingIndex> textureBindingIndices;
	Inreflect<shader::GlobalSetBindingIndex>::forEachAttribute([this, &textureBindingIndices](auto enumInfo) {
		if (enumInfo.name.ends_with(inreflect::EnumName(shader::GlobalSetBindingResource::Uniform))) {
			// Global set binding[0] : global uniform
			(*globalDescriptorSetLayout).appendBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
			);
		}
		else if (enumInfo.name.ends_with(inreflect::EnumName(shader::GlobalSetBindingResource::Texture))) {
			(*globalDescriptorSetLayout).appendBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
			);
			textureBindingIndices.push_back(enumInfo.raw);
		}
		else if (enumInfo.name.ends_with(inreflect::EnumName(shader::GlobalSetBindingResource::Sampler))) {
			(*globalDescriptorSetLayout).appendBinding(
				VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
			);
		}
	});
	// Initialize global inflight sets.
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		globalSetContext.globalInFlightSets[index].recreate(descriptorPool, globalDescriptorSetLayout, 1);
	}

	// Create global texture resource and initialize 
	for (auto textureBindingIndex : textureBindingIndices) {
		appendGlobalSetTextureResource(textureBindingIndex);
	}

	// Passes SetLayout binding and create resource
	passManager.forEachPass([this](AfterglowPassInterface& pass) {
		initPassDescriptorSet(pass);
	});
	// Write passes image Sets.
	initAllPassImageSets(allPassImages);
}

inline void AfterglowMaterialManager::Impl::initPassDescriptorSet(AfterglowPassInterface& pass) {
	// Init descriptor set layouts.
	auto& passSetLayout = allPassDescriptorSetLayouts[&pass];
	passSetLayout.recreate(texturePool.commandPool().device());

	/** Pass Set Layout
	*	A part       : Pass ImportAttachmentImages
	*	B part       : Pass InputAttachmentImages
	*/

	// Pass Import attachment: 
	for (const auto& importAttachment : pass.importAttachments()) {
		(*passSetLayout).appendBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}

	// Pass input attachment: 
	auto& subpassContext = pass.subpassContext();
	for (const auto& attachmentInfo : subpassContext.inputAttachmentInfos()) {
		(*passSetLayout).appendBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}

	// Create pass sets.
	auto& passInFlightSets = globalSetContext.allPassInFlightSets[&pass];
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		passInFlightSets[index].recreate(descriptorPool, passSetLayout, 1);
	}
}

inline void AfterglowMaterialManager::Impl::initAllPassImageSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages) {
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		passManager.forEachPass([this, &allPassImages, &index](AfterglowPassInterface& pass){
			applyPassImageSets(pass, allPassImages.at(&pass), index);
		});
	}
}

inline void AfterglowMaterialManager::Impl::reloadMaterialResources(AfterglowMaterialLayout& matLayout) {
	// Submit all matterial instances of this material.
	// TODO: optimize here by a graph: matLayout->{resources}
	for (auto& [matResourceName, matResource] : materialResources) {
		auto* a = &matResource.materialLayout();
		if (&matResource.materialLayout() == &matLayout) {
			matResource.reloadMaterialLayout();
			markAsDated(matResource);
		}
	}
}

inline void AfterglowMaterialManager::Impl::applyMaterialLayout(AfterglowMaterialLayout& matLayout) {
	matLayout.updateDescriptorSetLayouts(
		passManager, 
		allPassDescriptorSetLayouts, 
		globalDescriptorSetLayout, 
		perObjectDescriptorSetLayout
	);
	// Reload all derived material instances. 
	reloadMaterialResources(matLayout);
	// Delay the shader update if any external ssbo exists.
	if (datedComputeExternalSSBOContextKeys.contains(&matLayout)) {
		return;
	}
	try {
		matLayout.updatePipelines();
	}
	catch (const std::runtime_error& error) {
		manager.applyErrorShaders(matLayout);
		DEBUG_CLASS_ERROR(std::format("Failed to apply material, probably some problems occur in shaders: {}", error.what()));
		// After error shader compilation, Retry to update material layout.
		matLayout.updatePipelines();
	}
}

inline void AfterglowMaterialManager::Impl::applyMaterialResource(AfterglowMaterialResource& matResource, MaterialResourceUpdateFlag updateFlag, uint32_t frameIndex) {
	try {
		uint32_t flagValue = util::EnumValue(updateFlag);
		if (flagValue & util::EnumValue(MaterialResourceUpdateFlag::Uniform)) {
			matResource.updateUniforms(frameIndex);
		}
		if (flagValue & util::EnumValue(MaterialResourceUpdateFlag::Texture)) {
			matResource.updateTextures(frameIndex);
		}
		matResource.submitDescriptorSets(frameIndex);
	}
	catch (std::runtime_error& error) {
		DEBUG_CLASS_ERROR(std::format("Failed to apply material resource, due to: {}", error.what()));
	}
	// Update perObject set material changed flag.
	auto objectSetIterator = materialPerObjectSetContexts.find(&matResource);
	if (objectSetIterator != materialPerObjectSetContexts.end()) {
		for (auto& setContext : objectSetIterator->second) {
			setContext.inFlightMaterialChangedFlags[frameIndex] = true;
		}
	}
}

inline void AfterglowMaterialManager::Impl::applyPerObjectGlobalSetContext(AfterglowMaterialResource& matResource, PerObjectSetContextArray& perObjectSetContexts, uint32_t frameIndex) {
	/*uint32_t frameIndex = manager.device().currentFrameIndex();*/
	auto& matResourceSets = matResource.inFlightDescriptorSets()[frameIndex];
	auto& matLayout = matResource.materialLayout();

	bool inactivatedExists = false;
	// Many objects using same material instance.
	for (auto& perObjectSetContext : perObjectSetContexts) {
		// Initialize set.
		auto& sets = perObjectSetContext.inFlightSets[frameIndex];
		auto& buffer = perObjectSetContext.inFlightUniformBuffers[frameIndex];
		auto& setReferences = perObjectSetContext.inFlightSetReferences[frameIndex];
		bool& materialChanged = perObjectSetContext.inFlightMaterialChangedFlags[frameIndex];

		if (!sets) {
			sets.recreate(descriptorPool, perObjectDescriptorSetLayout, 1);
		}

		// Initialize / update buffer memory.
		if (!buffer || (*buffer).sourceData() != perObjectSetContext.meshUniform) {
			buffer.recreate(manager.device(), perObjectSetContext.meshUniform, sizeof(ubo::MeshUniform));
		}
		else {
			(*buffer).updateMemory();
		}

		// Update set references
		if (!setReferences.source() || materialChanged) {
			setReferences.reset(matResourceSets);

			// @note: Fill pass set only if material was changed for performance.
			// Here we use sets[0] due to we treat external set always one set contains many binding elements
			// Global set ref
			setReferences[util::EnumValue(shader::SetIndex::Global)] = globalSetContext.globalInFlightSets[frameIndex][0];
			// Pass set ref
			setReferences[util::EnumValue(shader::SetIndex::Pass)] = globalSetContext.allPassInFlightSets[&matLayout.pass()][frameIndex][0];
			// Per object set ref
			setReferences[util::EnumValue(shader::SetIndex::PerObject)] = sets[0];

			materialChanged = false;
		}

		// Register mesh uniform buffer.
		descriptorSetWriter.registerBuffer(
			*buffer,
			perObjectDescriptorSetLayout,
			sets[0],
			util::EnumValue(shader::PerObjectSetBindingIndex::MeshUniform)
		);

		inactivatedExists |= !perObjectSetContext.activated;
		perObjectSetContext.activated = false;
	}

	// Cache remove instead of immediatly due to resource could be using here.
	if (inactivatedExists) {
		perObjectSetContextRemovingCache.push_back(&perObjectSetContexts);
	}
}

inline void AfterglowMaterialManager::Impl::applyGlobalUniformSet(uint32_t frameIndex) {
	// Update global uniform buffer memory.
	auto& globalUniform = globalSetContext.globalUniform;
	auto& buffer = globalSetContext.inFlightUniformBuffers[frameIndex];
	if (!buffer || (*buffer).sourceData() != &globalUniform) {
		buffer.recreate(manager.device(), &globalUniform, sizeof(ubo::GlobalUniform));
	}
	else {
		(*buffer).updateMemory();
	}

	// Global uniform buffer
	descriptorSetWriter.registerBuffer(
		*globalSetContext.inFlightUniformBuffers[frameIndex],
		globalDescriptorSetLayout,
		// GlobalSet, never apply Gloabal data in a per object set
		globalSetContext.globalInFlightSets[frameIndex][0],
		util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)
	);
}

inline void AfterglowMaterialManager::Impl::applyPassImageSets(
	AfterglowPassInterface& pass, 
	img::ImageReferences& passImages, 
	uint32_t frameIndex
) {
	// Seperate for each pass inputAttachments and importAttachments
	/**
	* 	RenderPass Descriptor sets
	*		ImportAttachmentImageRefs...
	*		InputAttachmentImageRefs...
	*/

	// We use seperate setlayout for pass, binding begin from 0 (without uniform buffer etc.)
	uint32_t attachmentBindingIndex = 0;
	auto& subpassContext = pass.subpassContext();
	auto& passInFlightSet = globalSetContext.allPassInFlightSets[&pass];

	// Import Attachment Images
	for (const auto& importAttachment : pass.importAttachments()) {
		auto& imageRef = passImages[importAttachment.destAttachmentIndex];
		VkImageLayout imageLayout = subpassContext.isDepthAttachmentIndex(importAttachment.destAttachmentIndex)
			? AfterglowSubpassContext::depthAttachmentRWLayout() : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorSetWriter.registerImage(
			imageRef.sampler,
			imageRef.imageView,
			allPassDescriptorSetLayouts[&pass],
			passInFlightSet[frameIndex][0],
			attachmentBindingIndex,
			imageLayout
		);
		++attachmentBindingIndex;
	}

	// Input Attachment Images
	auto& inputAttachmentInfos = subpassContext.inputAttachmentInfos();
	for (const auto& info : inputAttachmentInfos) {
		auto& imageRef = passImages[info.attachmentIndex];
		VkImageLayout imageLayout = subpassContext.isDepthAttachmentIndex(info.attachmentIndex)
			? AfterglowSubpassContext::depthAttachmentRWLayout() : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorSetWriter.registerImage(
			imageRef.sampler,
			imageRef.imageView,
			allPassDescriptorSetLayouts[&pass],
			passInFlightSet[frameIndex][0],
			attachmentBindingIndex,
			imageLayout
		);
		++attachmentBindingIndex;
	}
}

inline void AfterglowMaterialManager::Impl::applySwapchainPassImageSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages, uint32_t frameIndex) {
	passManager.forEachPass([](AfterglowPassInterface& pass, auto& impl, auto& inAllPassImages, auto inFrameIndex){
		if (pass.extentMode() == AfterglowPassInterface::ExtentMode::Swapchain) {
			auto& passImages = inAllPassImages.at(&pass);
			impl.applyPassImageSets(pass, passImages, inFrameIndex);
		}
	}, *this, allPassImages, frameIndex);
}

void AfterglowMaterialManager::applyErrorShaders(AfterglowMaterialLayout& matLayout) {
	applyShaders(matLayout, mat::ErrorMaterialAsset(), false);
}

void AfterglowMaterialManager::safeApplyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset) noexcept {
	try {
		// Recompile both of stage shaders because of before that could be error shaders, which have different input-output variables.
		applyShaders(matLayout, matAsset);
	}
	catch (std::exception& error) {
		// If failed to compile shaders, use error material instead.
		applyErrorShaders(matLayout);
		DEBUG_TYPE_ERROR(Impl, std::format(
			"Failed to apply shaders, some compilation errors were occurred: {}", error.what()
		));
	}
}

AfterglowSharedTexturePool& AfterglowMaterialManager::texturePool() noexcept {
	return _impl->texturePool;
}

void AfterglowMaterialManager::waitGPU() const {
	_impl->synchronizer.wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	_impl->synchronizer.wait(AfterglowSynchronizer::FenceFlag::RenderInFlight);
}

inline void AfterglowMaterialManager::Impl::appendGlobalSetTextureResource(shader::GlobalSetBindingIndex textureBindingIndex) {
	auto assetInfo = shader::GlobalSetBindingTextureInfo(textureBindingIndex);

	// Create global texture
	auto& resource = globalSetContext.globalTextureResources.emplace_back(
		// std::string(Inreflect<shader::GlobalSetBindingIndex>::enumName(textureBindingIndex)),
		util::EnumValue(textureBindingIndex),
		std::make_unique<AfterglowTextureReference>(texturePool.texture({ assetInfo }))
	);

	// Write descriptor set
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		descriptorSetWriter.registerImage(
			resource.textureRef->texture(),
			globalDescriptorSetLayout,
			globalSetContext.globalInFlightSets[index][0], // Set 0
			util::EnumValue(textureBindingIndex)           // Binding n
		);
	}
}

inline AfterglowMaterialManager::Impl::PerObjectSetContextArray* AfterglowMaterialManager::Impl::perObjectSetContexts(AfterglowMaterialResource* matResource) {
	auto perObjectSetIterator = materialPerObjectSetContexts.find(matResource);
	if (perObjectSetIterator == materialPerObjectSetContexts.end()) {
		DEBUG_CLASS_WARNING("Have not SetContext was created from this material resource.");
		return nullptr;
	}
	return &perObjectSetIterator->second;
}

inline void AfterglowMaterialManager::Impl::appendDatedComputeExternalSSBOContext(AfterglowMaterialLayout& matLayout) {
	const auto& material = matLayout.material();
	if (!material.hasComputeTask()) {
		return;
	}

	// Mark as dated layouts which are associated with the current layout.
	for (auto& [otherMatLayout, otherContext] : computeExternalSSBOContexts) {
		if (otherMatLayout == &matLayout) {
			continue;
		}
		for (const auto& otherAssociatedMatResource : otherContext.associatedMaterialResources) {
			if (&otherAssociatedMatResource->materialLayout() == &matLayout) {
				// Only shader should be update.
				// datedMaterialLayouts.insert(otherMatLayout);
				datedComputeExternalSSBOContextKeys.insert(otherMatLayout);
			}
		}
	}

	// Curent layout context.
	datedComputeExternalSSBOContextKeys.insert(&matLayout);
}

inline void AfterglowMaterialManager::Impl::applyComputeExternalSSBOContext(AfterglowMaterialLayout& matLayout) {
	// Update self set layout.
	auto& context = computeExternalSSBOContexts[&matLayout];
	context.associatedMaterialResources.clear();
	context.associatedSSBOInfos.clear();

	const auto& material = matLayout.material();
	const auto& externalSSBOs = material.computeTask().externalSSBOs();

	context.setLayout.recreate(manager.device());
	for (const auto& [externalMaterialName, externalSSBOName] : externalSSBOs) {
		auto externalMatResourceIterator = materialResources.find(externalMaterialName);
		if (externalMatResourceIterator == materialResources.end()) {
			continue;
		}
		auto& externalMatResource = externalMatResourceIterator->second;
		const auto& externalMaterial = externalMatResource.materialLayout().material();
		if (!externalMaterial.hasComputeTask()) {
			DEBUG_CLASS_WARNING(std::format("External material does not have a compute task: {}", externalMaterialName));
			continue;	
		}
		const auto& externalComputeTask = externalMaterial.computeTask();
		const auto* externalSSBOInfo = externalComputeTask.findSSBOInfo(externalSSBOName);
		if (!externalSSBOInfo) {
			DEBUG_CLASS_WARNING(std::format("External ssbo is not exists: {}", externalSSBOName));
			continue;
		}
		// Just one read only binding for external ssbo? But Read only index will change, only if update its every frame.
		auto shaderStage = externalSSBOInfo->stage();
		auto shaderStageFlags = AfterglowMaterialLayout::vulkanShaderStage(shaderStage);
		if (externalSSBOInfo->isBuffer()) {
			(*context.setLayout).appendBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStageFlags);
		}
		else {
			(*context.setLayout).appendBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStageFlags);
		}
		context.associatedMaterialResources.push_back(&externalMatResource);
		context.associatedSSBOInfos.push_back(externalSSBOInfo);
	}

	for (uint32_t frameIndex = 0; frameIndex < cfg::maxFrameInFlight; ++frameIndex) {
		context.inflightSets[frameIndex].recreate(descriptorPool, context.setLayout, 1);
		for (uint32_t bindingIndex = 0; bindingIndex < context.associatedMaterialResources.size(); ++bindingIndex) {
			auto& associatedMaterialResource = context.associatedMaterialResources[bindingIndex];
			auto& associatedSSBOName = context.associatedSSBOInfos[bindingIndex]->name();
			// Use first physical ssboIndex for readonly resource.
			 //auto* resource = associatedMaterialResource->findStorageBufferResource(associatedSSBOName, frameIndex, 0);
			// Seems use the current ssbo is better.
			auto* resource = associatedMaterialResource->findStorageBufferResource(associatedSSBOName, frameIndex, cfg::maxFrameInFlight - 1);

			if (!resource) {
				continue;
			}
			if (resource->buffer) {
				descriptorSetWriter.registerBuffer(
					*resource->buffer, context.setLayout, context.inflightSets[frameIndex][0], bindingIndex
				);
			}
			else if (resource->image) {
				descriptorSetWriter.registerImage(
					*resource->image, context.setLayout, context.inflightSets[frameIndex][0], bindingIndex, VK_IMAGE_LAYOUT_GENERAL
				);
			}
		}
	}

	AfterglowMaterialAsset matAsset(matLayout.material());
	matLayout.activateComputeExternalSSBOSetLayout(context.setLayout);
	// TODO: Initializer compute shader use external ssbo support?
	manager.safeApplyShaders(matLayout, matAsset);
	matLayout.updatePipelines();
}

inline void AfterglowMaterialManager::Impl::applyComputeExternalSSBOSetReference(AfterglowMaterialLayout& matLayout, AfterglowDescriptorSetReferences& setRefs, uint32_t frameIndex) {
	if (!matLayout.material().hasComputeTask() || matLayout.material().computeTask().externalSSBOs().empty()) {
		return;
	}
	// Shared storage set ref.
	auto externalSSBOContextIterator = computeExternalSSBOContexts.find(&matLayout);
	if (externalSSBOContextIterator != computeExternalSSBOContexts.end() && !externalSSBOContextIterator->second.associatedMaterialResources.empty()) {
		// Here need to append manually due to dscriptorSets of material layout is not updated yet.
		constexpr auto externalStorageSetIndex = util::EnumValue(shader::SetIndex::ExternalStorage);
		if (setRefs.size() < externalStorageSetIndex + 1) {
			setRefs.resize(externalStorageSetIndex + 1);
		}
		setRefs[externalStorageSetIndex] = externalSSBOContextIterator->second.inflightSets[frameIndex][0];
	}
}

inline bool AfterglowMaterialManager::Impl::submitMaterialInstanceWithoutLock(const std::string& name, MaterialResourceUpdateFlag updateFlag) {
	auto iterator = materialResources.find(name);
	if (iterator == materialResources.end()) {
		return false;
	}
	markAsDated(iterator->second, updateFlag);
	return true;
}

inline void AfterglowMaterialManager::Impl::markAsDated(AfterglowMaterialResource& matResource, MaterialResourceUpdateFlag flag) {
	for (uint32_t frameIndex = 0; frameIndex < cfg::maxFrameInFlight; ++frameIndex) {
		auto [iterator, inserted] = datedMaterialResources[frameIndex].try_emplace(&matResource);
		if (inserted) { // New one inserted case
			iterator->second = flag;
		}
		else { // Element existed case
			reinterpret_cast<std::underlying_type_t<MaterialResourceUpdateFlag>&>(iterator->second) |= util::EnumValue(flag);
		}
		
	}
}

inline bool AfterglowMaterialManager::Impl::removeMaterialWithoutLock(const std::string& name) {
	auto& matResources = materialResources;
	auto layoutIterator = materialLayouts.find(name);
	if (layoutIterator == materialLayouts.end()) {
		DEBUG_CLASS_WARNING(std::format("Failed to remove material due to material is not exist: {}", name));
		return false;
	}
	auto& matLayout = layoutIterator->second;

	// Handling material layouts with is ssbo associated with this.
	for (auto& [otherMatLayout, externalSSBOContext] : computeExternalSSBOContexts) {
		for (auto* associatedMaterialResource : externalSSBOContext.associatedMaterialResources) {
			if (&associatedMaterialResource->materialLayout() == &matLayout) {
				datedMaterialLayouts.insert(otherMatLayout);
				break;
			}
		}
	}

	// Remove relative material resources.
	std::vector<std::string> invalidMaterialInstanceNames;
	for (auto& [materialInstanceName, matResource] : matResources) {
		if (&matResource.materialLayout() == &matLayout) {
			// _impl->datedMaterialResources.erase(&matResource);
			invalidMaterialInstanceNames.push_back(materialInstanceName);
		}
	}
	for (const auto& invalidMaterialInstanceName : invalidMaterialInstanceNames) {
		// matResources.erase(invalidMatResourceName);
		removeMaterialInstanceWithoutLock(invalidMaterialInstanceName);
	}

	// Remove material layout
	datedMaterialLayouts.erase(&matLayout);
	materialLayouts.erase(layoutIterator);
	// Remove compute external ssbo contexts.
	datedComputeExternalSSBOContextKeys.erase(&matLayout);
	computeExternalSSBOContexts.erase(&matLayout);
}

AfterglowMaterialManager::AfterglowMaterialManager(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowPassManager& passManager,
	AfterglowAssetMonitor& assetMonitor, 
	AfterglowSynchronizer& synchronizer) :
	_impl(std::make_unique<Impl>(*this, commandPool, graphicsQueue, passManager, assetMonitor, synchronizer)) {
	
	// Initialize ErrorMaterial (For base object)
	createMaterial(mat::ErrorMaterialName(), AfterglowMaterial::errorMaterial());
	createMaterialInstance(mat::ErrorMaterialName(), mat::ErrorMaterialName());

	// Initialize EmptyPostProcessMaterial (For disabled post process)
	createMaterial(mat::EmptyPostProcessMaterialName(), AfterglowMaterial::emptyPostProcessMaterial());
	createMaterialInstance(mat::EmptyPostProcessMaterialName(), mat::EmptyPostProcessMaterialName());
}

AfterglowMaterialManager::~AfterglowMaterialManager() {
}

void AfterglowMaterialManager::initGlobalDescriptorSets(render::PassUnorderedMap<img::ImageReferences>& allPassImages) {
	_impl->initGlobalDescriptorSets(allPassImages);
}

AfterglowDevice& AfterglowMaterialManager::device() noexcept {
	return _impl->texturePool.commandPool().device();
}

AfterglowPassManager& AfterglowMaterialManager::passManager() noexcept {
	return _impl->passManager;
}

AfterglowDescriptorPool& AfterglowMaterialManager::descriptorPool() {
	return _impl->descriptorPool;
}

AfterglowDescriptorSetWriter& AfterglowMaterialManager::descriptorSetWriter() {
	return _impl->descriptorSetWriter;
}

std::string AfterglowMaterialManager::registerMaterialAsset(const std::string& materialPath) {
	return _impl->assetRegistrar.registerMaterialAsset(materialPath);
}

std::string AfterglowMaterialManager::registerMaterialInstanceAsset(const std::string& materialInstancePath) {
	return _impl->assetRegistrar.registerMaterialInstanceAsset(materialInstancePath);
}

void AfterglowMaterialManager::unregisterMaterialAsset(const std::string& materialPath) {
	_impl->assetRegistrar.unregisterMaterialAsset(materialPath);
}

void AfterglowMaterialManager::unregisterMaterialInstanceAsset(const std::string& materialInstancePath) {
	_impl->assetRegistrar.unregisterMaterialInstanceAsset(materialInstancePath);
}

AfterglowMaterial& AfterglowMaterialManager::createMaterial(
	const std::string& name, 
	util::OptionalRef<AfterglowMaterial> sourceMaterial,
	util::OptionalRef<AfterglowMaterialAsset> materialAsset) {
	LockGuard lockGuard{_mutex};
	const AfterglowMaterial* safeSrcMaterial = nullptr;
	if (sourceMaterial == std::nullopt) {
		safeSrcMaterial = &AfterglowMaterial::emptyMaterial();
	}
	else {
		safeSrcMaterial = &(*sourceMaterial).get();
	}
	auto& matLayouts = _impl->materialLayouts;
	AfterglowMaterialLayout* matLayout = nullptr;
	if (matLayouts.find(name) == matLayouts.end()) { /* Create new material. */
		matLayout = &matLayouts.emplace(
			name, AfterglowMaterialLayout{ *safeSrcMaterial}
		).first->second;
	}
	else { /* Update exists material. */
		matLayout = &matLayouts.at(name);
		matLayout->setMaterial(*safeSrcMaterial);
		if (materialAsset != std::nullopt) { /* Update shaders if the material asset is provided. */ 
			safeApplyShaders(*matLayout, *materialAsset);
		}
	}
	_impl->datedMaterialLayouts.insert(matLayout);
	return matLayout->material();
}

AfterglowMaterialInstance& AfterglowMaterialManager::createMaterialInstance(const std::string& name, const std::string& parentMaterialName) {
	LockGuard lockGuard{ _mutex };
	return _impl->createMaterialInstanceWithoutLock(name, parentMaterialName);
}

void AfterglowMaterialManager::removeMaterial(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	_impl->materialRemovingCache.push_back(name);
}

void AfterglowMaterialManager::removeMaterialInstance(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	_impl->materialInstanceRemovingCache.push_back(name);
}

AfterglowMaterial* AfterglowMaterialManager::material(const std::string& name) {
	auto& matLayouts = _impl->materialLayouts;
	auto iterator = matLayouts.find(name);
	if (iterator != matLayouts.end()) {
		return &iterator->second.material();
	}
	return nullptr;
}

AfterglowMaterial* AfterglowMaterialManager::findMaterialByInstanceName(const std::string& name) {
	auto& matResources = _impl->materialResources;
	auto iterator = matResources.find(name);
	if (iterator != matResources.end()) {
		return &iterator->second.materialLayout().material();
	}
	return nullptr;
}

AfterglowMaterialLayout* AfterglowMaterialManager::materialLayout(const std::string& name) {
	auto& matLayouts = _impl->materialLayouts;
	auto iterator = matLayouts.find(name);
	if (iterator != matLayouts.end()) {
		return &iterator->second;
	}
	DEBUG_CLASS_WARNING(std::format("Material layout is not exists: \"{}\"", name));
	return nullptr;
}

const AfterglowMaterialLayout* AfterglowMaterialManager::materialLayout(const std::string& name) const {
	return const_cast<AfterglowMaterialManager*>(this)->materialLayout(name);
}

AfterglowMaterialInstance* AfterglowMaterialManager::materialInstance(const std::string& name) {
	auto& matResources = _impl->materialResources;
	auto iterator = matResources.find(name);
	if (iterator != matResources.end()) {
		return &iterator->second.materialInstance();
	}
	return nullptr;
}

AfterglowMaterialResource* AfterglowMaterialManager::materialResource(const std::string& name) {
	auto matResourceIterator = _impl->materialResources.find(name);
	if (matResourceIterator != _impl->materialResources.end()) {
		return &matResourceIterator->second;
	}
	DEBUG_CLASS_WARNING(std::format("Material instance is not exists: \"{}\"", name));
	return nullptr;
}

const AfterglowMaterialResource* AfterglowMaterialManager::materialResource(const std::string& name) const {
	return const_cast<AfterglowMaterialManager*>(this)->materialResource(name);
}

bool AfterglowMaterialManager::submitMaterial(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	auto iterator = _impl->materialLayouts.find(name);
	if (iterator == _impl->materialLayouts.end()) {
		return false;
	}
	_impl->datedMaterialLayouts.insert(&iterator->second);
	return true;
}

bool AfterglowMaterialManager::submitMaterialInstance(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	return _impl->submitMaterialInstanceWithoutLock(name, Impl::MaterialResourceUpdateFlag::UniformTexture);
}

bool AfterglowMaterialManager::submitMaterialInstanceUniformParams(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	return _impl->submitMaterialInstanceWithoutLock(name, Impl::MaterialResourceUpdateFlag::Uniform);
}

bool AfterglowMaterialManager::submitMaterialInstanceTextureParams(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	return _impl->submitMaterialInstanceWithoutLock(name, Impl::MaterialResourceUpdateFlag::Texture);
}

bool AfterglowMaterialManager::submitMeshUniform(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform) {
	auto& matResources = _impl->materialResources;
	auto matResourceIterator = matResources.find(materialInstanceName);
	if (matResourceIterator == matResources.end()) {
		if (!_impl->instantializeMaterial(materialInstanceName)) {
			return false;
		}
		matResourceIterator = matResources.find(materialInstanceName);
	}
	auto& materialResource = matResourceIterator->second;
	auto& perObjectSetContexts = _impl->materialPerObjectSetContexts[&materialResource];

	bool contextExists = false;
	for (auto& perObjectSetContext : perObjectSetContexts) {
		if (perObjectSetContext.meshUniform->objectID == meshUniform.objectID) {
			perObjectSetContext.meshUniform = &meshUniform;
			perObjectSetContext.activated = true;
			contextExists = true;
			break;
		}
	}
	if (!contextExists) {
		auto& perObjectSetContext = perObjectSetContexts.emplace_back();
		perObjectSetContext.meshUniform = &meshUniform;
		perObjectSetContext.activated = true;
		contextExists = true;
	}

	_impl->datedPerObjectSetContexts[&materialResource] = &perObjectSetContexts;
	return true;
}

void AfterglowMaterialManager::updateMaterials(
	render::PassUnorderedMap<img::ImageReferences>& allPassImages, bool swapchainImageSetOutdated
) {
	// To avoid resource conflitions if the GPU in flight.
	// @note: It's important for CPU-GPU synchornization.
	if (!_impl->datedMaterialLayouts.empty()) {
		waitGPU();
	}

	uint32_t frameIndex = device().currentFrameIndex();

	_impl->applyGlobalUniformSet(frameIndex);
	// If the swapchain was recreate, rewrite swapchain-relavant pass sets.
	if (swapchainImageSetOutdated) {
		std::fill(_impl->inFlightSwapchainImageSetOutdatedFlags.begin(), _impl->inFlightSwapchainImageSetOutdatedFlags.end(), true);
	}
	if (_impl->inFlightSwapchainImageSetOutdatedFlags[frameIndex]) {
		_impl->applySwapchainPassImageSets(allPassImages, frameIndex);
		_impl->inFlightSwapchainImageSetOutdatedFlags[frameIndex] = false;
	}

	for (auto* matLayout : _impl->datedMaterialLayouts) {
		_impl->appendDatedComputeExternalSSBOContext(*matLayout);
		_impl->applyMaterialLayout(*matLayout);
	}
	for (auto& [matResource, flag] : _impl->datedMaterialResources[frameIndex]) {
		_impl->applyMaterialResource(*matResource, flag, frameIndex);
	}
	for (auto& [materialResource, perObjectSetContexts] : _impl->datedPerObjectSetContexts) {
		_impl->applyPerObjectGlobalSetContext(*materialResource , *perObjectSetContexts, frameIndex);
	}

	for (auto* key : _impl->datedComputeExternalSSBOContextKeys) {
		_impl->applyComputeExternalSSBOContext(*key);
	}

	// Submit resource to device.
	_impl->descriptorSetWriter.write();

	std::lock_guard lock{ _mutex };
	_impl->datedMaterialLayouts.clear();
	_impl->datedMaterialResources[frameIndex].clear();
	_impl->datedPerObjectSetContexts.clear();
	_impl->datedComputeExternalSSBOContextKeys.clear();
}

void AfterglowMaterialManager::updateResources() {
	if (!_impl->perObjectSetContextRemovingCache.empty()
		|| !_impl->materialInstanceRemovingCache.empty()
		|| !_impl->materialRemovingCache.empty()) {
		waitGPU();
	}
	for (auto* perObjectSetContexts : _impl->perObjectSetContextRemovingCache) {
		// Clear inactivated context and reset the flag.
		// Recreate them all is effecient than erase_if due to all context were marked as inactivate.
		perObjectSetContexts->clear();
		// std::erase_if(*perObjectSetContexts, [](auto& context) { return !context.activated; });
	}	
	// Remove materials here due to some vulkan resources (pipelines) inside them.
	for (const auto& name : _impl->materialInstanceRemovingCache){
		_impl->removeMaterialInstanceWithoutLock(name);
	}
	for (const auto& name : _impl->materialRemovingCache) {
		_impl->removeMaterialWithoutLock(name);
	}

	std::lock_guard lock{ _mutex };
	_impl->texturePool.update();
	_impl->materialRemovingCache.clear();
	_impl->perObjectSetContextRemovingCache.clear();
	_impl->materialInstanceRemovingCache.clear();
}

AfterglowMaterialResource& AfterglowMaterialManager::errorMaterialResource() {
	return _impl->materialResources.at(mat::ErrorMaterialName());
}

ubo::GlobalUniform& AfterglowMaterialManager::globalUniform() noexcept {
	return _impl->globalSetContext.globalUniform;
}

AfterglowDescriptorSetReferences* AfterglowMaterialManager::descriptorSetReferences(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform) {
	auto* matResource = materialResource(materialInstanceName);
	auto* setContexts = _impl->perObjectSetContexts(matResource);
	if (!setContexts) {
		return nullptr;
	}
	for (auto& setContext : *setContexts) {
		//if (setContext.meshUniform == &meshUniform) {
		if (setContext.meshUniform->objectID == meshUniform.objectID) {
			// uint32_t frameIndex = device().currentFrameIndex();
			/**
			* @note:
			* Here we use lastFrameIndex due to the draw execution order:
			* func Draw:
			*	-- frame 0 --
			*	...;
			*	materialManager.update();
			*	prepareNextFrameContext(); // Here seperating old frameIndex and new frameIndex
			*	-- frame 1 --
			*	...;
			*	auto& setRefs = descriptorSetReferences(); 
			* 
			* @see: 
			*	AfterglowRenderer::Impl::draw();
			*/
			uint32_t frameIndex = device().lastFrameIndex();
			auto& setRefs = setContext.inFlightSetReferences[frameIndex];
			_impl->applyComputeExternalSSBOSetReference(matResource->materialLayout(), setRefs, frameIndex);
			return &setRefs;
		}
	}
	DEBUG_CLASS_WARNING("SetReferences was not found from this material instance: " + materialInstanceName);
	return nullptr;
}

void AfterglowMaterialManager::applyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset, bool useGlobalResources) {
	util::OptionalRef<AfterglowPassInterface> pass = 
		useGlobalResources ? matLayout.pass() : util::OptionalRef<AfterglowPassInterface>(std::nullopt);

	util::OptionalRef<AfterglowComputeTask::SSBOInfoRefs> associatedSSBOInfos =
		useGlobalResources
		? _impl->computeExternalSSBOContexts[&matLayout].associatedSSBOInfos
		: util::OptionalRef<AfterglowComputeTask::SSBOInfoRefs>(std::nullopt);

	auto& material = matLayout.material();
	if (!material.hasComputeTask() || !material.computeTask().isComputeOnly()) {
		matLayout.compileVertexShader(matAsset.generateShaderCode(
			shader::Stage::Vertex, pass, associatedSSBOInfos
		));
		matLayout.compileFragmentShader(matAsset.generateShaderCode(
			shader::Stage::Fragment, pass, associatedSSBOInfos
		));
	}

	if (material.hasComputeTask()) {
		matLayout.compileComputeShader(matAsset.generateShaderCode(
			shader::Stage::Compute, pass, associatedSSBOInfos
		));
		// No require to apply compute shader initializer, them will generated in updateComputePipeline automatically.
		// @see: AfterglowMaterialLayout::updateComputePipeline
	}
}
