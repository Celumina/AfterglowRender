#include "AfterglowMaterial.h"

#include <mutex>
#include <stdexcept>

#include "AfterglowComputeTask.h"
#include "AfterglowUtilities.h"
#include "GlobalAssets.h"
#include "DebugUtilities.h"
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
	_cullMode(other._cullMode),
	_wireframe(other._wireframe),
	_depthWrite(other._depthWrite),
	_faceStencilInfos(other._faceStencilInfos), 
	_vertexTypeIndex(other._vertexTypeIndex),
	_vertexShaderPath(other._vertexShaderPath),
	_fragmentShaderPath(other._fragmentShaderPath),
	_customPassName(other._customPassName), 
	_subpassName(other._subpassName), 
	_scalars(other._scalars),
	_vectors(other._vectors),
	_textures(other._textures) {
	if (other._computeTask) {
		_computeTask = std::make_unique<AfterglowComputeTask>(*other._computeTask);
	}
}

void AfterglowMaterial::operator=(const AfterglowMaterial& other) {
	_domain = other._domain;
	_topology = other._topology;
	_cullMode = other._cullMode;
	_wireframe = other._wireframe;
	_depthWrite = other._depthWrite;
	_faceStencilInfos = other._faceStencilInfos;
	_vertexTypeIndex = other._vertexTypeIndex;
	_vertexShaderPath = other._vertexShaderPath;
	_fragmentShaderPath = other._fragmentShaderPath;
	_customPassName = other._customPassName;
	_subpassName = other._subpassName;
	_scalars = other._scalars;
	_vectors = other._vectors;
	_textures = other._textures;
	if (other._computeTask) {
		_computeTask = std::make_unique<AfterglowComputeTask>(*other._computeTask);
	}
}

AfterglowMaterial::AfterglowMaterial(AfterglowMaterial&& rval) noexcept :
	_domain(std::move(rval._domain)),
	_topology(std::move(rval._topology)),
	_cullMode(std::move(rval._cullMode)),
	_wireframe(std::move(rval._wireframe)),
	_depthWrite(std::move(rval._depthWrite)),
	_faceStencilInfos(std::move(rval._faceStencilInfos)),
	_vertexTypeIndex(std::move(rval._vertexTypeIndex)),
	_vertexShaderPath(std::move(rval._vertexShaderPath)),
	_fragmentShaderPath(std::move(rval._fragmentShaderPath)),
	_customPassName(std::move(rval._customPassName)),
	_subpassName(std::move(rval._subpassName)),
	_scalars(std::move(rval._scalars)),
	_vectors(std::move(rval._vectors)),
	_textures(std::move(rval._textures)) {
	if (rval._computeTask) {
		_computeTask = std::move(rval._computeTask);
	}
}

void AfterglowMaterial::operator=(AfterglowMaterial&& rval) noexcept {
	_domain = std::move(rval._domain);
	_topology = std::move(rval._topology);
	_cullMode = std::move(rval._cullMode);
	_wireframe = std::move(rval._wireframe);
	_depthWrite = std::move(rval._depthWrite);
	_faceStencilInfos = std::move(rval._faceStencilInfos);
	_vertexTypeIndex = std::move(rval._vertexTypeIndex);
	_vertexShaderPath = std::move(rval._vertexShaderPath);
	_fragmentShaderPath = std::move(rval._fragmentShaderPath);
	_customPassName = std::move(rval._customPassName);
	_subpassName = std::move(rval._subpassName);
	_scalars = std::move(rval._scalars);
	_vectors = std::move(rval._vectors);
	_textures = std::move(rval._textures);
	if (rval._computeTask) {
		_computeTask = std::move(rval._computeTask);
	}
}

AfterglowMaterial::~AfterglowMaterial() {
}

const AfterglowMaterial& AfterglowMaterial::emptyMaterial() {
	static AfterglowMaterial material;
	return material;
}

const AfterglowMaterial& AfterglowMaterial::defaultMaterial() {
	static AfterglowMaterial material{
		render::Domain::Forward,
		shader::defaultForwardVSPath, 
		shader::defaultForwardFSPath,
		// scalars
		{}, 
		// vectors
		{}, 
		// textures
		{{shader::Stage::Fragment, {Parameter<TextureInfo>{"albedoTex", img::defaultTextureInfo, true}}}}
	};
	return material;
}

const AfterglowMaterial& AfterglowMaterial::errorMaterial() {
	static AfterglowMaterial material{
		render::Domain::Forward,
		shader::errorVSPath,
		shader::errorFSPath,
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
		computeTask.setComputeShader(shader::errorCSPath);
	});
	return material;
}

