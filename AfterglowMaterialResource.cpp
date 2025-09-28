#include "AfterglowMaterialResource.h"

#include <algorithm>

#include "AfterglowSSBOInitializer.h"
#include "GlobalAssets.h"
#include "Configurations.h"

AfterglowMaterialResource::AfterglowMaterialResource(
	AfterglowMaterialLayout& materialLayout,
	AfterglowDescriptorSetWriter& descriptorSetWriter,
	AfterglowSharedTexturePool& texturePool
) :
	_materialLayout(materialLayout), 
	_descriptorSetWriter(descriptorSetWriter) , 
	_texturePool(texturePool),
	_materialInstance(materialLayout.material()), 
	_shouldReregisterTextures(false) {
}

AfterglowDevice& AfterglowMaterialResource::device() {
	return _materialLayout.device();
}

void AfterglowMaterialResource::setMateiralInstance(const AfterglowMaterialInstance& materialInstance) {
	_materialInstance = materialInstance;
}

AfterglowMaterialInstance& AfterglowMaterialResource::materialInstance() {
	return _materialInstance;
}

const AfterglowMaterialInstance& AfterglowMaterialResource::materialInstance() const {
	return _materialInstance;
}

AfterglowMaterialLayout& AfterglowMaterialResource::materialLayout() {
	return _materialLayout;
}

const AfterglowMaterialLayout& AfterglowMaterialResource::materialLayout() const {
	return _materialLayout;
}

AfterglowDescriptorSets& AfterglowMaterialResource::descriptorSets() {
	return _inFlightDescriptorSets[device().currentFrameIndex()];
}

const AfterglowDescriptorSets& AfterglowMaterialResource::descriptorSets() const {
	return const_cast<AfterglowMaterialResource*>(this)->descriptorSets();
}

AfterglowMaterialResource::InFlightDescriptorSets& AfterglowMaterialResource::inFlightDescriptorSets() {
	return _inFlightDescriptorSets;
}

const AfterglowMaterialResource::InFlightDescriptorSets& AfterglowMaterialResource::inFlightDescriptorSets() const {
	return _inFlightDescriptorSets;
}

AfterglowStorageBuffer* AfterglowMaterialResource::vertexInputStorageBuffer() {
	auto& material = _materialLayout.material();
	if (!material.hasComputeTask()) {
		// DEBUG_CLASS_WARNING("Can not aquire vertex input storage buffer, due to compute task is not exist.");
		return nullptr;
	}
	auto* ssboInfo = material.computeTask().vertexInputSSBO();
	if (!ssboInfo) {
		// DEBUG_CLASS_WARNING("Can not aquire vertex input storage buffer, due to input vertex SSBO index is not setup.");
		return nullptr;
	}
	auto resourcesIterator = _stageResources.find(shader::Stage::ComputeVertex);
	if (resourcesIterator == _stageResources.end()) {
		// DEBUG_CLASS_WARNING("Can not aquire vertex input storage buffer, due to resource is not loaded.");
		return nullptr;
	}
	auto& resources = resourcesIterator->second;
	auto frameStorageBufferResourcesIterator = resources.storageBufferResources.find(ssboInfo->name);
	if (frameStorageBufferResourcesIterator == resources.storageBufferResources.end()) {
		// DEBUG_CLASS_WARNING("Can not aquire vertex input storage buffer, due to resource is not loaded.");
		return nullptr;
	}
	// TODO: Read only single SSBO support.
	return frameStorageBufferResourcesIterator->second[device().currentFrameIndex()].buffer;
}

const AfterglowStorageBuffer* AfterglowMaterialResource::vertexInputStorageBuffer() const {
	return const_cast<AfterglowMaterialResource*>(this)->vertexInputStorageBuffer();
}

void AfterglowMaterialResource::update() {
	submitUniforms();
	submitTextures();
	if (_materialLayout.material().hasComputeTask()) {
		submitStorageBuffers();
	}
}

void AfterglowMaterialResource::reloadMaterialLayout(AfterglowDescriptorPool& descriptorPool) {
	_shouldReregisterTextures = true;
	// Reset material insteance parent.
	if (!_materialInstance.parentMaterial().is(_materialLayout.material())) {
		_materialInstance = _materialInstance.makeRedirectedInstance(_materialLayout.material());
	}
	// Recreate descriptor sets
	// TODO: Set Destroy Conflit here
	for (int index = 0; index < cfg::maxFrameInFlight; ++index) {
		_inFlightDescriptorSets[index].recreate(
			descriptorPool,
			AfterglowDescriptorSets::RawLayoutArray{
				// 0 is global descriptor set.
				_materialLayout.rawDescriptorSetLayouts().begin() + shader::MaterialSetIndexBegin, _materialLayout.rawDescriptorSetLayouts().end()
			},
			shader::MaterialSetIndexBegin
		);
	}
}

