#include "AfterglowMaterialManager.h"
#include <utility>

#include "GlobalAssets.h"
#include "AfterglowImageAsset.h"

AfterglowMaterialManager::AfterglowMaterialManager(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowRenderPass& renderPass, 
	AfterglowAssetMonitor& assetMonitor) :
	_texturePool(commandPool, graphicsQueue),
	_renderPass(renderPass), 
	_assetRegistrar(*this, assetMonitor), 
	_globalDescriptorSetLayout(AfterglowDescriptorSetLayout::makeElement(commandPool.device())),
	_perObjectDescriptorSetLayout(AfterglowDescriptorSetLayout::makeElement(commandPool.device())),
	_descriptorPool(AfterglowDescriptorPool::makeElement(commandPool.device())), 
	_descriptorSetWriter(commandPool.device()) {

	// TODO: Check remaining set size every update, if have not enough size, reset pool and dated all material resources(remember reload layout ).
	(*_descriptorPool).extendUniformPoolSize(cfg::uniformDescriptorSize);
	(*_descriptorPool).extendImageSamplerPoolSize(cfg::samplerDescriptorSize);
	(*_descriptorPool).setMaxDescritporSets(cfg::descriptorSetSize);

	initGlobalDescriptorSet();

	// Initialize PerOjbect set binding[0] : mesh uniform
	(*_perObjectDescriptorSetLayout).appendBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	);
	
	// Initialize ErrorMaterial
	createMaterial(_errorMaterialName, AfterglowMaterial::errorMaterial());
	createMaterialInstance(_errorMaterialName, _errorMaterialName);
}

AfterglowDevice& AfterglowMaterialManager::device() {
	return _texturePool.commandPool().device();
}

AfterglowDescriptorPool& AfterglowMaterialManager::descriptorPool() {
	return _descriptorPool;
}

AfterglowDescriptorSetWriter& AfterglowMaterialManager::descriptorSetWriter() {
	return _descriptorSetWriter;
}

std::string AfterglowMaterialManager::registerMaterialAsset(const std::string& materialPath) {
	return _assetRegistrar.registerMaterialAsset(materialPath);
}

std::string AfterglowMaterialManager::registerMaterialInstanceAsset(const std::string& materialInstancePath) {
	return _assetRegistrar.registerMaterialInstanceAsset(materialInstancePath);
}

void AfterglowMaterialManager::unregisterMaterialAsset(const std::string& materialPath) {
	_assetRegistrar.unregisterMaterialAsset(materialPath);
}

void AfterglowMaterialManager::unregisterMaterialInstanceAsset(const std::string& materialInstancePath) {
	_assetRegistrar.unregisterMaterialInstanceAsset(materialInstancePath);
}

AfterglowMaterial& AfterglowMaterialManager::createMaterial(
	const std::string& name, 
	const AfterglowMaterial& sourceMaterial, 
	util::OptionalRef<AfterglowMaterialAsset> materialAsset) {
	LockGuard lockGuard{_mutex};
	AfterglowMaterialLayout* matLayout = nullptr;
	if (_materialLayouts.find(name) == _materialLayouts.end()) {
		matLayout = &_materialLayouts.emplace(
			name, AfterglowMaterialLayout{_renderPass, sourceMaterial}
		).first->second;
	}
	else {
		matLayout = &_materialLayouts.at(name);
		matLayout->setMaterial(sourceMaterial);
		if (materialAsset != std::nullopt) {
			applyShaders(*matLayout, *materialAsset);
		}
	}
	_datedMaterialLayouts.insert(matLayout);
	return matLayout->material();
}

AfterglowMaterialInstance& AfterglowMaterialManager::createMaterialInstance(const std::string& name, const std::string& parentMaterialName) {
	LockGuard lockGuard{ _mutex };
	return createMaterialInstanceWithoutLock(name, parentMaterialName);
}

AfterglowMaterial* AfterglowMaterialManager::material(const std::string& name) {
	if (_materialLayouts.find(name) != _materialLayouts.end()) {
		return &_materialLayouts.at(name).material();
	}
	return nullptr;
}

