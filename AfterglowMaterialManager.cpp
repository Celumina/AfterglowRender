#include "AfterglowMaterialManager.h"
#include <utility>

#include <map>
#include <unordered_set>
#include <unordered_map>

#include "GlobalAssets.h"
#include "AfterglowMaterialAsset.h"
#include "AfterglowSynchronizer.h"
#include "AfterglowMaterialAssetRegistrar.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowDescriptorSetWriter.h"
#include "AfterglowDescriptorSetReferences.h"


struct AfterglowMaterialManager::Impl {
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
		ubo::GlobalUniform globalUniform{};
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

	Impl(
		AfterglowMaterialManager& managerRef, 
		AfterglowCommandPool& CommandPoolRef,
		AfterglowGraphicsQueue& graphicsQueueRef,
		AfterglowRenderPass& renderPassRef,
		AfterglowAssetMonitor& assetMonitorRef
	);

	inline AfterglowMaterialInstance& createMaterialInstanceWithoutLock(const std::string& name, const std::string& parentMaterialName);
	inline bool instantializeMaterial(const std::string& name);

	inline void initGlobalDescriptorSet();

	// Call it when that material submit.
	inline void reloadMaterialResources(AfterglowMaterialLayout& matLayout);

	inline void applyMaterialLayout(AfterglowMaterialLayout& matLayout);
	inline void applyMaterialResource(AfterglowMaterialResource& matResource);
	inline void applyExternalSetContext(AfterglowMaterialResource& matResource, PerObjectSetContextArray& perObjectSetContexts);
	inline void applyGlobalSetContext(img::WriteInfoArray& imageWriteInfos);

	inline void appendGlobalSetTextureResource(shader::GlobalSetBindingIndex textureBindingIndex);

	inline PerObjectSetContextArray* perObjectSetContexts(AfterglowMaterialResource* matResource);

	// Static Pool Size
	// TODO: add a new pool for dynamic pool size.
	// Place front to make sure descriptor set destruct later than descriptor sets.
	AfterglowDescriptorPool::AsElement descriptorPool;
	AfterglowSharedTexturePool texturePool;

	// TODO: Different subpass as material domain.
	AfterglowRenderPass& renderPass;

	AfterglowMaterialAssetRegistrar assetRegistrar;

	MaterialLayouts materialLayouts;
	MaterialResources materialResources;

	// Global Set Layout
	AfterglowDescriptorSetLayout::AsElement globalDescriptorSetLayout;
	// Global Uniform Resources
	GlobalSetContext globalSetContext;

	// PerObject Set Layout
	AfterglowDescriptorSetLayout::AsElement perObjectDescriptorSetLayout;
	// Mesh Uniform Resources
	MaterialPerObjectSetContexts materialPerObjectSetContexts;
	ComputeMaterialPerObjectSetContexts computeMaterialPerObjectSetContexts;

	AfterglowDescriptorSetWriter descriptorSetWriter;

	DatedMaterialLayouts datedMaterialLayouts;
	DatedMaterialResources datedMaterialResources;
	DatedPerObjectSetContexts datedPerObjectSetContexts;

	AfterglowMaterialManager& manager;
};

AfterglowMaterialManager::Impl::Impl(
	AfterglowMaterialManager& managerRef,
	AfterglowCommandPool& commandPoolRef, 
	AfterglowGraphicsQueue& graphicsQueueRef, 
	AfterglowRenderPass& renderPassRef, 
	AfterglowAssetMonitor& assetMonitorRef) :
	texturePool(commandPoolRef, graphicsQueueRef),
	renderPass(renderPassRef),
	assetRegistrar(managerRef, assetMonitorRef),
	globalDescriptorSetLayout(AfterglowDescriptorSetLayout::makeElement(commandPoolRef.device())),
	perObjectDescriptorSetLayout(AfterglowDescriptorSetLayout::makeElement(commandPoolRef.device())),
	descriptorPool(AfterglowDescriptorPool::makeElement(commandPoolRef.device())),
	descriptorSetWriter(commandPoolRef.device()), 
	manager(managerRef) {
	// TODO: Check remaining set size every update, if have not enough size, reset pool and dated all material resources(remember reload layout ).
	(*descriptorPool).extendUniformPoolSize(cfg::uniformDescriptorSize);
	(*descriptorPool).extendImageSamplerPoolSize(cfg::samplerDescriptorSize);
	(*descriptorPool).setMaxDescritporSets(cfg::descriptorSetSize);

	initGlobalDescriptorSet();

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
			AfterglowMaterialLayout{ renderPass }
		).first->second;
		datedMaterialLayouts.insert(matLayout);
	}
	else {
		matLayout = &materialLayouts.at(parentMaterialName);
	}
	if (materialResources.find(name) == materialResources.end()) {
		matResource = &materialResources.emplace(
			name,
			AfterglowMaterialResource{ *matLayout, descriptorSetWriter, texturePool }
		).first->second;
	}
	else {
		matResource = &materialResources.at(name);
	}
	datedMaterialResources.insert(matResource);
	return matResource->materialInstance();
}

