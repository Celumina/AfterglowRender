#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

#include "AfterglowComputeTask.h"
#include "AssetDefinitions.h"
#include "RenderDefinitions.h"
#include "AfterglowUtilities.h"
#include "DebugUtilities.h"

// TODO: custom depth write
// TODO: Transparency domain.

class AfterglowMaterial {
public:
	using Scalar = float;
	using Vector = glm::vec4;
	using TextureInfo = img::AssetInfo;

	template<typename Type>
	struct Parameter {
		std::string name;
		Type value;
		bool modified;	// Initialize always be true, set to false if had been submitted to renderer.
	};

	template<typename Type>
	using Parameters = std::unordered_map<shader::Stage, std::vector<Parameter<Type>>>;

	// Constructor (exclude copy constructor) will never init compute task, compute task be initialized only if it been used.
	AfterglowMaterial();
	AfterglowMaterial(
		render::Domain domain,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath,
		const Parameters<Scalar>& scalars,
		const Parameters<Vector>& vectors,
		const Parameters<TextureInfo>& textures
	);

	AfterglowMaterial(const AfterglowMaterial& other);

	// @brief: Address based compare.
	bool is(const AfterglowMaterial& other) const;

	static constexpr uint32_t elementAlignment();
	static constexpr uint32_t vectorLength();

	static const AfterglowMaterial& emptyMaterial();
	static const AfterglowMaterial& defaultMaterial();
	static const AfterglowMaterial& errorMaterial();

	void operator=(const AfterglowMaterial& other);

	void setVertexTypeIndex(std::type_index vertexTypeIndex);
	void setTwoSided(bool twoSide);
	void setTopology(render::Topology topology);

	void setVertexShader(const std::string& shaderPath);
	void setFragmentShader(const std::string& shaderPath);

	void setScalar(shader::Stage stage, const std::string& name, Scalar defaultValue);
	void setVector(shader::Stage stage, const std::string& name, Vector defaultValue);
	void setTexture(shader::Stage stage, const std::string& name, const TextureInfo& textureInfo);

	void setDomain(render::Domain domain);

	std::type_index vertexTypeIndex() const;
	bool twoSided() const;

	Parameter<Scalar>* scalar(shader::Stage stage, const std::string& name);
	Parameter<Vector>* vector(shader::Stage stage, const std::string& name);
	Parameter<TextureInfo>* texture(shader::Stage stage, const std::string& name);

	const Parameter<Scalar>* scalar(shader::Stage stage, const std::string& name) const;
	const Parameter<Vector>* vector(shader::Stage stage, const std::string& name) const;
	const Parameter<TextureInfo>* texture(shader::Stage stage, const std::string& name) const;

	Parameters<Scalar>& scalars();
	Parameters<Vector>& vectors();
	Parameters<TextureInfo>& textures();

	const Parameters<Scalar>& scalars() const;
	const Parameters<Vector>& vectors() const;
	const Parameters<TextureInfo>& textures() const;

	const std::string& vertexShaderPath() const;
	const std::string& fragmentShaderPath() const;

	render::Domain domain() const;
	render::Topology topology() const;

	// @brief: Padding element size for alignment before vector.
	uint32_t scalarPaddingSize(shader::Stage stage) const;

	bool hasComputeTask() const;

	// @brief: Only initialize once.
	AfterglowComputeTask& initComputeTask();

	AfterglowComputeTask& computeTask();
	const AfterglowComputeTask& computeTask() const;

private:
	template<typename Type>
	void setParameter(
		Parameters<Type>& container,
		shader::Stage stage,
		const Parameter<Type>& parameter
	);

	template<typename Type>
	Parameter<Type>* parameter(
		Parameters<Type>& container, 
		shader::Stage stage, 
		const std::string& name
	);

	render::Domain _domain = render::Domain::Forward;
	render::Topology _topology = render::Topology::TriangleList;
	bool _twoSided = false;

	std::type_index _vertexTypeIndex;

	std::string _vertexShaderPath;
	std::string _fragmentShaderPath;

	Parameters<Scalar> _scalars;
	Parameters<Vector> _vectors;
	Parameters<TextureInfo> _textures;

	std::unique_ptr<AfterglowComputeTask> _computeTask = nullptr;
};

constexpr uint32_t AfterglowMaterial::elementAlignment() {
	return Vector::length();
}

constexpr uint32_t AfterglowMaterial::vectorLength() {
	return Vector::length();
}

template<typename Type>
inline void AfterglowMaterial::setParameter(
	Parameters<Type>& container,
	shader::Stage stage, 
	const Parameter<Type>& parameter) {
	auto stageIterator = container.find(stage);
	std::vector<Parameter<Type>>* parameters = nullptr;
	if (stageIterator == container.end()) {
		parameters = &container.emplace(stage, std::vector<Parameter<Type>>{}).first->second;
	}
	else {
		parameters = &stageIterator->second;
	}

	for (auto& old : *parameters) {
		if (old.name == parameter.name) {
			old = parameter;
			return;
		}
	}
	// If not exist, append it.
	parameters->push_back(parameter);
}

template<typename Type>
inline AfterglowMaterial::Parameter<Type>* AfterglowMaterial::parameter(Parameters<Type>& container, shader::Stage stage, const std::string& name) {
	auto iterator = container.find(stage);
	if (iterator == container.end()) {
		return nullptr;
	}
	for (auto& parameter : iterator->second) {
		if (parameter.name == name) {
			return &parameter;
		}
	}
	return nullptr;
}