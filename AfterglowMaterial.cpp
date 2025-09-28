#include "AfterglowMaterial.h"
#include "VertexStructs.h"

AfterglowMaterial::AfterglowMaterial() : 
	_vertexTypeIndex(util::TypeIndex<vert::StandardVertex>()) {
}

AfterglowMaterial::AfterglowMaterial(
	render::Domain domain,
	const std::string& vertexShaderPath,
	const std::string& fragmentShaderPath,
	const Parameters<Scalar>& scalars,
	const Parameters<Vector>& vectors,
	const Parameters<TextureInfo>& textures) :
	_domain(domain), 
	_vertexTypeIndex(util::TypeIndex<vert::StandardVertex>()),
	_vertexShaderPath(vertexShaderPath), 
	_fragmentShaderPath(fragmentShaderPath), 
	_scalars(scalars),
	_vectors(vectors),
	_textures(textures) {
}

AfterglowMaterial::AfterglowMaterial(const AfterglowMaterial& other) :
	_domain(other._domain),
	_topology(other._topology), 
	_twoSided(other._twoSided),
	_vertexTypeIndex(other._vertexTypeIndex),
	_vertexShaderPath(other._vertexShaderPath),
	_fragmentShaderPath(other._fragmentShaderPath),
	_scalars(other._scalars),
	_vectors(other._vectors),
	_textures(other._textures) {
	if (other._computeTask) {
		_computeTask = std::make_unique<AfterglowComputeTask>(*other._computeTask);
	}
}

const AfterglowMaterial& AfterglowMaterial::emptyMaterial() {
	static AfterglowMaterial material;
	return material;
}

const AfterglowMaterial& AfterglowMaterial::defaultMaterial() {
	static AfterglowMaterial material{
		render::Domain::Forward,
		"Shaders/Unlit_VS.hlsl", 
		"Shaders/Unlit_FS.hlsl", 
		// scalars
		{}, 
		// vectors
		{}, 
		// textures
		{{shader::Stage::Fragment, {Parameter<TextureInfo>{"albedoTex", {img::Format::RGBA, img::ColorSpace::SRGB, "Assets/Shared/Textures/White.png"}, true}}}}
	};
	return material;
}

const AfterglowMaterial& AfterglowMaterial::errorMaterial() {
	static AfterglowMaterial material{
		render::Domain::Forward,
		"Shaders/Error_VS.hlsl",
		"Shaders/Error_FS.hlsl",
		// scalars
		{},
		// vectors
		{},
		// textures
		{}
	};
	static std::once_flag onceFlag;
	std::call_once(onceFlag, []() {
		auto& computeTask = material.initComputeTask();
		computeTask.setComputeShader("Shaders/Error_CS.hlsl");
	});
	return material;
}

void AfterglowMaterial::operator=(const AfterglowMaterial& other) {
	_domain = other._domain;
	_topology = other._topology;
	_twoSided = other._twoSided;
	_vertexTypeIndex = other._vertexTypeIndex;
	_vertexShaderPath = other._vertexShaderPath;
	_fragmentShaderPath = other._fragmentShaderPath;
	_scalars = other._scalars;
	_vectors = other._vectors;
	_textures = other._textures;
	if (other._computeTask) {
		_computeTask = std::make_unique<AfterglowComputeTask>(*other._computeTask);
	}
}

bool AfterglowMaterial::is(const AfterglowMaterial& other) const {
	return this == &other;
}

void AfterglowMaterial::setVertexTypeIndex(std::type_index vertexTypeIndex) {
	_vertexTypeIndex = vertexTypeIndex;
}

void AfterglowMaterial::setTwoSided(bool twoSided) {
	_twoSided = twoSided;
}

void AfterglowMaterial::setTopology(render::Topology topology) {
	_topology = topology;
}

void AfterglowMaterial::setVertexShader(const std::string& shaderPath) {
	_vertexShaderPath = shaderPath;
}

void AfterglowMaterial::setFragmentShader(const std::string& shaderPath) {
	_fragmentShaderPath = shaderPath;
}

void AfterglowMaterial::setScalar(shader::Stage stage, const std::string& name, Scalar defaultValue) {
	setParameter<Scalar>(_scalars, stage, Parameter<Scalar>{ name, defaultValue, true });
}

void AfterglowMaterial::setVector(shader::Stage stage, const std::string& name, Vector defaultValue) {
	setParameter<Vector>(_vectors, stage, Parameter<Vector>{ name, defaultValue, true });
}