AfterglowMaterialLayout* AfterglowMaterialManager::materialLayout(const std::string& name) {
	auto iterator = _materialLayouts.find(name);
	if (iterator != _materialLayouts.end()) {
		return &iterator->second;
	}
	DEBUG_CLASS_WARNING(std::format("Material layout is not exists: \"{}\"", name));
	return nullptr;
}

const AfterglowMaterialLayout* AfterglowMaterialManager::materialLayout(const std::string& name) const {
	return const_cast<AfterglowMaterialManager*>(this)->materialLayout(name);
}

AfterglowMaterialInstance* AfterglowMaterialManager::materialInstance(const std::string& name) {
	if (_materialResources.find(name) != _materialResources.end()) {
		return &_materialResources.at(name).materialInstance();
	}
	return nullptr;
}

AfterglowMaterialResource* AfterglowMaterialManager::materialResource(const std::string& name) {
	auto matResourceIterator = _materialResources.find(name);
	if (matResourceIterator != _materialResources.end()) {
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
	auto layoutIterator = _materialLayouts.find(name);
	if (layoutIterator != _materialLayouts.end()) {
		auto& matLayout = layoutIterator->second;

		// Remove relative material resources.
		std::vector<std::string> invalidMatResourceNames;
		for (auto& [matResourceName, matResource ]: _materialResources) {
			if (&matResource.materialLayout() == &matLayout) {
				_datedMaterialResources.erase(&matResource);
				invalidMatResourceNames.push_back(matResourceName);		
			}
		}
		for (const auto& invalidMatResourceName : invalidMatResourceNames) {
			_materialResources.erase(invalidMatResourceName);
		}

		// Remove material layout
		_datedMaterialLayouts.erase(&matLayout);
		_materialLayouts.erase(layoutIterator);
		return true;
	}
	return false;
}

bool AfterglowMaterialManager::removeMaterialInstance(const std::string& name) {
	LockGuard lockGuard{ _mutex };
	auto matResourceIterator = _materialResources.find(name);
	if (matResourceIterator != _materialResources.end()) {
		auto& matResource = matResourceIterator->second;
		_datedMaterialResources.erase(&matResource);
		_materialPerObjectSetContexts.erase(&matResource);
		_materialResources.erase(matResourceIterator);
		return true;
	}
	return false;
}

bool AfterglowMaterialManager::submitMaterial(const std::string& name) {
	auto iterator = _materialLayouts.find(name);
	if (iterator == _materialLayouts.end()) {
		return false;
	}
	_datedMaterialLayouts.insert(&iterator->second);
	return true;
}

bool AfterglowMaterialManager::submitMaterialInstance(const std::string& name) {
	auto iterator = _materialResources.find(name);
	if (iterator == _materialResources.end()) {
		return false;
	}
	_datedMaterialResources.insert(&iterator->second);
	return true;
}

bool AfterglowMaterialManager::submitMeshUniform(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform) {
	auto matResourceIterator = _materialResources.find(materialInstanceName);
	if (matResourceIterator == _materialResources.end()) {
		if (!instantializeMaterial(materialInstanceName)) {
			return false;
		}
		matResourceIterator = _materialResources.find(materialInstanceName);
	}
	auto& materialResource = matResourceIterator->second;
	auto& perObjectSetContexts = _materialPerObjectSetContexts[&materialResource];

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

	_datedPerObjectSetContexts[&materialResource] = &perObjectSetContexts;
	return true;
}

void AfterglowMaterialManager::update(img::WriteInfoArray& imageWriteInfos, AfterglowSynchronizer& synchronizer) {
	// To avoid compute pipeline conflit.
	if (!_datedMaterialLayouts.empty() || !_datedMaterialResources.empty()) {
		synchronizer.wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	}

	// TODO: Check .modifed for auto submit.
	applyGlobalSetContext(imageWriteInfos);

	for (auto* matLayout : _datedMaterialLayouts) {
		applyMaterialLayout(*matLayout);
	}
	for (auto* matResource : _datedMaterialResources) {
		applyMaterialResource(*matResource);
	}
	for (auto& [materialResource, perObjectSetContexts] : _datedPerObjectSetContexts) {
		applyExternalSetContext(*materialResource , *perObjectSetContexts);
	}
	// Submit resource to device.
	_descriptorSetWriter.write();

	_datedMaterialLayouts.clear();
	_datedMaterialResources.clear();
	_datedPerObjectSetContexts.clear();
}

void AfterglowMaterialManager::updateCompute() {
	// Here use last frame index due to compute material layout and resource will be updated later, current frame is not prepared yet.
	uint32_t frameIndex = device().lastFrameIndex();

	// Apply compute gloabal uniform sets seperatly.
	auto& inFlightBuffer = _globalSetContext.inFlightBuffers[frameIndex];
	if (!inFlightBuffer) {
		return;
	}
	_descriptorSetWriter.registerBuffer(
		*inFlightBuffer,
		_globalDescriptorSetLayout,
		_globalSetContext.inFlightComputeSets[frameIndex][0],
		util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)
	);

	_descriptorSetWriter.write();
}

