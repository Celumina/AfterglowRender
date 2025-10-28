#include "AfterglowMaterialAsset.h"
#include <fstream>
#include <mutex>
#include <json.hpp>

#include "AfterglowMaterial.h"
#include "AfterglowComputeTask.h"
#include "GlobalAssets.h"
#include "Configurations.h"
#include "DebugUtilities.h"

struct AfterglowMaterialAsset::Impl {
	AfterglowMaterial material;
	ShaderDeclarations shaderDeclarations;
	ShaderAssets shaderAssets;
	nlohmann::json data;
};

AfterglowMaterialAsset::AfterglowMaterialAsset(const std::string& path):
	_impl(std::make_unique<Impl>()) {

	std::ifstream file(path);

	if (!file.is_open()) {
		DEBUG_CLASS_ERROR("Failed to load material file: " + path);
		throw std::runtime_error("[AfterglowMaterialAsset] Failed to open material file.");
	}
	try {
		file >> _impl->data;
	}
	catch (const nlohmann::json::parse_error& error) {
		DEBUG_CLASS_ERROR("Failed to parse material file " + path + " to json, due to: " + error.what());
		throw std::runtime_error("[AfterglowMaterialAsset] Failed to parse material file to json.");
	}

	initMaterial();
	initMaterialComputeTask();
	parseShaderDeclarations();
	loadShaderAssets();
}

AfterglowMaterialAsset::AfterglowMaterialAsset(AfterglowMaterialAsset&& rval)  noexcept  :
	_impl(std::move(rval._impl)) {

}

AfterglowMaterialAsset::AfterglowMaterialAsset(const AfterglowMaterial& material) noexcept : 
	_impl(std::make_unique<Impl>()) {
	_impl->material = material;
	parseShaderDeclarations();
	loadShaderAssets();
}

AfterglowMaterialAsset::~AfterglowMaterialAsset()  noexcept  {
}

const AfterglowMaterial& AfterglowMaterialAsset::material() const {
	return _impl->material;
}

std::string AfterglowMaterialAsset::materialName() const {
	auto& data = _impl->data;
	if (data.contains("name") &&data["name"].is_string()) {
		return data["name"];
	}
	DEBUG_CLASS_ERROR("Failed to acquire material name, make sure asset is created form path, and file context includes \"name\" segment.");
	return "";
}

const std::string& AfterglowMaterialAsset::shaderDeclaration(shader::Stage shaderStage) const {
	auto iterator = _impl->shaderDeclarations.find(shaderStage);
	if (iterator == _impl->shaderDeclarations.end()) {
		return "";
	}
	return iterator->second;
}

std::string AfterglowMaterialAsset::generateShaderCode(
	shader::Stage shaderStage, 
	util::OptionalRef<render::InputAttachmentInfos> inputAttachmentInfos) const {
	if (_impl->shaderDeclarations.find(shaderStage) == _impl->shaderDeclarations.end() 
		|| _impl->shaderAssets.find(shaderStage) == _impl->shaderAssets.end()) {
		throw std::runtime_error("[AfterglowMaterialAsset] Shader declaration or code is not exist.");
	}
	std::string inputAttachmentDeclaration;
	// Only Fragment shader support input attachments.
	if (inputAttachmentInfos && shaderStage == shader::Stage::Fragment) {
		inputAttachmentDeclaration = makeInputAttachmentDeclaration(*inputAttachmentInfos);
	}
	return std::string(
	_impl->shaderDeclarations.at(shaderStage)
	+ inputAttachmentDeclaration
	+ _impl->shaderAssets.at(shaderStage).code()
	);
}

