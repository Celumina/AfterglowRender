#include "AfterglowMaterialAsset.h"
#include <fstream>
#include <mutex>
#include <json.hpp>

#include "AfterglowMaterial.h"
#include "AfterglowComputeTask.h"
#include "GlobalAssets.h"
#include "Configurations.h"
#include "ExceptionUtilities.h"

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
		EXCEPT_CLASS_RUNTIME("Failed to load material file: " + path);
	}
	try {
		file >> _impl->data;
	}
	catch (const nlohmann::json::parse_error& error) {
		EXCEPT_CLASS_RUNTIME("Failed to parse material file " + path + " to json, due to: " + error.what());
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

AfterglowMaterialAsset::~AfterglowMaterialAsset() noexcept {
}

const AfterglowMaterial& AfterglowMaterialAsset::material() const noexcept {
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
		EXCEPT_CLASS_RUNTIME(std::format("Shader declaration not found, in material: \"{}\"", materialName()));
	}
	return iterator->second;
}

const std::string& AfterglowMaterialAsset::shaderBody(shader::Stage shaderStage) const {
	auto iterator = _impl->shaderAssets.find(shaderStage);
	if (iterator == _impl->shaderAssets.end()) {
		EXCEPT_CLASS_RUNTIME(std::format("Shader asset not found, in material: \"{}\"", materialName()));
	}
	return iterator->second.code();
}

std::string AfterglowMaterialAsset::generateShaderCode(
	shader::Stage shaderStage, 
	util::OptionalRef<AfterglowPassInterface> pass,
	util::OptionalRef<std::vector<const AfterglowSSBOInfo*>> externalSSBOInfoRefs) const {
	const std::string& declaration = shaderDeclaration(shaderStage);
	const std::string& body = shaderBody(shaderStage);

	std::string extraDeclarations;
	// Only Fragment shader support import and input attachments.
	if (pass && shaderStage == shader::Stage::Fragment) {
		uint32_t bindingIndex = 0;
		extraDeclarations += makeImportAttachmentDeclaration(*pass, bindingIndex);

		auto& inputAttachmentInfos = (*pass).get().subpassContext().inputAttachmentInfos();
		extraDeclarations += makeInputAttachmentDeclaration(inputAttachmentInfos, bindingIndex);
	}
	if (externalSSBOInfoRefs) {
		extraDeclarations += makeComputeExternalSSBODeclarations(*externalSSBOInfoRefs);
	}

	return std::string(declaration + extraDeclarations + body);
}