inline AfterglowMaterialResource::TextureResource* AfterglowMaterialResource::aquireTextureResource(shader::Stage stage, const std::string name) {
	auto& textureResources = _stageResources[stage].textureResources;
	auto iterator = textureResources.find(name);
	if (iterator == textureResources.end()) {
		DEBUG_CLASS_WARNING("Stage not found: " + std::string(inreflect::EnumName(stage)));
		return nullptr;
	}
	return &(iterator->second);
}

void AfterglowMaterialResource::submitUniforms() {
	// TODO: Considering .modified, don't clear every update.
	// Fill scalars.
	for (const auto& [stage, scalarParams] : _materialInstance.scalars()) {
		for (const auto& scalarParam : scalarParams) {
			_stageResources[stage].uniforms.push_back(scalarParam.value);
		}
	}

	// Memory alignment.
	for (auto& [stage, resource] : _stageResources) {
		resource.uniforms.resize(_materialLayout.material().scalarPaddingSize(stage) + resource.uniforms.size());
	}

	// Fill vectors.
	for (const auto& [stage, vectorParams] : _materialInstance.vectors()) {
		auto& resource = _stageResources[stage];
		for (const auto& vectorParam : vectorParams) {
			for (uint32_t index = 0; index < AfterglowMaterial::elementAlignment(); ++index) {
				resource.uniforms.push_back(vectorParam.value[index]);
			}
		}
	}

	// Create uniform buffers.
	for (auto& [stage, resources] : _stageResources) {
		auto& uniforms = resources.uniforms;
		if (uniforms.empty()) {
			continue;
		}
		for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
			resources.uniformBuffers[index].recreate(device(), uniforms.data(), uniforms.size() * sizeof(AfterglowMaterial::Scalar));
		}
	}

	// Write DescriptorSets
	for (uint32_t frameIndex = 0; frameIndex < cfg::maxFrameInFlight; ++frameIndex) {
		auto& descriptorSets = _inFlightDescriptorSets[frameIndex];
		// 0 is global descriptor set.
		for (auto& [stage, resource] : _stageResources) {
			if (resource.uniforms.empty()) {
				continue;
			}
			auto& uniformBuffer = resource.uniformBuffers[frameIndex];
			// First binding (index = 0) is always uniform, after then all texture bindings behind it.
			auto& setLayout = _materialLayout.descriptorSetLayouts()[stage];
			_descriptorSetWriter.registerBuffer(*uniformBuffer, setLayout, descriptorSets[util::EnumValue(stage)], 0);
		}
	}
}

void AfterglowMaterialResource::submitTextures() {
	// Load and submit Textures.
	synchronizeTextures();
	if (_shouldReregisterTextures) {
		reregisterUnmodifiedTextures();
		_shouldReregisterTextures = false;
	}
	reloadModifiedTextures();
}

inline void AfterglowMaterialResource::submitStorageBuffers() {
	synchronizeStorageBuffers();
	// Submit storage buffers
	for (uint32_t frameIndex = 0; frameIndex < cfg::maxFrameInFlight; ++frameIndex) {
		auto& stageDescriptorSets = _inFlightDescriptorSets[frameIndex];
		for(auto& [stage, resources] : _stageResources) {
			auto& ssboResources = resources.storageBufferResources;
			auto& set = stageDescriptorSets[util::EnumValue(stage)];
			auto& setLayout = _materialLayout.descriptorSetLayouts()[stage];
			registerFrameStorageBuffers(frameIndex, ssboResources, setLayout, set);
		}
	}
}

inline void AfterglowMaterialResource::synchronizeTextures() {
	// Check if old textures were removed from material instance.
	for (auto& [stage, resources] : _stageResources) {
		auto& textureResources = resources.textureResources;
		std::erase_if(textureResources, [this, stage](const auto& item) { return !_materialInstance.texture(stage, item.first); });
	}

	// Refleshing binding indices here, to avoid adding and removing influence.
	for (auto& [stage, textureParams] : _materialInstance.textures()) {
		auto& textureResources = _stageResources[stage].textureResources;
		for (uint32_t index = 0; index < textureParams.size(); ++index) {
			textureResources[textureParams[index].name].bindingIndex = index + 1;
		}
	}
}

inline void AfterglowMaterialResource::reregisterUnmodifiedTextures() {
	for (const auto& [stage, textureParams] : _materialInstance.textures()) {
		for (const auto& textureParam : textureParams) {
			if (textureParam.modified) {
				continue;
			}
			// If synchronizeTexturesFromParameters() was called before, here will not return a nullptr.
			auto& textureResource = *aquireTextureResource(stage, textureParam.name);
			auto& setLayout = (*_materialLayout.descriptorSetLayouts()[stage]);
			for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
				_descriptorSetWriter.registerImage(
					textureResource.textureRef->texture(),
					setLayout,
					*(*_inFlightDescriptorSets[index]).find(setLayout),  // Ugly find, try to remove it.
					textureResource.bindingIndex
				);
			}
		}
	}
}

