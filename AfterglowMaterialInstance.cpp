#include "AfterglowMaterialInstance.h"

AfterglowMaterialInstance::AfterglowMaterialInstance() : 
	AfterglowMaterial(AfterglowMaterial::defaultMaterial()), _parent(&AfterglowMaterial::defaultMaterial()) {
}

AfterglowMaterialInstance::AfterglowMaterialInstance(const AfterglowMaterial& parent) :
	AfterglowMaterial(parent), _parent(&parent) {
}

AfterglowMaterialInstance::AfterglowMaterialInstance(const AfterglowMaterialInstance& other) : 
	AfterglowMaterial(other), _parent(other._parent) {
}

AfterglowMaterialInstance AfterglowMaterialInstance::makeRedirectedInstance(const AfterglowMaterial& newParent) {
	AfterglowMaterialInstance instance(newParent);
	for (const auto& [stage, scalarParams] : scalars()) {
		for (const auto& scalarParam : scalarParams) {
			instance.setScalar(stage, scalarParam.name, scalarParam.value);
		}
	}
	for (const auto& [stage, vectorParams] : vectors()) {
		for (const auto& vectorParam : vectorParams) {
			instance.setVector(stage, vectorParam.name, vectorParam.value);
		}
		
	}
	for (const auto& [stage, textureParams] : textures()) {
		for (const auto& textureParam : textureParams) {
			instance.setTexture(stage, textureParam.name, textureParam.value);
		}
	}
	return instance;
}

bool AfterglowMaterialInstance::setScalar(shader::Stage stage, const std::string& name, Scalar value) {
	auto*oldScalar = scalar(stage, name);
	if (oldScalar) {
		AfterglowMaterial::setScalar(stage, name, value);
		return true;
	}
	return false;
}

bool AfterglowMaterialInstance::setVector(shader::Stage stage, const std::string& name, Vector value) {
	auto* oldVector = vector(stage, name);
	if (oldVector) {
		AfterglowMaterial::setVector(stage, name, value);
		return true;
	}
	return false;
}

bool AfterglowMaterialInstance::setTexture(shader::Stage stage, const std::string& name, const TextureInfo& assetInfo) {
	auto* oldTexture = texture(stage, name);
	if (!oldTexture) {
		return false;
	}
	auto targetFormat = 
		(assetInfo.format == img::Format::Undefined) ? oldTexture->value.format : assetInfo.format;
	auto targetColorSpace = 
		(assetInfo.colorSpace == img::ColorSpace::Undefined) ? oldTexture->value.colorSpace : assetInfo.colorSpace;

	AfterglowMaterial::setTexture(stage, name, {targetFormat, targetColorSpace, assetInfo.path});
	return true;
}

AfterglowMaterialInstance::Parameter<AfterglowMaterialInstance::Scalar>* AfterglowMaterialInstance::scalar(shader::Stage stage, const std::string& name) {
	return AfterglowMaterial::scalar(stage, name);
}

AfterglowMaterialInstance::Parameter<AfterglowMaterialInstance::Vector>* AfterglowMaterialInstance::vector(shader::Stage stage, const std::string& name) {
	return AfterglowMaterial::vector(stage, name);
}

AfterglowMaterialInstance::Parameter<AfterglowMaterialInstance::TextureInfo>* AfterglowMaterialInstance::texture(shader::Stage stage, const std::string& name) {
	return AfterglowMaterial::texture(stage, name);
}

AfterglowMaterialInstance::Parameters<AfterglowMaterialInstance::Scalar>& AfterglowMaterialInstance::scalars() {
	return AfterglowMaterial::scalars();
}

AfterglowMaterialInstance::Parameters<AfterglowMaterialInstance::Vector>& AfterglowMaterialInstance::vectors() {
	return AfterglowMaterial::vectors();
}

AfterglowMaterialInstance::Parameters<AfterglowMaterialInstance::TextureInfo>& AfterglowMaterialInstance::textures() {
	return AfterglowMaterial::textures();
}

const AfterglowMaterialInstance::Parameters<AfterglowMaterialInstance::Scalar>& AfterglowMaterialInstance::scalars() const {
	return AfterglowMaterial::scalars();
}

const AfterglowMaterialInstance::Parameters<AfterglowMaterialInstance::Vector>& AfterglowMaterialInstance::vectors() const {
	return AfterglowMaterial::vectors();
}

const AfterglowMaterialInstance::Parameters<AfterglowMaterialInstance::TextureInfo>& AfterglowMaterialInstance::textures() const {
	return AfterglowMaterial::textures();
}

void AfterglowMaterialInstance::operator=(const AfterglowMaterialInstance& other) {
	AfterglowMaterial::operator=(other);
	_parent = other._parent;
}

const AfterglowMaterial& AfterglowMaterialInstance::parentMaterial() const {
	return *_parent;
}

void AfterglowMaterialInstance::reset() {
	this->operator=(*_parent);
}