AfterglowMaterialResource& AfterglowMaterialManager::errorMaterialResource() {
	return _materialResources.at(errorMaterialInstanceName());
}

ubo::GlobalUniform& AfterglowMaterialManager::globalUniform() {
	return _globalSetContext.globalUniform;
}

AfterglowDescriptorSetReferences* AfterglowMaterialManager::descriptorSetReferences(const std::string& materialInstanceName, const ubo::MeshUniform& meshUniform) {
	auto* setContexts = perObjectSetContexts(materialResource(materialInstanceName));
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
	auto* graphicsSetContexts = perObjectSetContexts(materialResource(materialName));
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
	auto& computeSetRefs = _computeMaterialPerObjectSetContexts[matResource].inFlightComputeSetReferences[frameIndex];
	computeSetRefs = *graphicsSetRefs;
	computeSetRefs[util::EnumValue(shader::SetIndex::Global)] =
		_globalSetContext.inFlightComputeSets[frameIndex][util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)];
	return &computeSetRefs;
}

AfterglowMaterialManager::UniqueLock AfterglowMaterialManager::lock() const {
	return UniqueLock{_mutex};
}

inline AfterglowMaterialInstance& AfterglowMaterialManager::createMaterialInstanceWithoutLock(const std::string& name, const std::string& parentMaterialName) {
	AfterglowMaterialLayout* matLayout = nullptr;
	AfterglowMaterialResource* matResource = nullptr;
	if (_materialLayouts.find(parentMaterialName) == _materialLayouts.end()) {
		matLayout = &_materialLayouts.emplace(
			parentMaterialName,
			AfterglowMaterialLayout{ _renderPass }
		).first->second;
		_datedMaterialLayouts.insert(matLayout);
	}
	else {
		matLayout = &_materialLayouts.at(parentMaterialName);
	}
	if (_materialResources.find(name) == _materialResources.end()) {
		matResource = &_materialResources.emplace(
			name,
			AfterglowMaterialResource{ *matLayout, _descriptorSetWriter, _texturePool }
		).first->second;
	}
	else {
		matResource = &_materialResources.at(name);
	}
	_datedMaterialResources.insert(matResource);
	return matResource->materialInstance();
}

inline bool AfterglowMaterialManager::instantializeMaterial(const std::string& name) {
	auto matLayoutIterator = _materialLayouts.find(name);
	if (matLayoutIterator == _materialLayouts.end()) {
		DEBUG_CLASS_ERROR(std::format("Material not found: \"{}\"", name));
		return false;
	}
	// If material exists but have not material instance, try to create one which has same name with material. 
	DEBUG_CLASS_INFO("Material instance not found, same name instance will be created from material: " + name);
	createMaterialInstanceWithoutLock(name, name);
	return true;
}