inline bool AfterglowMaterialManager::Impl::instantializeMaterial(const std::string& name) {
	auto matLayoutIterator = materialLayouts.find(name);
	if (matLayoutIterator == materialLayouts.end()) {
		DEBUG_CLASS_ERROR(std::format("Material not found: \"{}\"", name));
		return false;
	}
	// If material exists but have not material instance, try to create one which has same name with material. 
	DEBUG_CLASS_INFO("Material instance not found, same name instance will be created from material: " + name);
	createMaterialInstanceWithoutLock(name, name);
	return true;
}

inline void AfterglowMaterialManager::Impl::initGlobalDescriptorSet() {
	// Uniform buffer object description set layout.
	// Additional bindingInfo from enum
	std::vector<shader::GlobalSetBindingIndex> textureBindingIndices;
	Inreflect<shader::GlobalSetBindingIndex>::forEachAttribute([&](auto enumInfo) {
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

	// Global set binding[GlobalSetBindingIndex::enumCount] ~ binding[n] : attachment textures.
	auto& subpassContext = renderPass.subpassContext();
	for (const auto& attachmentInfo : subpassContext.inputAttachmentInfos()) {
		(*globalDescriptorSetLayout).appendBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		// TODO: Depth should make different procession?
	}

	// Initialize inflight sets.
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		globalSetContext.inFlightSets[index].recreate(descriptorPool, globalDescriptorSetLayout, 1);
	}
	// Seperately compute Sets.
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		globalSetContext.inFlightComputeSets[index].recreate(descriptorPool, globalDescriptorSetLayout, 1);
	}

	for (auto textureBindingIndex : textureBindingIndices) {
		appendGlobalSetTextureResource(textureBindingIndex);
	}
}

inline void AfterglowMaterialManager::Impl::reloadMaterialResources(AfterglowMaterialLayout& matLayout) {
	// Submit all matterial instances of this material.
	// TODO: optimize here.
	for (auto& [matResourceName, matResource] : materialResources) {
		if (&matResource.materialLayout() == &matLayout) {
			matResource.reloadMaterialLayout(descriptorPool);
			datedMaterialResources.insert(&matResource);
		}
	}
}

inline void AfterglowMaterialManager::Impl::applyMaterialLayout(AfterglowMaterialLayout& matLayout) {
	matLayout.updateDescriptorSetLayouts(globalDescriptorSetLayout, perObjectDescriptorSetLayout);
	try {
		matLayout.updatePipelines();
	}
	catch (const std::runtime_error& error) {
		manager.applyErrorShaders(matLayout);
		DEBUG_CLASS_ERROR(std::format("Failed to apply material, probably some problems occur in shaders: {}", error.what()));
		// After error shader compilation, Retry to update material layout.
		matLayout.updatePipelines();
	}
	// Reload all derived material instances. 
	reloadMaterialResources(matLayout);
}

inline void AfterglowMaterialManager::Impl::applyMaterialResource(AfterglowMaterialResource& matResource) {
	try {
		matResource.update();
	}
	catch (std::runtime_error& error) {
		DEBUG_CLASS_ERROR(std::format("Failed to apply material resource, due to: {}", error.what()));
	}
	// Update perObject set material changed flag.
	auto objectSetIterator = materialPerObjectSetContexts.find(&matResource);
	if (objectSetIterator != materialPerObjectSetContexts.end()) {

		std::for_each(
			objectSetIterator->second.begin(),
			objectSetIterator->second.end(),
			[](auto& context) {
				for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
					context.inFlightMaterialChangedFlags[index] = true;
				}
			});
	}
}