// TODO: Deserializer
void AfterglowMaterialAsset::initMaterial() {
	auto& data = _impl->data;
	auto& material = _impl->material;
	if (data.contains("vertexShaderPath") && data["vertexShaderPath"].is_string()) {
		material.setVertexShader(data["vertexShaderPath"]);
	}
	if (data.contains("fragmentShaderPath") && data["fragmentShaderPath"].is_string()) {
		material.setFragmentShader(data["fragmentShaderPath"]);
	}
	// The name "domain" is from a legacy reason. 
	if (data.contains("domain") && data["domain"].is_string()) {
		std::string domainName = data["domain"];
		Inreflect<render::Domain>::forEachAttribute([&domainName, &material](auto enumInfo){
			if (enumInfo.name == domainName) {
				material.setDomain(enumInfo.raw);
			}
		});
	}
	if (data.contains("customPassName") && data["customPassName"].is_string()) {
		material.setCustomPass(data["customPassName"]);
	}
	if (data.contains("subpassName") && data["subpassName"].is_string()) {
		material.setSubpass(data["subpassName"]);
	}
	if (data.contains("topology") && data["topology"].is_number_integer()) {
		material.setTopology(data["topology"]);
	}
	if (data.contains("vertexType") && data["vertexType"].is_number_integer()) {
		material.setVertexTypeIndex(vertexTypeIndex(data["vertexType"]));
	}
	if (data.contains("cullMode") && data["cullMode"].is_number_integer()) {
		material.setCullMode(data["cullMode"]);
	}
	if (data.contains("wireframe") && data["wireframe"].is_boolean()) {
		material.setWireframe(data["wireframe"]);
	}
	if (data.contains("depthWrite") && data["depthWrite"].is_boolean()) {
		material.setDepthWrite(data["depthWrite"]);
	}
	if (data.contains("stencil") && data["stencil"].is_object()) {
		render::FaceStencilInfos faceStencilInfos{};
		initMaterialStencilInfo("front", faceStencilInfos.front);
		initMaterialStencilInfo("back", faceStencilInfos.back);
		material.setFaceStencilInfo(std::move(faceStencilInfos));
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
				|| !value.contains("colorSpace") || !value["colorSpace"].is_number_integer()) {
				continue;
			}
			material.setTexture(texture["stage"], texture["name"], {value["colorSpace"], value["path"]});
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

	// [Optional] External SSBOs
	if (computeTaskData.contains("externalSSBOs") &&  computeTaskData["externalSSBOs"].is_array()) {
		auto& externalSSBOs = computeTask.externalSSBOs();
		for (const auto& externalSSBOData : computeTaskData["externalSSBOs"]) {
			if (!externalSSBOData.is_object()
				|| !externalSSBOData.contains("materialName") || !externalSSBOData["materialName"].is_string()
				|| !externalSSBOData.contains("ssboName") || !externalSSBOData["ssboName"].is_string()) {
					continue;
				}
			externalSSBOs.emplace_back(externalSSBOData["materialName"], externalSSBOData["ssboName"]);
		}
	}

	// SSBO Infos
	if (!computeTaskData.contains("ssboInfos") || !computeTaskData["ssboInfos"].is_array()) {
		return;
	} 

	for (const auto& ssboInfoData : computeTaskData["ssboInfos"]) {
		if (!ssboInfoData.is_object()
			|| !ssboInfoData.contains("name") || !ssboInfoData["name"].is_string()
			|| !ssboInfoData.contains("stage") || !ssboInfoData["stage"].is_number_integer()
			|| !ssboInfoData.contains("usage") || !ssboInfoData["usage"].is_number_integer()
			|| !ssboInfoData.contains("accessMode") || !ssboInfoData["accessMode"].is_number_integer()
			|| !ssboInfoData.contains("initMode") || !ssboInfoData["initMode"].is_number_integer()
			|| !ssboInfoData.contains("initResource") || !ssboInfoData["initResource"].is_string()
			|| !ssboInfoData.contains("numElements") || !ssboInfoData["numElements"].is_number_integer()) {
			continue;
		}
		
		AfterglowStructLayout elementLayout{};

		// Optional keys
		compute::SSBOTextureMode textureMode = compute::SSBOTextureMode::Unused;
		compute::SSBOTextureDimension textureDimension = compute::SSBOTextureDimension::Texture2D;
		compute::SSBOTextureSampleMode textureSampleMode = compute::SSBOTextureSampleMode::LinearRepeat;
		if (ssboInfoData.contains("textureMode") && ssboInfoData["textureMode"].is_number_integer()) {
			textureMode = ssboInfoData["textureMode"];
		}
		if (ssboInfoData.contains("textureDimension") && ssboInfoData["textureDimension"].is_number_integer()) {
			textureDimension = ssboInfoData["textureDimension"];
		}
		if (ssboInfoData.contains("textureSampleMode") && ssboInfoData["textureSampleMode"].is_number_integer()) {
			textureSampleMode = ssboInfoData["textureSampleMode"];
		}

		// ElementLayout
		if (ssboInfoData.contains("elementLayout") && ssboInfoData["elementLayout"].is_array()) {
			auto& layoutData = ssboInfoData["elementLayout"];
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
		}

		 // Discard invalid ssbo info.
		if (elementLayout.byteSize() == 0 
			&& textureMode == compute::SSBOTextureMode::Unused
			&& !computeTask.hasDefaultElementLayout(ssboInfoData["usage"])) {
			continue;
		}

		computeTask.appendSSBOInfo({
			ssboInfoData["name"],
			ssboInfoData["stage"],
			ssboInfoData["usage"],
			ssboInfoData["accessMode"],
			textureMode, 
			textureDimension, 
			textureSampleMode, 
			ssboInfoData["initMode"],
			ssboInfoData["initResource"],
			elementLayout,
			ssboInfoData["numElements"]
		});
	}
}