inline void AfterglowMaterialManager::initGlobalDescriptorSet() {
	// Uniform buffer object description set layout.
	// Additional bindingInfo from enum
	std::vector<shader::GlobalSetBindingIndex> textureBindingIndices;
	Inreflect<shader::GlobalSetBindingIndex>::forEachAttribute([&](auto enumInfo){
		if (enumInfo.name.ends_with(inreflect::EnumName(shader::GlobalSetBindingResource::Uniform))) {
			// Global set binding[0] : global uniform
			(*_globalDescriptorSetLayout).appendBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
			);
		}
		else if (enumInfo.name.ends_with(inreflect::EnumName(shader::GlobalSetBindingResource::Texture))) {
			(*_globalDescriptorSetLayout).appendBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
			);
			textureBindingIndices.push_back(enumInfo.raw);
		}
		else if (enumInfo.name.ends_with(inreflect::EnumName(shader::GlobalSetBindingResource::Sampler))) {
			(*_globalDescriptorSetLayout).appendBinding(
				VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
			);
		}
	});

	// Global set binding[GlobalSetBindingIndex::enumCount] ~ binding[n] : attachment textures.
	auto& subpassContext = _renderPass.subpassContext();
	for (const auto& attachmentInfo : subpassContext.inputAttachmentInfos()) {
		(*_globalDescriptorSetLayout).appendBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		// TODO: Depth should make different procession?
	}

	// Initialize inflight sets.
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		_globalSetContext.inFlightSets[index].recreate(_descriptorPool, _globalDescriptorSetLayout, 1);
	}
	// Seperately compute Sets.
	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		_globalSetContext.inFlightComputeSets[index].recreate(_descriptorPool, _globalDescriptorSetLayout, 1);
	}

	for (auto textureBindingIndex : textureBindingIndices) {
		appendGlobalSetTextureResource(textureBindingIndex);
	}
}

inline void AfterglowMaterialManager::reloadMaterialResources(AfterglowMaterialLayout& matLayout) {
	// Submit all matterial instances of this material.
	// TODO: optimize here.
	for (auto& [matResourceName, matResource] : _materialResources) {
		if (&matResource.materialLayout() == &matLayout) {
			matResource.reloadMaterialLayout(_descriptorPool);
			_datedMaterialResources.insert(&matResource);
		}
	}
}

inline void AfterglowMaterialManager::applyMaterialLayout(AfterglowMaterialLayout& matLayout) {
	matLayout.updateDescriptorSetLayouts(_globalDescriptorSetLayout, _perObjectDescriptorSetLayout);
	try {
		matLayout.updatePipelines();
	}
	catch (const std::runtime_error& error) {
		applyErrorShaders(matLayout);
		DEBUG_CLASS_ERROR(std::format("Failed to apply material, probably some problems occur in shaders: {}", error.what()));
		// After error shader compilation, Retry to update material layout.
		matLayout.updatePipelines();
	}
	// Reload all derived material instances. 
	reloadMaterialResources(matLayout);
}

inline void AfterglowMaterialManager::applyMaterialResource(AfterglowMaterialResource& matResource) {
	try {
		matResource.update();
	}
	catch (std::runtime_error& error) {
		DEBUG_CLASS_ERROR(std::format("Failed to apply material resource, due to: {}", error.what()));
	}
	// Update perObject set material changed flag.
	auto objectSetIterator = _materialPerObjectSetContexts.find(&matResource);
	if (objectSetIterator != _materialPerObjectSetContexts.end()) {

		std::for_each(
			objectSetIterator->second.begin(), 
			objectSetIterator->second.end(), 
			[](auto& context){
				for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
					context.inFlightMaterialChangedFlags[index] = true;
				}
		});
	}
}

inline void AfterglowMaterialManager::applyExternalSetContext(AfterglowMaterialResource& matResource, PerObjectSetContextArray& perObjectSetContexts) {
	int32_t index = device().currentFrameIndex();
	AfterglowDescriptorSets& matResourceSets = matResource.inFlightDescriptorSets()[index];

	// Clear inactivated constext.
	std::erase_if(perObjectSetContexts, [](const auto& context){return !context.activated;});

	// Many objects using same material instance.
	for (auto& perObjectSetContext : perObjectSetContexts) {
		// Initialize set.
		auto& sets = perObjectSetContext.inFlightSets[index];
		auto& buffer = perObjectSetContext.inFlightBuffers[index];
		auto& setReferences = perObjectSetContext.inFlightSetReferences[index];
		bool& materialChanged = perObjectSetContext.inFlightMaterialChangedFlags[index];

		if (!sets) {
			sets.recreate(_descriptorPool, _perObjectDescriptorSetLayout, 1);
		}

		// Initialize / update buffer memory.
		if (!buffer || (*buffer).sourceData() != perObjectSetContext.meshUniform) {
			buffer.recreate(device(), perObjectSetContext.meshUniform, sizeof(ubo::MeshUniform));
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
			_globalSetContext.inFlightSets[index][util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)];
		// Mesh uniform ref
		setReferences[util::EnumValue(shader::SetIndex::PerObject)] = 
			sets[util::EnumValue(shader::PerObjectSetBindingIndex::MeshUniform)];
		
		// Register mesh uniform buffer.
		_descriptorSetWriter.registerBuffer(
			*buffer,
			_perObjectDescriptorSetLayout,
			sets[0],
			util::EnumValue(shader::PerObjectSetBindingIndex::MeshUniform)
		);
		perObjectSetContext.activated = false;
	}
}