inline void AfterglowMaterialResource::reloadModifiedTextures() {
	auto& textures = _materialInstance.textures();
	for (auto& [stage, textureParams] : textures) {
		for (auto& textureParam : textureParams) {
			std::string texturePath = textureParam.value.path;
			// Handle no texture condition.
			if (texturePath == "") {
				DEBUG_CLASS_WARNING("Material texture path is empty, texture name: " + textureParam.name);
				texturePath = img::defaultTextureInfo.path;
				textureParam.modified = true;
			}
			if (textureParam.value.colorSpace == img::ColorSpace::Undefined) {
				textureParam.value.colorSpace = img::ColorSpace::SRGB;
				DEBUG_CLASS_WARNING("Material texture color space is redirected to SRGB, due to it is undefined, texture name:" + textureParam.name);
			}
			if (!textureParam.modified) {
				continue;
			}

			auto& textureResource = *aquireTextureResource(stage, textureParam.name);
			textureResource.textureRef = std::make_unique<AfterglowTextureReference>(
				_texturePool.texture({ textureParam.value.format , textureParam.value.colorSpace , texturePath })
			);

			auto& setLayout = (*_materialLayout.descriptorSetLayouts()[stage]);
			for (uint32_t index = 0; index < cfg::maxFrameInFlight; ++index) {
				_descriptorSetWriter.registerImage(
					textureResource.textureRef->texture(),
					setLayout,
					*(*_inFlightDescriptorSets[index]).find(setLayout),  // Ugly find, try to remove it.
					textureResource.bindingIndex
				);
			}
			textureParam.modified = false;
		}
	}
}

inline void AfterglowMaterialResource::synchronizeStorageBuffers() {
	// Clear outdated resources.
	// TODO: Reload all resource, otherwise it will bring very terrible complexity.
	// TODO: Textures also should do that.
	for(auto& [stage, resources] : _stageResources) {
		auto& ssboResources = resources.storageBufferResources;
		auto& computeTask = _materialLayout.material().computeTask();
		std::erase_if(ssboResources, [&computeTask](const auto& item){
			return computeTask.findSSBOInfo(item.first) == computeTask.ssboInfos().end();
		});
	};

	// Init Buffers
	const auto& computeTask = _materialLayout.material().computeTask();
	const auto& ssboInfos = computeTask.ssboInfos();
	std::unordered_map<shader::Stage, uint32_t> bindingIndices;
	for (const auto& ssboInfo : ssboInfos) {
		auto& ssboResources = _stageResources[ssboInfo.stage].storageBufferResources;

		// Binding index: Minimum index == 1 due to index 0 is uniform.
		if (bindingIndices.find(ssboInfo.stage) == bindingIndices.end()) {
			bindingIndices.emplace(ssboInfo.stage, _stageResources[ssboInfo.stage].textureResources.size() + 1);
		}
		uint32_t& bindingIndex = bindingIndices.at(ssboInfo.stage);

		auto& frameSSBOResources = ssboResources[ssboInfo.name];
		uint32_t numFrameSSBOs = computeTask.numSSBOs(ssboInfo);
		if (frameSSBOResources.empty()) {
			// TODO: Reintialize ssbo if .mat compute task was changed.
			AfterglowSSBOInitializer initializer{ ssboInfo };
			AfterglowStagingBuffer stagingBuffer(device(), initializer.data(), initializer.byteSize());
			for (uint32_t index = 0; index < numFrameSSBOs; ++index) {
				auto& ssboResource = frameSSBOResources.emplace_back();
				ssboResource.buffer.recreate(device(), initializer.data(), initializer.byteSize());
				(*ssboResource.buffer).submit(_texturePool.commandPool(), _texturePool.graphicsQueue(), stagingBuffer);
			}
		}
		for (uint32_t index = 0; index < numFrameSSBOs; ++index) {
			frameSSBOResources[index].bindingIndex = bindingIndex;
			++bindingIndex;
		}
	}
}

inline void AfterglowMaterialResource::registerFrameStorageBuffers(
	uint32_t frameIndex,
	StageResource::StorageBufferResources& ssboResources,
	AfterglowDescriptorSetLayout& setLayout,
	VkDescriptorSet& set) {
	// TODO: Read only single SSBO support.
	for (auto& [name, frameSSBOResources] : ssboResources) {
		for (uint32_t ssboIndex = 0; ssboIndex < frameSSBOResources.size(); ++ssboIndex) {
			uint32_t bufferIndex = (cfg::maxFrameInFlight + frameIndex - ssboIndex - 1) % cfg::maxFrameInFlight;
			_descriptorSetWriter.registerBuffer(
				*frameSSBOResources[bufferIndex].buffer, setLayout, set, frameSSBOResources[ssboIndex].bindingIndex
			);
		}
	}
}