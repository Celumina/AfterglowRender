#pragma once
#include "AfterglowMaterial.h"

// TODO: Mark parameters changed in runtime for writeDescriptor update.

class AfterglowMaterialInstance : private AfterglowMaterial {
public:
	AfterglowMaterialInstance();
	AfterglowMaterialInstance(const AfterglowMaterial& parent);

	AfterglowMaterialInstance(const AfterglowMaterialInstance& other);
	void operator=(const AfterglowMaterialInstance& other);

	AfterglowMaterialInstance(AfterglowMaterialInstance&& rval) noexcept;
	void operator=(AfterglowMaterialInstance&& rval) noexcept;

	// @brief: Reset parent material and keep instance own modifications.
	AfterglowMaterialInstance makeRedirectedInstance(const AfterglowMaterial& newParent);

	// @return: true if set parameter successfully.
	bool setScalar(shader::Stage stage, const std::string& name, Scalar value);

	// @return: true if set parameter successfully.
	bool setVector(shader::Stage stage, const std::string& name, Vector value);

	// @return: true if set parameter successfully.
	bool setTexture(shader::Stage stage, const std::string& name, const TextureInfo& assetInfo);

	// TODO: Deprecate these non-const container functions. modify parameters from set functions.
	Parameter<Scalar>* scalar(shader::Stage stage, const std::string& name);
	Parameter<Vector>* vector(shader::Stage stage, const std::string& name);
	Parameter<TextureInfo>* texture(shader::Stage stage, const std::string& name);

	Parameters<Scalar>& scalars();
	Parameters<Vector>& vectors();
	Parameters<TextureInfo>& textures();

	const Parameters<Scalar>& scalars() const;
	const Parameters<Vector>& vectors() const;
	const Parameters<TextureInfo>& textures() const;


	const AfterglowMaterial& parentMaterial() const noexcept;

	// @brief: Restore all parameters from parent.
	void reset();

private:
	/**
	* @desc: 
	* 	If current material instance exists this param and has a non-default value, 
	* 	set the dstValue to the current material instance param value.
	*/
	template<auto ParamFunc, typename Type>
	bool tryPreserve(shader::Stage stage, const Parameter<Type>& newParam, const Type* dstValue);

	const AfterglowMaterial* _parent;
};

// typename: Type; auto: Anoymous type value
template<auto ParamFunc, typename Type>
inline bool AfterglowMaterialInstance::tryPreserve(shader::Stage stage, const Parameter<Type>& newParam, const Type* dstValue) {
	auto* oldParam = (this->*ParamFunc)(stage, newParam.name);
	if (oldParam) {
		dstValue = &oldParam->value;
		return true;
	}
	return false;
	// Useless, default param was updated  before. 
	//auto* oldDefaultParam = (_parent->*ParamFunc)(stage, newParam.name);
	//if (oldParam->value != oldDefaultParam->value) {
	//	dstValue = &oldParam->value;
	//}
}