void AfterglowMaterialAsset::initMaterial() {
	auto& data = _impl->data;
	auto& material = _impl->material;
	if (data.contains("vertexShaderPath") && data["vertexShaderPath"].is_string()) {
		material.setVertexShader(data["vertexShaderPath"]);
	}
	if (data.contains("fragmentShaderPath") && data["fragmentShaderPath"].is_string()) {
		material.setFragmentShader(data["fragmentShaderPath"]);
	}
	if (data.contains("domain") && data["domain"].is_number_integer()) {
		material.setDomain(data["domain"]);
	}
	if (data.contains("topology") && data["topology"].is_number_integer()) {
		material.setTopology(data["topology"]);
	}
	if (data.contains("vertexType") && data["vertexType"].is_number_integer()) {
		material.setVertexTypeIndex(vertexTypeIndex(data["vertexType"]));
	}
	if (data.contains("twoSided") && data["twoSided"].is_boolean()) {
		material.setTwoSided(data["twoSided"]);
	}

	if (data.contains("scalars") && data["scalars"].is_array()) {
		for (const auto& scalar : data["scalars"]) {
			if (scalar.contains("name") && scalar["name"].is_string()
				&& scalar.contains("value") && scalar["value"].is_number()
				&& scalar.contains("stage") && scalar["stage"].is_number_integer()) {
				material.setScalar(scalar["stage"], scalar["name"], scalar["value"]);
			}
		}
	}
	
	if (data.contains("vectors") && data["vectors"].is_array()) {
		for (const auto& vector : data["vectors"]) {
			if (!vector.contains("name") || !vector["name"].is_string()
				|| !vector.contains("value") || !vector["value"].is_array()
				|| !vector.contains("stage") || !vector["stage"].is_number_integer()) {
				continue;
			}
			auto& value = vector["value"];
			if (value.size() >= 4 && value[0].is_number() && value[1].is_number() && value[2].is_number() && value[3].is_number()) {
				material.setVector(vector["stage"], vector["name"], { value[0], value[1], value[2], value[3] });
			}
		}
	}
	
	if (data.contains("textures") && data["textures"].is_array()) {
		for (const auto& texture : data["textures"]) {
			if (!texture.contains("name") || !texture["name"].is_string()
				|| !texture.contains("value") || !texture["value"].is_object()
				|| !texture.contains("stage") || !texture["stage"].is_number_integer()) {
				continue;
			}
			auto& value = texture["value"];
			if (!value.contains("path") || !value["path"].is_string()
				|| !value.contains("format") || !value["format"].is_number_integer()
				|| !value.contains("colorSpace") || !value["colorSpace"].is_number_integer()) {
				continue;
			}
			material.setTexture(texture["stage"], texture["name"], {value["format"], value["colorSpace"], value["path"]});
		}
	}
}

void AfterglowMaterialAsset::initMaterialComputeTask() {
	auto& data = _impl->data;

	if (!data.contains("computeTask") || !data["computeTask"].is_object()) {
		return;
	}

	auto& computeTaskData = data["computeTask"];
	auto& computeTask = _impl->material.initComputeTask();

	if (computeTaskData.contains("computeOnly") && computeTaskData["computeOnly"].is_boolean()) {
		computeTask.setComputeOnly(computeTaskData["computeOnly"]);
	}
	if (computeTaskData.contains("computeShaderPath") && computeTaskData["computeShaderPath"].is_string()) {
		computeTask.setComputeShader(computeTaskData["computeShaderPath"]);
	}
	if (computeTaskData.contains("dispatchGroup")
		&& computeTaskData["dispatchGroup"].is_array()
		&& computeTaskData["dispatchGroup"].size() >= 3) {
		auto& value = computeTaskData["dispatchGroup"];
		if (value[0].is_number_integer() && value[1].is_number_integer() && value[2].is_number_integer()) {
			computeTask.setDispatchGroup({ value[0], value[1], value[2] });
		}
	}
	if (computeTaskData.contains("dispatchFrequency") && computeTaskData["dispatchFrequency"].is_number_integer()) {
		computeTask.setDispatchFrequency(computeTaskData["dispatchFrequency"]);
	}

	// SSBO Infos
	if (!computeTaskData.contains("ssboInfos") || !computeTaskData["ssboInfos"].is_array()) {
		return;
	} 
	auto& ssboInfos = computeTask.ssboInfos();
	for (const auto& ssboInfoData : computeTaskData["ssboInfos"]) {
		if (!ssboInfoData.is_object()
			|| !ssboInfoData.contains("name") || !ssboInfoData["name"].is_string()
			|| !ssboInfoData.contains("stage") || !ssboInfoData["stage"].is_number_integer()
			|| !ssboInfoData.contains("usage") || !ssboInfoData["usage"].is_number_integer()
			|| !ssboInfoData.contains("accessMode") || !ssboInfoData["accessMode"].is_number_integer()
			|| !ssboInfoData.contains("initMode") || !ssboInfoData["initMode"].is_number_integer()
			|| !ssboInfoData.contains("initResource") || !ssboInfoData["initResource"].is_string()
			|| !ssboInfoData.contains("elementLayout") || !ssboInfoData["elementLayout"].is_array()
			|| !ssboInfoData.contains("numElements") || !ssboInfoData["numElements"].is_number_integer()) {
			continue;
		}
		auto& layoutData = ssboInfoData["elementLayout"];
		AfterglowStructLayout elementLayout{};
		for (const auto& attribute : layoutData) {
			if (!attribute.is_object()) {
				continue;
			}
			// This attribute object keep one item only.
			auto iterator = attribute.begin();
			if (iterator == attribute.end() || !iterator.value().is_number_integer()) {
				continue;
			}
			// attributeType, attributeName
			elementLayout.addAttribute(iterator.value(), iterator.key());
		}
		if (elementLayout.byteSize() == 0) {
			continue;
		}
		ssboInfos.emplace_back(AfterglowSSBOInfo {
			.name = ssboInfoData["name"],
			.stage = ssboInfoData["stage"],
			.usage = ssboInfoData["usage"],
			.accessMode = ssboInfoData["accessMode"],
			.initMode = ssboInfoData["initMode"],
			.initResource = ssboInfoData["initResource"],
			.elementLayout = elementLayout,
			.numElements = ssboInfoData["numElements"]
		});
	}
}