const AfterglowMaterial& AfterglowMaterial::emptyPostProcessMaterial() {
	static AfterglowMaterial material {
		render::Domain::PostProcess,
		shader::emptyPostprocessVSPath,
		shader::emptyPostprocessFSPath,
		{}, {}, {}
	};
	static std::once_flag onceFlag;
	std::call_once(onceFlag, []() {
		material.setVertexTypeIndex(util::TypeIndex<vert::VertexPT0>());
	});
	return material;
}

bool AfterglowMaterial::is(const AfterglowMaterial& other) const noexcept {
	return this == &other;
}

void AfterglowMaterial::setVertexTypeIndex(std::type_index vertexTypeIndex) noexcept {
	_vertexTypeIndex = vertexTypeIndex;
}

void AfterglowMaterial::setCullMode(render::CullMode cullMode) noexcept {
	_cullMode = cullMode;
}

void AfterglowMaterial::setWireframe(bool wireframe) noexcept {
	_wireframe = wireframe;
}

void AfterglowMaterial::setDepthWrite(bool depthWrite) noexcept {
	_depthWrite = depthWrite;
}

void AfterglowMaterial::setTopology(render::Topology topology) noexcept {
	_topology = topology;
}

void AfterglowMaterial::setVertexShader(const std::string& shaderPath) {
	_vertexShaderPath = shaderPath;
}

void AfterglowMaterial::setFragmentShader(const std::string& shaderPath) {
	_fragmentShaderPath = shaderPath;
}

void AfterglowMaterial::setCustomPass(const std::string& passName) {
	_customPassName = passName;
}

void AfterglowMaterial::setSubpass(const std::string& subpassName) {
	_subpassName = subpassName;
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

render::Domain AfterglowMaterial::domain() const noexcept {
	return _domain;
}

void AfterglowMaterial::setDomain(render::Domain domain) noexcept {
	_domain = domain;
}

const render::FaceStencilInfos& AfterglowMaterial::faceStencilInfos() const noexcept {
	return _faceStencilInfos;
}

void AfterglowMaterial::setFaceStencilInfo(const render::FaceStencilInfos& faceStencilInfo) noexcept {
	_faceStencilInfos = faceStencilInfo;
}

std::type_index AfterglowMaterial::vertexTypeIndex() const noexcept {
	return _vertexTypeIndex;
}

render::CullMode AfterglowMaterial::cullMode() const noexcept {
	return _cullMode;
}

bool AfterglowMaterial::wireframe() const noexcept {
	return _wireframe;
}

bool AfterglowMaterial::depthWrite() const noexcept {
	return _depthWrite;
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

AfterglowMaterial::Parameters<AfterglowMaterial::Scalar>& AfterglowMaterial::scalars() noexcept {
	return _scalars;
}

AfterglowMaterial::Parameters<AfterglowMaterial::Vector>& AfterglowMaterial::vectors() noexcept {
	return _vectors;
}

AfterglowMaterial::Parameters<AfterglowMaterial::TextureInfo>& AfterglowMaterial::textures() noexcept {
	return _textures;
}

const AfterglowMaterial::Parameters<AfterglowMaterial::Scalar>& AfterglowMaterial::scalars() const noexcept {
	return _scalars;
}

const AfterglowMaterial::Parameters<AfterglowMaterial::Vector>& AfterglowMaterial::vectors() const noexcept {
	return _vectors;
}

const AfterglowMaterial::Parameters<AfterglowMaterial::TextureInfo>& AfterglowMaterial::textures() const noexcept {
	return _textures;
}

const std::string& AfterglowMaterial::vertexShaderPath() const noexcept {
	return _vertexShaderPath;
}

const std::string& AfterglowMaterial::fragmentShaderPath() const noexcept {
	return _fragmentShaderPath;
}

const std::string& AfterglowMaterial::customPassName() const noexcept {
	return _customPassName;
}

const std::string& AfterglowMaterial::subpassName() const noexcept {
	return _subpassName;
}

render::Topology AfterglowMaterial::topology() const noexcept {
	return _topology;
}

uint32_t AfterglowMaterial::scalarPaddingSize(shader::Stage stage) const noexcept {
	auto iterator = _scalars.find(stage);
	if (iterator == _scalars.end()) {
		return 0;
	}
	size_t numScalars = iterator->second.size();
	return static_cast<uint32_t>(util::Align(numScalars, elementAlignment()) - numScalars);
}

bool AfterglowMaterial::hasComputeTask() const noexcept {
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
	return *_computeTask;
}

const AfterglowComputeTask& AfterglowMaterial::computeTask() const {
	return const_cast<AfterglowMaterial*>(this)->computeTask();
}
