#include "AfterglowMaterialResource.h"

#include <algorithm>

#include "AfterglowSSBOInitializer.h"
#include "AfterglowComputeTask.h"
#include "AfterglowDescriptorSetWriter.h"
#include "GlobalAssets.h"
#include "Configurations.h"

AfterglowMaterialResource::AfterglowMaterialResource(
	AfterglowMaterialLayout& materialLayout,
	AfterglowDescriptorSetWriter& descriptorSetWriter,
	AfterglowDescriptorPool& descriptorPool, 
	AfterglowSharedTexturePool& texturePool
) :
	_materialLayout(materialLayout), 
	_descriptorSetWriter(descriptorSetWriter) , 
	_texturePool(texturePool),
	_descriptorPool(descriptorPool), 
	_materialInstance(materialLayout.material()), 
	_shouldReregisterTextures(false) {

	if (!_materialLayout.rawDescriptorSetLayouts().empty()) {
		reloadMaterialLayout();
	}	
}

AfterglowDevice& AfterglowMaterialResource::device() noexcept {
	return _materialLayout.device();
}

void AfterglowMaterialResource::setMateiralInstance(const AfterglowMaterialInstance& materialInstance) noexcept {
	_materialInstance = materialInstance;
}

AfterglowMaterialInstance& AfterglowMaterialResource::materialInstance() noexcept {
	return _materialInstance;
}

const AfterglowMaterialInstance& AfterglowMaterialResource::materialInstance() const noexcept {
	return _materialInstance;
}

AfterglowMaterialLayout& AfterglowMaterialResource::materialLayout() noexcept {
	return _materialLayout;
}

const AfterglowMaterialLayout& AfterglowMaterialResource::materialLayout() const noexcept {
	return _materialLayout;
}

AfterglowDescriptorSets& AfterglowMaterialResource::descriptorSets() {
	return _inFlightDescriptorSets[device().currentFrameIndex()];
}

const AfterglowDescriptorSets& AfterglowMaterialResource::descriptorSets() const {
	return const_cast<AfterglowMaterialResource*>(this)->descriptorSets();
}

AfterglowMaterialResource::InFlightDescriptorSets& AfterglowMaterialResource::inFlightDescriptorSets() noexcept {
	return _inFlightDescriptorSets;
}

const AfterglowMaterialResource::InFlightDescriptorSets& AfterglowMaterialResource::inFlightDescriptorSets() const noexcept {
	return _inFlightDescriptorSets;
}

AfterglowStorageBuffer* AfterglowMaterialResource::indexInputStorageBuffer() noexcept {
	return currentFrameStorageBuffer(_specifiedSSBOResources->inFlightIndexInputs);
}

const AfterglowStorageBuffer* AfterglowMaterialResource::indexInputStorageBuffer() const noexcept {
	return const_cast<AfterglowMaterialResource*>(this)->indexInputStorageBuffer();
}

AfterglowStorageBuffer* AfterglowMaterialResource::vertexInputStorageBuffer() noexcept {
	return currentFrameStorageBuffer(_specifiedSSBOResources->inFlightVertexInputs);
}

const AfterglowStorageBuffer* AfterglowMaterialResource::vertexInputStorageBuffer() const noexcept {
	return const_cast<AfterglowMaterialResource*>(this)->vertexInputStorageBuffer();
}

AfterglowStorageBuffer* AfterglowMaterialResource::indirectStorageBuffer() noexcept {
	return currentFrameStorageBuffer(_specifiedSSBOResources->inFlightIndirects);
}

const AfterglowStorageBuffer* AfterglowMaterialResource::indirectStorageBuffer() const noexcept {
	return const_cast<AfterglowMaterialResource*>(this)->indirectStorageBuffer();
}

//void AfterglowMaterialResource::update(uint32_t frameIndex) {
//	updateUniforms(frameIndex);
//	updateTextures(frameIndex);
//}