void AfterglowMaterialAsset::parseShaderDeclarations() {	
	// <shader::Stage, {ScalarCount, memberDeclarations}>
	// ScalarCount use for memory alignment.
	StageDeclarations<UniformMemberDeclaration> uniformMemberDeclarations;
	fillUniformMemberDeclarations(uniformMemberDeclarations);

	StageDeclarations<TextureDeclaration> textureDeclarations;
	fillTextureDeclarations(textureDeclarations);	

	std::string globalUniformStructDeclaration = makeUniformStructDeclaration(
		"GlobalUniform",
		makeUniformMemberDeclarationContext<ubo::GlobalUniform>(),
		shader::SetIndex::Global,
		util::EnumValue(shader::GlobalSetBindingIndex::GlobalUniform)
	);

	std::string perObjectUniformStructDeclaration = makeUniformStructDeclaration(
		"PerObjectUniform",
		makeUniformMemberDeclarationContext<ubo::MeshUniform>(),
		shader::SetIndex::PerObject,
		util::EnumValue(shader::PerObjectSetBindingIndex::MeshUniform)
	);

	// Shared uniform struct declaration.
	std::string sharedUniformStructDeclaration = makeUniformStructDeclaration(
		"MaterialSharedUniform",
		uniformMemberDeclarations[shader::Stage::Shared].declaration,
		shader::SetIndex::MaterialShared,
		0
	);

	// Vertex Shader
	std::string& vertexShaderDeclaration = _impl->shaderDeclarations[shader::Stage::Vertex];
	// Vertex Shader: global uniform
	vertexShaderDeclaration += globalUniformStructDeclaration;
	// Vertex Shader: Mesh uniform	
	vertexShaderDeclaration += perObjectUniformStructDeclaration;
	// Vertex Shader: Vertex stage uniform declaration.
	vertexShaderDeclaration += makeUniformStructDeclaration(
		"MaterialVertexUniform", 
		uniformMemberDeclarations[shader::Stage::Vertex].declaration, 
		shader::SetIndex::MaterialVertex, 
		0
	);
	// Vertex Shader: shared uniform declaration.
	vertexShaderDeclaration += sharedUniformStructDeclaration;
	// Vertex Shader: Texture declarations.
	vertexShaderDeclaration += textureDeclarations[shader::Stage::Vertex].declaration;
	vertexShaderDeclaration += textureDeclarations[shader::Stage::Shared].declaration;
	// Vertex Shader: Vertex input struct declaration.
	auto& material = _impl->material;
	if (!material.hasComputeTask() || !material.computeTask().vertexInputSSBO()) {
		vertexShaderDeclaration += vertexInputStructDeclaration(material.vertexTypeIndex());
	}
	else { 
		// If compute task ssbo as vertex input was defined.
		vertexShaderDeclaration += vertexInputStructDeclaration(material.computeTask().vertexInputSSBO()->elementLayout);
	}
	
	// Fragment Shader
	std::string& fragmentShaderDeclaration = _impl->shaderDeclarations[shader::Stage::Fragment];
	// Fragment Shader: global uniform
	fragmentShaderDeclaration += globalUniformStructDeclaration;
	// Fragment Shader: global textures
	fragmentShaderDeclaration += makeGlobalCombinedTextureSamplerDeclarations();
	// Fragment Shader: Fragment stage uniform declaration.
	fragmentShaderDeclaration += makeUniformStructDeclaration(
		"MaterialFragmentUniform", 
		uniformMemberDeclarations[shader::Stage::Fragment].declaration,
		shader::SetIndex::MaterialFragment,
		0
	);
	// Fragment Shader: shared uniform declaration.
	fragmentShaderDeclaration += sharedUniformStructDeclaration;
	// Fragment Shader: Texture declarations.
	fragmentShaderDeclaration += textureDeclarations[shader::Stage::Fragment].declaration;
	fragmentShaderDeclaration += textureDeclarations[shader::Stage::Shared].declaration;

	if (_impl->material.hasComputeTask()) {
		// Storage buffer will be filled after textures, so stage beginBindingIndices are aquire from textures.
		StageDeclarations<StorageBufferDeclaration> storageBufferDeclarations;
		fillStorageBufferDeclarations(storageBufferDeclarations, textureDeclarations);

		std::string storageBufferStructDeclaration = makeStorageBufferStructDeclarations();
		// Compute Shader
		std::string& computeShaderDeclaration = _impl->shaderDeclarations[shader::Stage::Compute];
		computeShaderDeclaration += globalUniformStructDeclaration;
		computeShaderDeclaration += perObjectUniformStructDeclaration;
		computeShaderDeclaration += storageBufferStructDeclaration;
		computeShaderDeclaration += storageBufferDeclarations[shader::Stage::Compute].declaration;
		computeShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeVertex].declaration;
		computeShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeFragment].declaration;

		// Additional compute declaration to VS and FS.
		// TODO: Remove them multi frame ssbos last..frame and RW features.
		vertexShaderDeclaration += storageBufferStructDeclaration;
		vertexShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeVertex].declaration;

		fragmentShaderDeclaration += storageBufferStructDeclaration;
		fragmentShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeFragment].declaration;
	}
}