void AfterglowMaterial::setTexture(shader::Stage stage, const std::string& name, const TextureInfo& textureInfo) {
	setParameter<TextureInfo>(
		_textures, 
		stage, 
		Parameter<TextureInfo>{ name, textureInfo, true }
	);
}

void AfterglowMaterial::setDomain(render::Domain domain) {
	_domain = domain;
}

std::type_index AfterglowMaterial::vertexTypeIndex() const {
	return _vertexTypeIndex;
}

bool AfterglowMaterial::twoSided() const {
	return _twoSided;
}

AfterglowMaterial::Parameter<AfterglowMaterial::Scalar>* AfterglowMaterial::scalar(shader::Stage stage, const std::string& name) {
	return parameter<Scalar>(_scalars, stage, name);
}

AfterglowMaterial::Parameter<AfterglowMaterial::Vector>* AfterglowMaterial::vector(shader::Stage stage, const std::string& name) {
	return parameter<Vector>(_vectors, stage, name);
}

AfterglowMaterial::Parameter<AfterglowMaterial::TextureInfo>* AfterglowMaterial::texture(shader::Stage stage, const std::string& name) {
	return parameter<TextureInfo>(_textures, stage, name);
}

const AfterglowMaterial::Parameter<AfterglowMaterial::Scalar>* AfterglowMaterial::scalar(shader::Stage stage, const std::string& name) const {
	return const_cast<AfterglowMaterial*>(this)->scalar(stage, name);
}


const AfterglowMaterial::Parameter<AfterglowMaterial::Vector>* AfterglowMaterial::vector(shader::Stage stage, const std::string& name) const {
	return const_cast<AfterglowMaterial*>(this)->vector(stage, name);
}


const AfterglowMaterial::Parameter<AfterglowMaterial::TextureInfo>* AfterglowMaterial::texture(shader::Stage stage, const std::string& name) const {
	return const_cast<AfterglowMaterial*>(this)->texture(stage, name);
}

AfterglowMaterial::Parameters<AfterglowMaterial::Scalar>& AfterglowMaterial::scalars() {
	return _scalars;
}

AfterglowMaterial::Parameters<AfterglowMaterial::Vector>& AfterglowMaterial::vectors() {
	return _vectors;
}

AfterglowMaterial::Parameters<AfterglowMaterial::TextureInfo>& AfterglowMaterial::textures() {
	return _textures;
}

const AfterglowMaterial::Parameters<AfterglowMaterial::Scalar>& AfterglowMaterial::scalars() const {
	return _scalars;
}

const AfterglowMaterial::Parameters<AfterglowMaterial::Vector>& AfterglowMaterial::vectors() const {
	return _vectors;
}

const AfterglowMaterial::Parameters<AfterglowMaterial::TextureInfo>& AfterglowMaterial::textures() const {
	return _textures;
}

const std::string& AfterglowMaterial::vertexShaderPath() const {
	return _vertexShaderPath;
}

const std::string& AfterglowMaterial::fragmentShaderPath() const {
	return _fragmentShaderPath;
}

render::Domain AfterglowMaterial::domain() const {
	return _domain;
}

render::Topology AfterglowMaterial::topology() const {
	return _topology;
}

uint32_t AfterglowMaterial::scalarPaddingSize(shader::Stage stage) const {
	auto iterator = _scalars.find(stage);
	if (iterator == _scalars.end()) {
		return 0;
	}
	uint32_t numScalars = iterator->second.size();
	return util::Align(numScalars, elementAlignment()) - numScalars;
}

bool AfterglowMaterial::hasComputeTask() const {
	return _computeTask.get();
}

AfterglowComputeTask& AfterglowMaterial::initComputeTask() {
	if (!_computeTask) {
		_computeTask = std::make_unique<AfterglowComputeTask>();
		DEBUG_CLASS_INFO("Compute task is created.");
	}
	return *_computeTask;
}

AfterglowComputeTask& AfterglowMaterial::computeTask() {
	if (!_computeTask) {
		throw std::runtime_error("ComputeTask has not been initialized, make sure initComputeTask() first.");
	}
	return *_computeTask;
}

const AfterglowComputeTask& AfterglowMaterial::computeTask() const {
	return const_cast<AfterglowMaterial*>(this)->computeTask();
}
