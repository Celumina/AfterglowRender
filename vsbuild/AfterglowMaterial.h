#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include <glm/glm.hpp>
#include "AssetDefinitions.h"
#include "RenderDefinitions.h"
#include "ShaderDefinitions.h"

class AfterglowComputeTask;

// TODO: Wrap bools into feature flag bits.

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

	template<typename ParamType>
	using GetParamFunc = Parameter<ParamType>* (AfterglowMaterial::*)(shader::Stage, const std::string&);

	template<typename ParamType>
	using GetParamFuncConst = const Parameter<ParamType>* (AfterglowMaterial::*)(shader::Stage, const std::string&) const;

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
	void operator=(const AfterglowMaterial& other);

	AfterglowMaterial(AfterglowMaterial&& rval) noexcept;
	void operator=(AfterglowMaterial&& rval) noexcept;

	~AfterglowMaterial();

	// @brief: Address based compare.
	bool is(const AfterglowMaterial& other) const noexcept;

	static constexpr uint32_t elementAlignment();
	static constexpr uint32_t vectorLength();

	static const AfterglowMaterial& emptyMaterial();
	static const AfterglowMaterial& defaultMaterial();
	static const AfterglowMaterial& errorMaterial();
	static const AfterglowMaterial& emptyPostProcessMaterial();

	void setVertexTypeIndex(std::type_index vertexTypeIndex) noexcept;
	void setCullMode(render::CullMode cullMode) noexcept;
	void setWireframe(bool wireframe) noexcept;
	void setDepthWrite(bool depthWrite) noexcept;
	void setTopology(render::Topology topology) noexcept;

	void setVertexShader(const std::string& shaderPath);
	void setFragmentShader(const std::string& shaderPath);
	void setCustomPass(const std::string& passName);
	void setSubpass(const std::string& subpassName);

	void setScalar(shader::Stage stage, const std::string& name, Scalar defaultValue);
	void setVector(shader::Stage stage, const std::string& name, Vector defaultValue);
	void setTexture(shader::Stage stage, const std::string& name, const TextureInfo& textureInfo);

	render::Domain domain() const noexcept;
	void setDomain(render::Domain domain) noexcept;

	const render::FaceStencilInfos& faceStencilInfos() const noexcept;
	void setFaceStencilInfo(const render::FaceStencilInfos& faceStencilInfo) noexcept;

	std::type_index vertexTypeIndex() const noexcept;
	render::CullMode cullMode() const noexcept;
	bool wireframe() const noexcept;
	bool depthWrite() const noexcept;

	Parameter<Scalar>* scalar(shader::Stage stage, const std::string& name);
	Parameter<Vector>* vector(shader::Stage stage, const std::string& name);
	Parameter<TextureInfo>* texture(shader::Stage stage, const std::string& name);

	const Parameter<Scalar>* scalar(shader::Stage stage, const std::string& name) const;
	const Parameter<Vector>* vector(shader::Stage stage, const std::string& name) const;
	const Parameter<TextureInfo>* texture(shader::Stage stage, const std::string& name) const;

	Parameters<Scalar>& scalars() noexcept;
	Parameters<Vector>& vectors() noexcept;
	Parameters<TextureInfo>& textures() noexcept;

	const Parameters<Scalar>& scalars() const noexcept;
	const Parameters<Vector>& vectors() const noexcept;
	const Parameters<TextureInfo>& textures() const noexcept;

	const std::string& vertexShaderPath() const noexcept;
	const std::string& fragmentShaderPath() const noexcept;

	const std::string& customPassName() const noexcept;
	const std::string& subpassName() const noexcept;

	render::Topology topology() const noexcept;

	// @brief: Padding element size for alignment before vector.
	uint32_t scalarPaddingSize(shader::Stage stage) const noexcept;

	bool hasComputeTask() const noexcept;

	// @brief: Only initialize once.
	AfterglowComputeTask& initComputeTask();

	/**
	* @warning: 
	*	Could throw a exception when the compute task is not initialized. 
	*	If you can't make sure compute task is exists, invoke hasComputeTask() to verify that.
	*	Or invoke initComputeTask() for init (If it is not exists before) and get.
	*/ 
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
	render::CullMode _cullMode = render::CullMode::Back;

	bool _wireframe = false;
	bool _depthWrite = true;

	render::FaceStencilInfos _faceStencilInfos{};

	std::type_index _vertexTypeIndex;

	std::string _vertexShaderPath;
	std::string _fragmentShaderPath;
	std::string _customPassName;
	std::string _subpassName;

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