void AfterglowMaterialAsset::loadShaderAssets() {
	auto& material = _impl->material;
	auto& shaderAsset = _impl->shaderAssets;
	// We just assume that shader is exists, ShaderAsset weill throw a error, just let them go.
	shaderAsset.emplace(shader::Stage::Vertex, material.vertexShaderPath());
	shaderAsset.emplace(shader::Stage::Fragment, material.fragmentShaderPath());
	if (material.hasComputeTask()) {
		shaderAsset.emplace(shader::Stage::Compute, material.computeTask().computeShaderPath());
	}
}

void AfterglowMaterialAsset::fillUniformMemberDeclarations(StageDeclarations<UniformMemberDeclaration>& destMemberDeclarations) {
	const auto& material = _impl->material;
	
	for (const auto& [stage, scalarParams] : material.scalars()) {
		auto& memberDeclaration = destMemberDeclarations[stage];
		// Fill scalar member declarations.
		for (const auto& scalarParam : scalarParams) {
			// Note that material name should fit to program syntax.
			memberDeclaration.declaration += std::format("\t{} {};\n", "float", scalarParam.name);
		}
		// Fill padding scalar member declarations.
		uint32_t numPaddings = material.scalarPaddingSize(stage);
		for (uint32_t index = 0; index < numPaddings; ++index) {
			memberDeclaration.declaration += std::format(
				"\t{} {};\n",
				"float",
				std::string("__padding_") + std::to_string(util::EnumValue(stage)) + "_" + std::to_string(index)
			);
		}
	}
	// Fill vector member decalrations.
	for (const auto& [stage, vectorParams] : material.vectors()) {
		auto& memberDeclaration = destMemberDeclarations[stage];
		for (const auto& vectorParam : vectorParams) {
			memberDeclaration.declaration += std::format("\t{} {};\n", "float4", vectorParam.name);
		}
	}
}

inline void AfterglowMaterialAsset::fillTextureDeclarations(StageDeclarations<TextureDeclaration>& destTextureDeclarations) {
	const auto& material = _impl->material;
	for (const auto& [stage, textureParams] : material.textures()) {
		auto& textureDeclaration = destTextureDeclarations[stage];
		// First binding is uniform, skip it.
		uint32_t bindingIndex = 1;
		for (const auto& textureParam : textureParams) {
			textureDeclaration.declaration += makeCombinedTextureSamplerDeclaration(
				util::EnumValue(stage),
				bindingIndex,
				img::HLSLPixelTypeName(textureParam.value.format),
				textureParam.name
			);
			textureDeclaration.BindingEndIndex = bindingIndex;
			++bindingIndex;
		}
	}
}

