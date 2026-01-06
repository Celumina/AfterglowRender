#include "AfterglowSSBOInfo.h"

#include <mutex>
#include <cmath>
#include "AfterglowUtilities.h"
#include "DebugUtilities.h"

AfterglowSSBOInfo::AfterglowSSBOInfo() {
}

AfterglowSSBOInfo::AfterglowSSBOInfo(
	const std::string& name, 
	shader::Stage stage, 
	compute::SSBOUsage usage, 
	compute::SSBOAccessMode accessMode, 
	compute::SSBOTextureMode textureMode, 
	compute::SSBOTextureDimension textureDimension,
	compute::SSBOTextureSampleMode textureSampleMode, 
	compute::SSBOInitMode initMode, 
	const std::string& initResource, 
	const AfterglowStructLayout& elementLayout, 
	uint64_t numElements) : 
	_name(name), 
	_stage(stage), 
	_usage(usage), 
	_accessMode(accessMode), 
	_textureMode(textureMode), 
	_textureDimension(textureDimension),
	_textureSampleMode(textureSampleMode), 
	_initMode(initMode), 
	_initResource(initResource), 
	_elementLayout(elementLayout), 
	_numElements(numElements) {
	alignNumElements();
}

const std::string& AfterglowSSBOInfo::name() const noexcept {
	return _name;
}

void AfterglowSSBOInfo::rename(const std::string& name) {
	_name = name;
}

shader::Stage AfterglowSSBOInfo::stage() const noexcept {
	return _stage;
}

void AfterglowSSBOInfo::setStage(shader::Stage stage) noexcept {
	_stage = stage;
}

compute::SSBOUsage AfterglowSSBOInfo::usage() const noexcept {
	return _usage;
}

void AfterglowSSBOInfo::setUsage(compute::SSBOUsage usage) noexcept {
	_usage = usage;
}

compute::SSBOAccessMode AfterglowSSBOInfo::accessMode() const noexcept {
	return _accessMode;
}

void AfterglowSSBOInfo::setAccessMode(compute::SSBOAccessMode accessMode) noexcept {
	_accessMode = accessMode;
}

compute::SSBOTextureMode AfterglowSSBOInfo::textureMode() const noexcept {
	return _textureMode;
}

void AfterglowSSBOInfo::setTextureMode(compute::SSBOTextureMode textureMode) noexcept {
	_textureMode = textureMode;
}

compute::SSBOTextureDimension AfterglowSSBOInfo::textureDimension() const noexcept {
	return _textureDimension;
}

void AfterglowSSBOInfo::setTextureDimension(compute::SSBOTextureDimension textureDimension) noexcept {
	_textureDimension = textureDimension;
}

compute::SSBOTextureSampleMode AfterglowSSBOInfo::textureSampleMode() const noexcept {
	return _textureSampleMode;
}

void AfterglowSSBOInfo::setTextureSampleMode(compute::SSBOTextureSampleMode sampleMode) noexcept {
	_textureSampleMode = sampleMode;
}

compute::SSBOInitMode AfterglowSSBOInfo::initMode() const noexcept {
	return _initMode;
}

void AfterglowSSBOInfo::setInitMode(compute::SSBOInitMode initMode) noexcept {
	_initMode = initMode;
}

const std::string& AfterglowSSBOInfo::initResource() const noexcept {
	return _initResource;
}

void AfterglowSSBOInfo::setInitResource(const std::string& resourceName) {
	_initResource = resourceName;
}

AfterglowStructLayout& AfterglowSSBOInfo::elementLayout() noexcept {
	static std::array<AfterglowStructLayout, util::EnumValue(compute::SSBOTextureMode::EnumCount)> textureElementLayouts;
	static std::once_flag textureLayoutInitFlag;
	std::call_once(textureLayoutInitFlag, [](){
		Inreflect<compute::SSBOTextureMode>::forEachAttribute([](auto enumInfo){
			textureElementLayouts[enumInfo.value].addAttribute(
				AfterglowStructLayout::AttributeType::Float, "__texturePixel__"
			);
		});
	});

	if (_textureMode == compute::SSBOTextureMode::Unused) {
		return _elementLayout;
	}
	else {
		return textureElementLayouts[util::EnumValue(_textureMode)];
	}
}

const AfterglowStructLayout& AfterglowSSBOInfo::elementLayout() const noexcept {
	return const_cast<AfterglowSSBOInfo*>(this)->elementLayout();
}

void AfterglowSSBOInfo::setElementLayout(AfterglowStructLayout&& elementLayout) noexcept {
	_elementLayout = std::move(elementLayout);
}

uint64_t AfterglowSSBOInfo::numElements() const noexcept {
	return _numElements;
}

void AfterglowSSBOInfo::setNumElements(uint64_t numElements) noexcept {
	_numElements = numElements;
	alignNumElements();
}

bool AfterglowSSBOInfo::isBuffer() const noexcept {
	return _textureMode == compute::SSBOTextureMode::Unused;
}

bool AfterglowSSBOInfo::isTexture1D() const noexcept {
	return !isBuffer() && _textureDimension == compute::SSBOTextureDimension::Texture1D;
}

bool AfterglowSSBOInfo::isTexture2D() const noexcept {
	return !isBuffer() && _textureDimension == compute::SSBOTextureDimension::Texture2D;
}

bool AfterglowSSBOInfo::isTexture3D() const noexcept {
	return !isBuffer() && _textureDimension == compute::SSBOTextureDimension::Texture3D;
}

const std::string& AfterglowSSBOInfo::hlslTemplateName() const {
	static std::string sturctTypeName{ "StructuredBuffer" };
	static std::string texture1DTypeName{ "Texture1D" };
	static std::string texture2DTypeName{ "Texture2D" };
	static std::string texture3DTypeName{ "Texture3D" };
	if (isBuffer()) {
		return sturctTypeName;
	}

	switch (_textureDimension) {
	case compute::SSBOTextureDimension::Texture1D:
		return texture1DTypeName;
	case compute::SSBOTextureDimension::Texture2D:
		return texture2DTypeName;
	case compute::SSBOTextureDimension::Texture3D:
		return texture3DTypeName;
	}

	DEBUG_CLASS_FATAL("Unsupported hlsl template name.");
	throw std::runtime_error("Unsupported hlsl template name.");
}

uint64_t AfterglowSSBOInfo::numUnpackedIndices() const noexcept {
	return _numElements * 4;
}

void AfterglowSSBOInfo::alignNumElements() noexcept {
	// Only align texture yet. 
	// numElements follows: n^2 for 2D texture, n^3 for 3D texture.
	if (isTexture2D()) {
		uint64_t side = static_cast<uint64_t>(std::ceil(std::sqrt(_numElements)));
		_numElements = side * side;
	}
	else if (isTexture3D()) {
		uint64_t side = static_cast<uint64_t>(std::ceil(std::cbrt(_numElements)));
		_numElements = side * side * side;
	}
}
