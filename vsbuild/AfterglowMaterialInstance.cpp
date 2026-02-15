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

void AfterglowMaterialInstance::operator=(const AfterglowMaterialInstance& other) {
	AfterglowMaterial::operator=(other);
	_parent = other._parent;
}

AfterglowMaterialInstance::AfterglowMaterialInstance(AfterglowMaterialInstance&& rval) noexcept :
	AfterglowMaterial(std::forward<AfterglowMaterial>(rval)), _parent(rval._parent) {
}

void AfterglowMaterialInstance::operator=(AfterglowMaterialInstance&& rval) noexcept {
	AfterglowMaterial::operator=(std::forward<AfterglowMaterial>(rval));
	_parent = rval._parent;
}

AfterglowMaterialInstance AfterglowMaterialInstance::makeRedirectedInstance(const AfterglowMaterial& newParent) {
	AfterglowMaterialInstance instance(newParent);
	for (const auto& [stage, scalarParams] : scalars()) {
		for (const auto& scalarParam : scalarParams) {
			// Find old scalar for preserving instance configs.
			auto* value = &scalarParam.value;
			tryPreserve<static_cast<GetParamFuncConst<Scalar>>(&AfterglowMaterial::scalar)>(stage, scalarParam, value);
			instance.setScalar(stage, scalarParam.name, *value);
		}
	}
	for (const auto& [stage, vectorParams] : vectors()) {
		for (const auto& vectorParam : vectorParams) {
			auto* value = &vectorParam.value;
			tryPreserve<static_cast<GetParamFuncConst<Vector>>(&AfterglowMaterial::vector)>(stage, vectorParam, value);
			instance.setVector(stage, vectorParam.name, *value);
		}
	}
	for (const auto& [stage, textureParams] : textures()) {
		for (const auto& textureParam : textureParams) {
			auto* value = &textureParam.value;
			tryPreserve<static_cast<GetParamFuncConst<TextureInfo>>(&AfterglowMaterial::texture)>(stage, textureParam, value);
			instance.setTexture(stage, textureParam.name, *value);
		}
	}
	return instance;
}

bool AfterglowMaterialInstance::setScalar(shader::Stage stage, const std::string& name, Scalar value) {
	auto*oldScalar = scalar(stage, name);
	// @note: _parent->scalar(stage, name) handle parent material changed case.
	if (oldScalar || _parent->scalar(stage, name)) {
		AfterglowMaterial::setScalar(stage, name, value);
		return true;
	}
	return false;
}

bool AfterglowMaterialInstance::setVector(shader::Stage stage, const std::string& name, Vector value) {
	auto* oldVector = vector(stage, name);
	if (oldVector || _parent->vector(stage, name)) {
		AfterglowMaterial::setVector(stage, name, value);
		return true;
	}
	return false;
}

bool AfterglowMaterialInstance::setTexture(shader::Stage stage, const std::string& name, const TextureInfo& assetInfo) {
	auto* oldTexture = texture(stage, name);
	if (!oldTexture || !_parent->texture(stage, name)) {
		return false;
	}
	auto targetColorSpace = 
		(assetInfo.colorSpace == img::ColorSpace::Undefined) ? oldTexture->value.colorSpace : assetInfo.colorSpace;

	AfterglowMaterial::setTexture(stage, name, {targetColorSpace, assetInfo.path});
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

const AfterglowMaterial& AfterglowMaterialInstance::parentMaterial() const noexcept {
	return *_parent;
}

void AfterglowMaterialInstance::reset() {
	this->operator=(*_parent);
}