void AfterglowMaterialResource::reloadMaterialLayout() {
	_shouldReregisterTextures = true;
	// Update material parent and inheritance material instance modification.
	//DEBUG_COST_BEGIN("Redirect material instance");
	_materialInstance = _materialInstance.makeRedirectedInstance(_materialLayout.material());
	//DEBUG_COST_END;

	// Recreate descriptor sets
	for (int index = 0; index < cfg::maxFrameInFlight; ++index) {
		// Recreate descriptor sets
		_inFlightDescriptorSets[index].recreate(
			_descriptorPool,
			AfterglowDescriptorSets::RawLayoutArray{
				// 0 is global descriptor set.
				_materialLayout.rawDescriptorSetLayouts().begin() + shader::materialSetIndexBegin, _materialLayout.rawDescriptorSetLayouts().end()
			},
			// Before the material set index begin is shared set independent with material. 
			// e.g. global and per object.
			shader::materialSetIndexBegin
		);

		// Clear uniform buffers
		for (auto& [stage, resource]:_stageResources) {
			resource.uniformBuffers[index].reset();
		}
	}

	// Storage buffers relative to Material, instead of MaterialInstance.
	if (_materialLayout.material().hasComputeTask()) {
		submitStorageBuffers();
	}
}

AfterglowMaterialResource::StorageBufferResource* AfterglowMaterialResource::findStorageBufferResource(const std::string& ssboName, uint32_t frameIndex, uint32_t ssboIndex) {
	for (uint32_t setIndex = shader::computeSetIndexBegin; setIndex < shader::computeSetIndexEnd; ++setIndex) {
		auto& resources = _stageResources[static_cast<shader::Stage>(setIndex)].storageBufferResources;
		auto iterator = resources.find(ssboName);
		if (iterator == resources.end()) {
			continue;
		}
		return &iterator->second[frameStorageResourceIndex(frameIndex, ssboIndex, iterator->second.size() > 1)];
	}
	return nullptr;
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

void AfterglowMaterialResource::updateUniforms(uint32_t frameIndex) {
	// Clear old uniforms
	for (auto& [stage, resources] : _stageResources) {
		resources.uniforms.clear();
	}

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
		auto& uniformBuffer = resources.uniformBuffers[frameIndex];
		if (!uniformBuffer) {
			uniformBuffer.recreate(device(), uniforms.data(), uniforms.size() * sizeof(AfterglowMaterial::Scalar));
		}
		else {
			(*uniformBuffer).updateMemory(uniforms.data());
		}
	}
}

void AfterglowMaterialResource::updateTextures(uint32_t frameIndex) {
	// Load and submit Textures.
	synchronizeTextures();
	if (_shouldReregisterTextures) {
		reregisterUnmodifiedTextures();
		_shouldReregisterTextures = false;
	}
	reloadModifiedTextures(frameIndex);
}

void AfterglowMaterialResource::submitDescriptorSets(uint32_t frameIndex) {
	//DEBUG_COST_BEGIN("Submit descriptor sets.");
	// Write DescriptorSets
	auto& descriptorSets = _inFlightDescriptorSets[frameIndex];
	// 0 is global descriptor set.
	for (auto& [stage, resource] : _stageResources) {
		if (!resource.uniforms.empty()) {
			auto& uniformBuffer = resource.uniformBuffers[frameIndex];
			// First binding (index = 0) is always uniform, after then all texture bindings behind it.
			auto& setLayout = _materialLayout.descriptorSetLayouts()[stage];
			_descriptorSetWriter.registerBuffer(*uniformBuffer, setLayout, descriptorSets[util::EnumValue(stage)], 0);
		}

		for (auto& [textureName, textureResource] : resource.textureResources) {
			auto& setLayout = (*_materialLayout.descriptorSetLayouts()[stage]);
			_descriptorSetWriter.registerImage(
				textureResource.textureRef->texture(),
				setLayout,
				*(*_inFlightDescriptorSets[frameIndex]).find(setLayout),  // Ugly find, try to remove it.
				textureResource.bindingIndex
			);
		}
	}
	//DEBUG_COST_END;
}