inline void AfterglowMaterialManager::applyGlobalSetContext(img::WriteInfoArray& imageWriteInfos) {
	int32_t index = device().currentFrameIndex();

	// Update global uniform buffer memory.
	auto& globalUniform = _globalSetContext.globalUniform;
	auto& buffer = _globalSetContext.inFlightBuffers[index];
	if (!buffer || (*buffer).sourceData() != &globalUniform) {
		buffer.recreate(device(), &globalUniform, sizeof(ubo::GlobalUniform));
	}
	else {
		(*buffer).updateMemory();
	}

	_descriptorSetWriter.registerBuffer(
		*_globalSetContext.inFlightBuffers[index],
		_globalDescriptorSetLayout,
		// GlobalSet, never apply Gloabal data in a per object set
		_globalSetContext.inFlightSets[index][0],
		util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)
	);

	auto& inputAttachmentInfos = _renderPass.subpassContext().inputAttachmentInfos();
	uint32_t attachmentBindingIndex = util::EnumValue(shader::GlobalSetBindingIndex::EnumCount);
	for (const auto& info : inputAttachmentInfos) {
		auto& imageWriteInfo = imageWriteInfos[info.attachmentIndex];
		_descriptorSetWriter.registerImage(
			imageWriteInfo.sampler, 
			imageWriteInfo.imageView, 
			_globalDescriptorSetLayout, 
			_globalSetContext.inFlightSets[index][0],
			attachmentBindingIndex
		);
		++attachmentBindingIndex;
	}
}

void AfterglowMaterialManager::applyErrorShaders(AfterglowMaterialLayout& matLayout) {
	applyShaders(matLayout, errorMaterialAsset());
}

inline void AfterglowMaterialManager::appendGlobalSetTextureResource(shader::GlobalSetBindingIndex textureBindingIndex) {
	auto assetInfo = shader::GlobalSetBindingTextureInfo(textureBindingIndex);
	 
	auto& resource = _globalSetContext.textureResources.emplace_back(
		// std::string(Inreflect<shader::GlobalSetBindingIndex>::enumName(textureBindingIndex)),
		util::EnumValue(textureBindingIndex),
		std::make_unique<AfterglowTextureReference>(_texturePool.texture({ assetInfo }))
	);

	for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
		_descriptorSetWriter.registerImage(
			resource.textureRef->texture(),
			_globalDescriptorSetLayout, 
			_globalSetContext.inFlightSets[index][0], 
			util::EnumValue(textureBindingIndex)
		);
	}
}

inline AfterglowMaterialManager::PerObjectSetContextArray* AfterglowMaterialManager::perObjectSetContexts(AfterglowMaterialResource* matResource) {
	auto perObjectSetIterator = _materialPerObjectSetContexts.find(matResource);
	if (perObjectSetIterator == _materialPerObjectSetContexts.end()) {
		DEBUG_CLASS_WARNING("Have not SetContext was created from this material resource.");
		return nullptr;
	}
	return &perObjectSetIterator->second;
}

void AfterglowMaterialManager::applyShaders(AfterglowMaterialLayout& matLayout, const AfterglowMaterialAsset& matAsset) {
	auto& inputeAttachmentInfos = _renderPass.subpassContext().inputAttachmentInfos();
	matLayout.compileVertexShader(matAsset.generateShaderCode(shader::Stage::Vertex, inputeAttachmentInfos));
	matLayout.compileFragmentShader(matAsset.generateShaderCode(shader::Stage::Fragment, inputeAttachmentInfos));
	if (matLayout.material().hasComputeTask()) {
		matLayout.compileComputeShader(matAsset.generateShaderCode(shader::Stage::Compute, inputeAttachmentInfos));
		// TODO: Apply compute shader initializer?
	}
}