inline void AfterglowMaterialAsset::fillStorageBufferDeclarations(
	StageDeclarations<StorageBufferDeclaration>& destStorageBufferDeclarations,
	const StageDeclarations<TextureDeclaration>& textureDeclarations) {
	const auto& material = _impl->material;
	auto& computeTask = material.computeTask();

	/* Multiple SSBOs for Frame in Flight
	 If accessMode is `ReadWrite`,
	 program will declare multiply SSBOs automatically for frame in flight.
	 In this multiply method, Last SSBO is ReadWrite and before then are all ReadOnly.
	 Also a suffix of these multiple buffers will be appended for compute shader, e.g:
		`source name` :
			SSBOName
		`multipleSSBO names` :
			SSBONameIn		(Last Frame)
			SSBONameIn1		(Last Last Frame)
			SSBONameIn2		(Last Last Last Frame)
			...				(...)
			SSBONameOut		(Current Frame)
	 These SSBOs are not fixed by name, They exchange name and actual buffer frame by frame, SSBONameOut is the SSBO of current frame index.
	*/
	for (const auto& ssboInfo : computeTask.ssboInfos()) {
		StorageBufferDeclaration* storageBufferDeclaration = nullptr;
		auto storageBufferDeclarationIterator = destStorageBufferDeclarations.find(ssboInfo.stage);

		// Init texture declarations binding index offset.
		if (storageBufferDeclarationIterator == destStorageBufferDeclarations.end()) {
			storageBufferDeclaration = &destStorageBufferDeclarations.insert({ssboInfo.stage, StorageBufferDeclaration{}}).first->second;
			auto textureDeclarationIterator = textureDeclarations.find(ssboInfo.stage);
			if (textureDeclarationIterator != textureDeclarations.end()) {
				storageBufferDeclaration->BindingEndIndex = textureDeclarationIterator->second.BindingEndIndex + 1;
			}
			else {
				storageBufferDeclaration->BindingEndIndex = 1;
			}
		}
		else {
			storageBufferDeclaration = &storageBufferDeclarationIterator->second;
		}

		std::string structName = ssboInfo.name + "Struct";
		auto declarationNames = computeTask.ssboInfoDeclarationNames(ssboInfo);
		auto accessMode = compute::SSBOAccessMode::ReadOnly;
		for (uint32_t nameIndex = 0; nameIndex < declarationNames.size(); ++nameIndex) {
			if (nameIndex == declarationNames.size() - 1) {
				accessMode = ssboInfo.accessMode;
			}

			storageBufferDeclaration->declaration += makeStorageBufferDeclaration(
				util::EnumValue(ssboInfo.stage),
				storageBufferDeclaration->BindingEndIndex,
				accessMode,
				structName,
				declarationNames[nameIndex]
			);
			++storageBufferDeclaration->BindingEndIndex;
		}
	}
}

inline std::string AfterglowMaterialAsset::makeUniformStructDeclaration(
	const std::string& structName, 
	const std::string& memberContext, 
	shader::SetIndex shaderSet, 
	uint32_t bindingIndex) {
	std::string declaration;
	declaration += std::format(
		"[[vk::binding({}, {})]] cbuffer {} {{\n",
		bindingIndex,
		util::EnumValue(shaderSet),
		structName
	);
	declaration += memberContext;
	declaration += "};\n";
	return declaration;
}

inline std::string AfterglowMaterialAsset::makeCombinedTextureSamplerDeclaration(
	uint32_t setIndex, 
	uint32_t bindingIndex, 
	const std::string& typeName, 
	const std::string& name, 
	bool isMultiSample) {
	std::string headDeclaration = std::format(
		"[[vk::combinedImageSampler]] [[vk::binding({}, {})]] ",
		bindingIndex, 
		setIndex
	);

	std::string declaration = headDeclaration;
	// TODO: Seprated texture and sampler mode.
	// TODO: Texture3D support.
	// Texture
	if (isMultiSample) {
		declaration += std::format(
			"Texture2DMS<{}> {};\n",
			typeName,
			name
		);
	}
	else {
		declaration += std::format(
			"Texture2D<{}> {};\n",
			typeName,
			name
		);
	}
	
	// Texture Sampler
	declaration += headDeclaration;
	declaration += std::format(
		"SamplerState {};\n",
		name + "Sampler"
	);
	return declaration;
}