inline void AfterglowMaterialResource::submitStorageBuffers() {
	// Reset usage refs.
	_specifiedSSBOResources = std::make_unique<SpecifiedSSBOResources>();

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

inline void AfterglowMaterialResource::reloadModifiedTextures(uint32_t frameIndex) {
	auto& textures = _materialInstance.textures();
	for (auto& [stage, textureParams] : textures) {
		for (auto& textureParam : textureParams) {
			const std::string* texturePath = &textureParam.value.path;
			// Handle no texture condition.
			if (*texturePath == "") {
				DEBUG_CLASS_WARNING("Material texture path is empty, texture name: " + textureParam.name);
				const std::string* texturePath = &img::defaultTextureInfo.path;
				textureParam.modified = true;
			}
			if (textureParam.value.colorSpace == img::ColorSpace::Undefined) {
				textureParam.value.colorSpace = img::ColorSpace::SRGB;
				DEBUG_CLASS_WARNING("Material texture color space is redirected to SRGB, due to it is undefined, texture name:" + textureParam.name);
			}

			auto& textureResource = *aquireTextureResource(stage, textureParam.name);
			if (textureParam.modified) {
				std::fill(textureResource.inFlightModifiedFlags.begin(), textureResource.inFlightModifiedFlags.end(), true);
				textureParam.modified = false;
			}

			if (!textureResource.inFlightModifiedFlags[frameIndex]) {
				continue;
			}

			textureResource.textureRef = std::make_unique<AfterglowTextureReference>(
				_texturePool.texture({ textureParam.value.colorSpace , *texturePath })
			);

			textureResource.inFlightModifiedFlags[frameIndex] = false;
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
			return !computeTask.findSSBOInfo(item.first);
		});
	};

	// Init Buffers
	const auto& computeTask = _materialLayout.material().computeTask();
	const auto& ssboInfos = computeTask.ssboInfos();
	std::unordered_map<shader::Stage, uint32_t> bindingIndices;
	for (const auto& ssboInfo : ssboInfos) {
		auto& ssboResources = _stageResources[ssboInfo.stage()].storageBufferResources;

		// Binding index: Minimum index == 1 because index 0 is uniform.
		if (bindingIndices.find(ssboInfo.stage()) == bindingIndices.end()) {
			bindingIndices.emplace(ssboInfo.stage(), _stageResources[ssboInfo.stage()].textureResources.size() + 1);
		}
		uint32_t& bindingIndex = bindingIndices.at(ssboInfo.stage());

		auto& frameSSBOResources = ssboResources[ssboInfo.name()];
		uint32_t numFrameSSBOs = computeTask.numSSBOs(ssboInfo);

		// Recreate buffers Whatever these buffers are exist or not, due to material layout may change the buffer size and layout.
		frameSSBOResources.resize(numFrameSSBOs);
		// TODO: Badsize for texture
		AfterglowSSBOInitializer initializer{ ssboInfo };
		AfterglowStagingBuffer stagingBuffer(device(), initializer.data(), initializer.byteSize());
		for (auto& ssboResource : frameSSBOResources) {
			if (ssboInfo.isBuffer()) {
				ssboResource.buffer.recreate(device(), initializer.data(), initializer.byteSize(), ssboInfo.usage());
				(*ssboResource.buffer).submit(_texturePool.commandPool(), _texturePool.graphicsQueue(), stagingBuffer);
			}
			else {
				ssboResource.image.recreate(
					device(), 
					computeTextureExtent(ssboInfo), 
					computeTextureFormat(ssboInfo), 
					ssboInfo.textureDimension(), 
					ssboInfo.textureSampleMode()
				);
				(*ssboResource.image).submit(_texturePool.commandPool(), _texturePool.graphicsQueue(), stagingBuffer);
			}
		}

		for (uint32_t index = 0; index < numFrameSSBOs; ++index) {
			frameSSBOResources[index].bindingIndex = bindingIndex;
			++bindingIndex;
		}

		// Usage refs for fast index.
		if (ssboInfo.usage() == compute::SSBOUsage::IndexInput) {
			_specifiedSSBOResources->inFlightIndexInputs = &frameSSBOResources;
		}
		else if (ssboInfo.usage() == compute::SSBOUsage::VertexInput) {
			_specifiedSSBOResources->inFlightVertexInputs = &frameSSBOResources;
		}
		else if (ssboInfo.usage() == compute::SSBOUsage::Indirect) {
			_specifiedSSBOResources->inFlightIndirects = &frameSSBOResources;
		}
	}
}