inline void AfterglowMaterialManager::Impl::applyExternalSetContext(AfterglowMaterialResource& matResource, PerObjectSetContextArray& perObjectSetContexts) {
	int32_t index = manager.device().currentFrameIndex();
	AfterglowDescriptorSets& matResourceSets = matResource.inFlightDescriptorSets()[index];

	// Clear inactivated constext.
	std::erase_if(perObjectSetContexts, [](const auto& context) {return !context.activated; });

	// Many objects using same material instance.
	for (auto& perObjectSetContext : perObjectSetContexts) {
		// Initialize set.
		auto& sets = perObjectSetContext.inFlightSets[index];
		auto& buffer = perObjectSetContext.inFlightBuffers[index];
		auto& setReferences = perObjectSetContext.inFlightSetReferences[index];
		bool& materialChanged = perObjectSetContext.inFlightMaterialChangedFlags[index];

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
			materialChanged = false;
		}
		// Global uniform ref
		setReferences[util::EnumValue(shader::SetIndex::Global)] =
			globalSetContext.inFlightSets[index][util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)];
		// Mesh uniform ref
		setReferences[util::EnumValue(shader::SetIndex::PerObject)] =
			sets[util::EnumValue(shader::PerObjectSetBindingIndex::MeshUniform)];

		// Register mesh uniform buffer.
		descriptorSetWriter.registerBuffer(
			*buffer,
			perObjectDescriptorSetLayout,
			sets[0],
			util::EnumValue(shader::PerObjectSetBindingIndex::MeshUniform)
		);
		perObjectSetContext.activated = false;
	}
}

inline void AfterglowMaterialManager::Impl::applyGlobalSetContext(img::WriteInfoArray& imageWriteInfos) {
	int32_t index = manager.device().currentFrameIndex();

	// Update global uniform buffer memory.
	auto& globalUniform = globalSetContext.globalUniform;
	auto& buffer = globalSetContext.inFlightBuffers[index];
	if (!buffer || (*buffer).sourceData() != &globalUniform) {
		buffer.recreate(manager.device(), &globalUniform, sizeof(ubo::GlobalUniform));
	}
	else {
		(*buffer).updateMemory();
	}

	descriptorSetWriter.registerBuffer(
		*globalSetContext.inFlightBuffers[index],
		globalDescriptorSetLayout,
		// GlobalSet, never apply Gloabal data in a per object set
		globalSetContext.inFlightSets[index][0],
		util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)
	);

	auto& inputAttachmentInfos = renderPass.subpassContext().inputAttachmentInfos();
	uint32_t attachmentBindingIndex = util::EnumValue(shader::GlobalSetBindingIndex::EnumCount);
	for (const auto& info : inputAttachmentInfos) {
		auto& imageWriteInfo = imageWriteInfos[info.attachmentIndex];
		descriptorSetWriter.registerImage(
			imageWriteInfo.sampler,
			imageWriteInfo.imageView,
			globalDescriptorSetLayout,
			globalSetContext.inFlightSets[index][0],
			attachmentBindingIndex
		);
		++attachmentBindingIndex;
	}
}

void AfterglowMaterialManager::applyErrorShaders(AfterglowMaterialLayout& matLayout) {
	applyShaders(matLayout, errorMaterialAsset());
}