inline void AfterglowMaterialAsset::initMaterialStencilInfo(std::string_view srcFaceName, render::StencilInfo& dstStencilInfo) {
	auto& srcStencilInfoData = _impl->data["stencil"][srcFaceName];
	if (srcStencilInfoData.contains("stencilValue") && srcStencilInfoData["stencilValue"].is_number_integer()) {
		dstStencilInfo.stencilValue = srcStencilInfoData["stencilValue"];
	}
	if (srcStencilInfoData.contains("compareMask") && srcStencilInfoData["compareMask"].is_number_integer()) {
		dstStencilInfo.compareMask = srcStencilInfoData["compareMask"];
	}
	if (srcStencilInfoData.contains("writeMask") && srcStencilInfoData["writeMask"].is_number_integer()) {
		dstStencilInfo.writeMask = srcStencilInfoData["writeMask"];
	}
	if (srcStencilInfoData.contains("compareOperation") && srcStencilInfoData["compareOperation"].is_number_integer()) {
		dstStencilInfo.compareOperation = srcStencilInfoData["compareOperation"];
	}
	if (srcStencilInfoData.contains("failOperation") && srcStencilInfoData["failOperation"].is_number_integer()) {
		dstStencilInfo.failOperation = srcStencilInfoData["failOperation"];
	}
	if (srcStencilInfoData.contains("passOperation") && srcStencilInfoData["passOperation"].is_number_integer()) {
		dstStencilInfo.passOperation = srcStencilInfoData["passOperation"];
	}
	if (srcStencilInfoData.contains("depthFailOperation") && srcStencilInfoData["depthFailOperation"].is_number_integer()) {
		dstStencilInfo.depthFailOperation = srcStencilInfoData["depthFailOperation"];
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
	if (!material.hasComputeTask() || !material.computeTask().vertexInputSSBOInfo()) {
		vertexShaderDeclaration += vertexInputStructDeclaration(material.vertexTypeIndex());
	}
	else { 
		// If compute task ssbo as vertex input was defined.
		vertexShaderDeclaration += vertexInputStructDeclaration(material.computeTask().vertexInputSSBOInfo()->elementLayout());
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
		for (auto index = shader::computeSetIndexBegin; index < shader::computeSetIndexEnd; ++index) {
			computeShaderDeclaration += storageBufferDeclarations[shader::Stage(index)].declaration;
		}

		// Material parameters
		computeShaderDeclaration += makeUniformStructDeclaration(
			"MaterialComputeUniform",
			uniformMemberDeclarations[shader::Stage::Compute].declaration,
			shader::SetIndex::Compute,
			0
		);
		// Compute shaders are not support the texture parameters due to the binding position conflit with storage buffers.

		// Additional compute declaration to VS and FS.
		// TODO: Remove them multi frame ssbos last..frame and RW features.
		vertexShaderDeclaration += storageBufferStructDeclaration;
		vertexShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeVertex].declaration;
		vertexShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeShared].declaration;

		fragmentShaderDeclaration += storageBufferStructDeclaration;
		fragmentShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeFragment].declaration;
		fragmentShaderDeclaration += storageBufferDeclarations[shader::Stage::ComputeShared].declaration;
	}
}

