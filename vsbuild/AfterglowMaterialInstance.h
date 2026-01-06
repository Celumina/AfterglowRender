#pragma once
#include "AfterglowMaterial.h"

// TODO: Mark parameters changed in runtime for writeDescriptor update.

class AfterglowMaterialInstance : private AfterglowMaterial {
public:
	AfterglowMaterialInstance();
	AfterglowMaterialInstance(const AfterglowMaterial& parent);
	AfterglowMaterialInstance(const AfterglowMaterialInstance& other);

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

	void operator=(const AfterglowMaterialInstance& other);

	const AfterglowMaterial& parentMaterial() const noexcept;

	// @brief: Restore all parameters from parent.
	void reset();

private:
	const AfterglowMaterial* _parent;
};