inline void AfterglowMaterialResource::registerFrameStorageBuffers(
	uint32_t frameIndex,
	StorageBufferResources& ssboResources,
	AfterglowDescriptorSetLayout& setLayout,
	VkDescriptorSet& set) {
	for (auto& [name, frameSSBOResources] : ssboResources) {
		for (uint32_t ssboIndex = 0; ssboIndex < frameSSBOResources.size(); ++ssboIndex) {
			uint32_t bufferIndex = frameStorageResourceIndex(frameIndex, ssboIndex, frameSSBOResources.size() > 1);
			// If ssbo is buffer, write buffer.
			if (frameSSBOResources[bufferIndex].buffer) {
				_descriptorSetWriter.registerBuffer(
					*frameSSBOResources[bufferIndex].buffer, setLayout, set, frameSSBOResources[ssboIndex].bindingIndex
				);
			}
			// If ssbo is image, write image.
			else if (frameSSBOResources[bufferIndex].image) {
				_descriptorSetWriter.registerImage(
					*frameSSBOResources[bufferIndex].image, setLayout, set, frameSSBOResources[ssboIndex].bindingIndex, VK_IMAGE_LAYOUT_GENERAL
				);
			}
		}
	}
}

inline AfterglowStorageBuffer* AfterglowMaterialResource::currentFrameStorageBuffer(FrameStorageBufferResources* inFlightResources) noexcept {
	if (!inFlightResources) {
		return nullptr;
	}
	// Limit maximum index for read only buffer.
	uint32_t index = std::min(
		static_cast<uint32_t>(inFlightResources->size()) - 1, device().currentFrameIndex()
	);
	return (*inFlightResources)[index].buffer;
}

inline uint32_t AfterglowMaterialResource::frameStorageResourceIndex(uint32_t frameIndex, uint32_t ssboIndex, bool multipleSSBOs) noexcept {
	// Read only single SSBO support.
	if (!multipleSSBOs) {
		return 0;
	}
	else {
		return (cfg::maxFrameInFlight + frameIndex - ssboIndex - 1) % cfg::maxFrameInFlight;
	}
}

VkFormat AfterglowMaterialResource::computeTextureFormat(const AfterglowSSBOInfo& ssboInfo) noexcept {
	switch (ssboInfo.textureMode()) {
	case compute::SSBOTextureMode::RGBA8Uint:
		return VK_FORMAT_R8G8B8A8_UINT;
	case compute::SSBOTextureMode::RGBA8Unorm:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case compute::SSBOTextureMode::RGBA8Snorm:
		return VK_FORMAT_R8G8B8A8_SNORM;
	case compute::SSBOTextureMode::RG16Float:
		return VK_FORMAT_R16G16_SFLOAT;
	case compute::SSBOTextureMode::RGBA16Float:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case compute::SSBOTextureMode::R32Float:
		return VK_FORMAT_R32_SFLOAT;
	case compute::SSBOTextureMode::RG32Float:
		return VK_FORMAT_R32G32_SFLOAT;
	case compute::SSBOTextureMode::RGB32Float:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case compute::SSBOTextureMode::RGBA32Float:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		DEBUG_ERROR("[AfterglowMaterialResource::computeTextureFormat] Invalid texture mode.");
		return VK_FORMAT_UNDEFINED;
	}
}

VkExtent3D AfterglowMaterialResource::computeTextureExtent(const AfterglowSSBOInfo& ssboInfo) noexcept {
	if (ssboInfo.isTexture1D()) {
		return VkExtent3D(static_cast<uint32_t>(ssboInfo.numElements()), 1, 1);
	}
	else if (ssboInfo.isTexture2D()) {
		uint32_t side = static_cast<uint32_t>(ceil(std::sqrt(ssboInfo.numElements())));
		return VkExtent3D(side, side, 1);
	}
	else if (ssboInfo.isTexture3D()) {
		uint32_t side = static_cast<uint32_t>(ceil(std::cbrt(ssboInfo.numElements())));
		return VkExtent3D(side, side, side);
	}
	DEBUG_ERROR("[AfterglowMaterialResource::computeTextureExtent] Invalid texture mode.");
	return VkExtent3D{};
}