void AfterglowMaterialAsset::loadShaderAssets() {
	auto& material = _impl->material;
	auto& shaderAsset = _impl->shaderAssets;
	// We assume that shader is exists, ShaderAsset would throw some errors, just let them spread out.
	if (!material.hasComputeTask() || !material.computeTask().isComputeOnly()) {
		shaderAsset.emplace(shader::Stage::Vertex, material.vertexShaderPath());
		shaderAsset.emplace(shader::Stage::Fragment, material.fragmentShaderPath());
	}
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
				/*img::HLSLPixelTypeName(imageInfo.channels, imageInfo.format),*/
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
		auto storageBufferDeclarationIterator = destStorageBufferDeclarations.find(ssboInfo.stage());

		// Init texture declarations binding index offset.
		if (storageBufferDeclarationIterator == destStorageBufferDeclarations.end()) {
			storageBufferDeclaration = &destStorageBufferDeclarations.insert({ssboInfo.stage(), StorageBufferDeclaration{}}).first->second;
			auto textureDeclarationIterator = textureDeclarations.find(ssboInfo.stage());
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

		std::string typeName = storageBufferTypeName(ssboInfo);
		auto declarationNames = computeTask.ssboInfoDeclarationNames(ssboInfo);
		auto accessMode = compute::SSBOAccessMode::ReadOnly;
		for (uint32_t nameIndex = 0; nameIndex < declarationNames.size(); ++nameIndex) {
			if (nameIndex == declarationNames.size() - 1) {
				accessMode = ssboInfo.accessMode();
			}
			if (ssboInfo.isBuffer() || accessMode != compute::SSBOAccessMode::ReadOnly) {
				storageBufferDeclaration->declaration += makeStorageBufferDeclaration(
					util::EnumValue(ssboInfo.stage()),
					storageBufferDeclaration->BindingEndIndex,
					accessMode,
					ssboInfo.hlslTemplateName(),
					typeName,
					declarationNames[nameIndex]
				);
			}
			else {
				// Combined sampler declaration
				storageBufferDeclaration->declaration += makeCombinedTextureSamplerDeclaration(
					util::EnumValue(ssboInfo.stage()), 
					storageBufferDeclaration->BindingEndIndex, 
					declarationNames[nameIndex], 
					false, 
					typeName, 
					ssboInfo.textureDimension()
				);
			}

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
	const std::string& name, 
	bool isMultiSample, 
	const std::string& typeName, 
	compute::SSBOTextureDimension textureDimension) {
	std::string headDeclaration = std::format(
		"[[vk::combinedImageSampler]] [[vk::binding({}, {})]] ",
		bindingIndex, 
		setIndex
	);

	std::string declaration = headDeclaration;
	
	char dimensionChar = '0'; 
	switch (textureDimension) {
	case compute::SSBOTextureDimension::Texture1D:
		dimensionChar = '1';
		break;
	case compute::SSBOTextureDimension::Texture2D:
		dimensionChar = '2';
		break;
	case compute::SSBOTextureDimension::Texture3D:
		dimensionChar = '3';
		break;
	default:
		DEBUG_TYPE_ERROR(AfterglowMaterialAsset, "Unknown texture dimension.");
		break;
	}

	// Texture
	if (isMultiSample) {
		declaration += std::format(
			"Texture{}DMS<{}> {};\n",
			dimensionChar, 
			typeName,
			name
		);
	}
	else {
		declaration += std::format(
			"Texture{}D {};\n",
			dimensionChar, 
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

inline std::string AfterglowMaterialAsset::makeStorageBufferDeclaration(
	uint32_t setIndex,
	uint32_t bindingIndex,
	compute::SSBOAccessMode accessMode,
	const std::string& templateName, 
	const std::string& typeName,
	const std::string& storageBufferName) {

	static const std::string emptyStr("");
	static const std::string readWriteStr("RW");
	const std::string* readWriteSelector = &emptyStr;
	char tuMark = 't';
	if (accessMode == compute::SSBOAccessMode::ReadWrite) {
		readWriteSelector = &readWriteStr;
		tuMark = 'u';
	}

	return std::format(
		"[[vk::binding({}, {})]] {}{}<{}> {} : register({}{}, space{});\n",
		bindingIndex, 
		setIndex, 
		*readWriteSelector, 
		templateName, 
		typeName, 
		storageBufferName, 
		tuMark,
		bindingIndex, 
		setIndex
	);
}

inline std::string AfterglowMaterialAsset::makeStorageBufferStructDeclarations() {
	std::string declarations;
	auto& ssboInfos = _impl->material.computeTask().ssboInfos();
	 for (const auto& ssboInfo : ssboInfos) {
		 declarations += makeStorageBufferStructDeclaration(ssboInfo);
	 }
	 return declarations;
}

inline std::string AfterglowMaterialAsset::makeStorageBufferStructDeclaration(const AfterglowSSBOInfo& ssboInfo) {
	std::string declaration;
	if (!ssboInfo.isBuffer()) {
		return std::string();
	}
	declaration += std::format("struct {} {{\n", ssboInfo.name() + "Struct");
	auto attributeMembers = ssboInfo.elementLayout().generateHLSLStructMembers();
	for (const auto& member : attributeMembers) {
		declaration += std::format("{} {};\n", AfterglowStructLayout::hlslTypeName(member.type), member.name);
	}
	// TODO: Padding attributes.
	declaration += "};\n";
	return declaration;
}

inline std::string AfterglowMaterialAsset::makeComputeExternalSSBODeclarations(const std::vector<const AfterglowSSBOInfo*> ssboInfoRefs) {
	std::string declarations;
	for (uint32_t index = 0; index < ssboInfoRefs.size(); ++index) {
		auto& ssboInfo = *ssboInfoRefs[index];

		declarations += makeStorageBufferStructDeclaration(ssboInfo);

		std::string typeName = storageBufferTypeName(ssboInfo);
		if (ssboInfo.isBuffer()) {
			declarations += makeStorageBufferDeclaration(
				util::EnumValue(shader::SetIndex::ExternalStorage),
				index,
				compute::SSBOAccessMode::ReadOnly,
				ssboInfo.hlslTemplateName(),
				typeName,
				ssboInfo.name() // External ssbos just use them name directly, without "in" or "out".
			);
		}
		else {
			// Combined sampler declaration
			declarations += makeCombinedTextureSamplerDeclaration(
				util::EnumValue(shader::SetIndex::ExternalStorage),
				index,
				ssboInfo.name(),
				false,
				typeName, 
				ssboInfo.textureDimension()
			);
		}
	}
	return declarations;
}

inline std::string AfterglowMaterialAsset::storageBufferTypeName(const AfterglowSSBOInfo& ssboInfo) {
	if (ssboInfo.isBuffer()) {
		return ssboInfo.name() + "Struct";
	}
	else {
		return compute::HLSLTypeName(ssboInfo.textureMode());
	}
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

inline std::string AfterglowMaterialAsset::makeImportAttachmentDeclaration(const AfterglowPassInterface& pass, uint32_t& bindingIndex) const {
	std::string declaration;
	auto& subpassContext = pass.subpassContext();
	for (const auto& importAttachment : pass.importAttachments()) {
		// TODO: depth stencil support.
		auto attachmentType = render::AttachmentType::Color;
		if (subpassContext.isDepthAttachmentIndex(importAttachment.destAttachmentIndex)) {
			attachmentType = render::AttachmentType::Depth;
		}
		declaration += makeCombinedTextureSamplerDeclaration(
			util::EnumValue(shader::SetIndex::Pass),
			bindingIndex,
			importAttachment.attachmentName,
			importAttachment.isMultipleSample,
			render::HLSLTexturePixelTypeName(attachmentType)
		);
		++bindingIndex;
	}
	return declaration;
}

inline std::string AfterglowMaterialAsset::makeInputAttachmentDeclaration(const render::InputAttachmentInfos& inputAttachmentInfos, uint32_t& bindingIndex) const {
	std::string declaration;
	for (const auto& info : inputAttachmentInfos) {
		// Texture sampler method.
		declaration += makeCombinedTextureSamplerDeclaration(
			util::EnumValue(shader::SetIndex::Pass),
			bindingIndex,
			info.name,
			info.isMultiSample,
			render::HLSLTexturePixelTypeName(info.type)
		);
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
				/*img::HLSLPixelTypeName(imageInfo.channels, imageInfo.format),*/
				std::string(enumInfo.name)
			);
		});
	});
	
	return declarations;
}