inline std::string AfterglowMaterialAsset::makeStorageBufferDeclaration(uint32_t setIndex,
	uint32_t bindingIndex,
	compute::SSBOAccessMode accessMode,
	const std::string& structName,
	const std::string& storageBufferName) {
	if (accessMode == compute::SSBOAccessMode::ReadOnly) {
		return std::format(
			"[[vk::binding({}, {})]] StructuredBuffer<{}> {} : register(t{}, space{});\n", 
			bindingIndex, setIndex, structName, storageBufferName, bindingIndex, setIndex
		);
	}
	else if (accessMode == compute::SSBOAccessMode::ReadWrite) {
		return std::format(
			"[[vk::binding({}, {})]] RWStructuredBuffer<{}> {} : register(u{}, space{});\n",
			bindingIndex, setIndex, structName, storageBufferName, bindingIndex, setIndex
		);
	}
	DEBUG_TYPE_WARNING(AfterglowMaterialAsset, "Access mode is undefined.");
}

inline std::string AfterglowMaterialAsset::makeStorageBufferStructDeclarations() {
	std::string declarations;
	auto& ssboInfos = _impl->material.computeTask().ssboInfos();
	 for (const auto& ssboInfo : ssboInfos) {
		declarations += std::format("struct {} {{\n", ssboInfo.name + "Struct");
		auto attributeMembers = ssboInfo.elementLayout.generateHLSLStructMembers();
		for (const auto& member : attributeMembers) {
			declarations += std::format("{} {};\n", AfterglowStructLayout::hlslTypeName(member.type), member.name);
		}
		// TODO: Padding attributes.
		 declarations += "};\n";
	 }
	 return declarations;
}

inline std::string AfterglowMaterialAsset::vertexInputStructDeclaration(const AfterglowStructLayout& structLayout) {
	std::string declaration;
	uint32_t location = 0;
	declaration += "struct VSInput {\n";
	auto attributeMembers = structLayout.generateHLSLStructMembers();
	for (const auto& member : attributeMembers) {
		declaration += std::format(
			"[[vk::location({})]] {} {} : {};\n",
			location,
			AfterglowStructLayout::hlslTypeName(member.type),
			member.name,
			util::UpperCase(member.name)
		);
		++location;
	}
	declaration += "};\n";
	return declaration;
}

inline std::string AfterglowMaterialAsset::makeInputAttachmentDeclaration(const render::InputAttachmentInfos& inputAttachmentInfos) const {
	std::string declaration;
	auto domain = _impl->material.domain();
	uint32_t bindingIndex = util::EnumValue(shader::GlobalSetBindingIndex::EnumCount);
	for (const auto& info : inputAttachmentInfos) {
		if (info.domain == domain) {
			// Texture sampler method.
			declaration += makeCombinedTextureSamplerDeclaration(
				util::EnumValue(shader::SetIndex::Global), 
				bindingIndex, 
				render::HLSLTexturePixelTypeName(info.type),
				info.name, 
				info.isMultiSample
			);
			// Subpass method.			
			//std::format(
			//	"[[vk::input_attachment_index({})]] SubpassInput<{}> {}; \n",
			//	shader::AttachmentTextureBindingIndex(bindingIndex),
			//	render::inputAttachmentPixelTypeNames[util::EnumValue(info.type)],
			//	info.name
			//);
		}
		++bindingIndex;
	}
	return declaration;
}

inline std::string AfterglowMaterialAsset::makeGlobalCombinedTextureSamplerDeclarations() {
	static std::once_flag onceFlag;
	static std::string declarations;

	std::call_once( onceFlag, [&]() {
		Inreflect<shader::GlobalSetBindingIndex>::forEachAttribute([&](auto enumInfo) {
			if (!enumInfo.name.ends_with(Inreflect<shader::GlobalSetBindingResource>::enumName(shader::GlobalSetBindingResource::Texture))) {
				return;
			}
			declarations += makeCombinedTextureSamplerDeclaration(
				util::EnumValue(shader::SetIndex::Global),
				enumInfo.value,
				img::HLSLPixelTypeName(shader::GlobalSetBindingTextureInfo(enumInfo.raw).format),
				std::string(enumInfo.name)
			);
		});
	});
	
	return declarations;
}