inline void AfterglowMaterialManager::Impl::appendGlobalSetTextureResource(shader::GlobalSetBindingIndex textureBindingIndex) {
	auto assetInfo = shader::GlobalSetBindingTextureInfo(textureBindingIndex);

	auto& resource = globalSetContext.textureResources.emplace_back(
		// std::string(Inreflect<shader::GlobalSetBindingIndex>::enumName(textureBindingIndex)),
		util::EnumValue(textureBindingIndex),
		std::make_unique<AfterglowTextureReference>(texturePool.texture({ assetInfo }))
	);

	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		descriptorSetWriter.registerImage(
			resource.textureRef->texture(),
			globalDescriptorSetLayout,
			globalSetContext.inFlightSets[index][0],
			util::EnumValue(textureBindingIndex)
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

AfterglowMaterialManager::AfterglowMaterialManager(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowRenderPass& renderPass, 
	AfterglowAssetMonitor& assetMonitor) :
	_impl(std::make_unique<Impl>(*this, commandPool, graphicsQueue, renderPass, assetMonitor)) {
	
	// Initialize ErrorMaterial
	createMaterial(_errorMaterialName, AfterglowMaterial::errorMaterial());
	createMaterialInstance(_errorMaterialName, _errorMaterialName);
}

AfterglowMaterialManager::~AfterglowMaterialManager() {
}

AfterglowDevice& AfterglowMaterialManager::device() noexcept {
	return _impl->texturePool.commandPool().device();
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
	if (matLayouts.find(name) == matLayouts.end()) {
		matLayout = &matLayouts.emplace(
			name, AfterglowMaterialLayout{ _impl->renderPass, *safeSrcMaterial }
		).first->second;
	}
	else {
		matLayout = &matLayouts.at(name);
		matLayout->setMaterial(*safeSrcMaterial);
		if (materialAsset != std::nullopt) {
			applyShaders(*matLayout, *materialAsset);
		}
	}
	_impl->datedMaterialLayouts.insert(matLayout);
	return matLayout->material();
}

AfterglowMaterialInstance& AfterglowMaterialManager::createMaterialInstance(const std::string& name, const std::string& parentMaterialName) {
	LockGuard lockGuard{ _mutex };
	return _impl->createMaterialInstanceWithoutLock(name, parentMaterialName);
}

AfterglowMaterial* AfterglowMaterialManager::material(const std::string& name) {
	auto& matLayouts = _impl->materialLayouts;
	if (matLayouts.find(name) != matLayouts.end()) {
		return &matLayouts.at(name).material();
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
	if (matResources.find(name) != matResources.end()) {
		return &matResources.at(name).materialInstance();
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

AfterglowMaterialAsset& AfterglowMaterialManager::errorMaterialAsset() {
	static AfterglowMaterialAsset errorMaterialAsset(AfterglowMaterial::errorMaterial());
	return errorMaterialAsset;
}

const std::string& AfterglowMaterialManager::errorMaterialName() {
	static std::string errorMaterialName(_errorMaterialName);
	return errorMaterialName;
}

const std::string& AfterglowMaterialManager::errorMaterialInstanceName() {
	return errorMaterialName();
}

bool AfterglowMaterialManager::removeMaterial(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	auto& matLayouts = _impl->materialLayouts;
	auto& matResources = _impl->materialResources;
	auto layoutIterator = matLayouts.find(name);
	if (layoutIterator != matLayouts.end()) {
		auto& matLayout = layoutIterator->second;

		// Remove relative material resources.
		std::vector<std::string> invalidMatResourceNames;
		for (auto& [matResourceName, matResource ]: matResources) {
			if (&matResource.materialLayout() == &matLayout) {
				_impl->datedMaterialResources.erase(&matResource);
				invalidMatResourceNames.push_back(matResourceName);		
			}
		}
		for (const auto& invalidMatResourceName : invalidMatResourceNames) {
			matResources.erase(invalidMatResourceName);
		}

		// Remove material layout
		_impl->datedMaterialLayouts.erase(&matLayout);
		matLayouts.erase(layoutIterator);
		return true;
	}
	return false;
}

bool AfterglowMaterialManager::removeMaterialInstance(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	auto& matResources = _impl->materialResources;
	auto matResourceIterator = matResources.find(name);
	if (matResourceIterator != matResources.end()) {
		auto& matResource = matResourceIterator->second;
		_impl->datedMaterialResources.erase(&matResource);
		_impl->materialPerObjectSetContexts.erase(&matResource);
		matResources.erase(matResourceIterator);
		return true;
	}
	return false;
}

bool AfterglowMaterialManager::submitMaterial(const std::string& name) {
	auto iterator = _impl->materialLayouts.find(name);
	if (iterator == _impl->materialLayouts.end()) {
		return false;
	}
	_impl->datedMaterialLayouts.insert(&iterator->second);
	return true;
}

bool AfterglowMaterialManager::submitMaterialInstance(const std::string& name) {
	auto iterator = _impl->materialResources.find(name);
	if (iterator == _impl->materialResources.end()) {
		return false;
	}
	_impl->datedMaterialResources.insert(&iterator->second);
	return true;
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

void AfterglowMaterialManager::update(img::WriteInfoArray& imageWriteInfos, AfterglowSynchronizer& synchronizer) {
	// To avoid compute pipeline conflit.
	if (!_impl->datedMaterialLayouts.empty() || !_impl->datedMaterialResources.empty()) {
		synchronizer.wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	}

	// TODO: Check .modifed for auto submit.
	_impl->applyGlobalSetContext(imageWriteInfos);

	for (auto* matLayout : _impl->datedMaterialLayouts) {
		_impl->applyMaterialLayout(*matLayout);
	}
	for (auto* matResource : _impl->datedMaterialResources) {
		_impl->applyMaterialResource(*matResource);
	}
	for (auto& [materialResource, perObjectSetContexts] : _impl->datedPerObjectSetContexts) {
		_impl->applyExternalSetContext(*materialResource , *perObjectSetContexts);
	}
	// Submit resource to device.
	_impl->descriptorSetWriter.write();

	_impl->datedMaterialLayouts.clear();
	_impl->datedMaterialResources.clear();
	_impl->datedPerObjectSetContexts.clear();
}

void AfterglowMaterialManager::updateCompute() {
	// Here use last frame index due to compute material layout and resource will be updated later, current frame is not prepared yet.
	uint32_t frameIndex = device().lastFrameIndex();

	// Apply compute gloabal uniform sets seperatly.
	auto& inFlightBuffer = _impl->globalSetContext.inFlightBuffers[frameIndex];
	if (!inFlightBuffer) {
		return;
	}
	_impl->descriptorSetWriter.registerBuffer(
		*inFlightBuffer,
		_impl->globalDescriptorSetLayout,
		_impl->globalSetContext.inFlightComputeSets[frameIndex][0],
		util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)
	);

	_impl->descriptorSetWriter.write();
}

AfterglowMaterialResource& AfterglowMaterialManager::errorMaterialResource() {
	return _impl->materialResources.at(errorMaterialInstanceName());
}

ubo::GlobalUniform& AfterglowMaterialManager::globalUniform() noexcept {
	return _impl->globalSetContext.globalUniform;
}

AfterglowDescriptorSetReferences* AfterglowMaterialManager::descriptorSetReferences(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform) {
	auto* setContexts = _impl->perObjectSetContexts(materialResource(materialInstanceName));
	if (!setContexts) {
		return nullptr;
	}
	for (auto& setContext : *setContexts) {
		if (setContext.meshUniform == &meshUniform) {
			return &setContext.inFlightSetReferences[device().currentFrameIndex()];
		}
	}
	DEBUG_CLASS_WARNING("SetReferences was not found from this material instance: " + materialInstanceName);
	return nullptr;
}

AfterglowDescriptorSetReferences* AfterglowMaterialManager::computeDescriptorSetReferences(const std::string& materialName, const ubo::MeshUniform& meshUniform) {
	auto* matResource = materialResource(materialName);
	if (!matResource) {
		return nullptr;
	}
	auto* graphicsSetContexts = _impl->perObjectSetContexts(materialResource(materialName));
	if (!graphicsSetContexts) {
		return nullptr;
	}
	// Here use last frame index due to compute material layout and resource will be updated later, current frame is not prepared yet.
	uint32_t frameIndex = device().lastFrameIndex();
	AfterglowDescriptorSetReferences* graphicsSetRefs = nullptr;
	for (auto& setContext : *graphicsSetContexts) {
		if (setContext.meshUniform == &meshUniform) {
			graphicsSetRefs = &setContext.inFlightSetReferences[frameIndex];
		}
	}
	if (!graphicsSetRefs || !graphicsSetRefs->source()) {
		return nullptr;
	}
	auto& computeSetRefs = 
		_impl->computeMaterialPerObjectSetContexts[matResource].inFlightComputeSetReferences[frameIndex];
	computeSetRefs = *graphicsSetRefs;
	computeSetRefs[util::EnumValue(shader::SetIndex::Global)] =
		_impl->globalSetContext.inFlightComputeSets[frameIndex][util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)];
	return &computeSetRefs;
}

std::mutex& AfterglowMaterialManager::mutex() {
	return _mutex;
}

void AfterglowMaterialManager::applyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset) {
	auto& inputeAttachmentInfos = _impl->renderPass.subpassContext().inputAttachmentInfos();
	matLayout.compileVertexShader(matAsset.generateShaderCode(shader::Stage::Vertex, inputeAttachmentInfos));
	matLayout.compileFragmentShader(matAsset.generateShaderCode(shader::Stage::Fragment, inputeAttachmentInfos));
	if (matLayout.material().hasComputeTask()) {
		matLayout.compileComputeShader(matAsset.generateShaderCode(shader::Stage::Compute, inputeAttachmentInfos));
		// TODO: Apply compute shader initializer?
	}
}
