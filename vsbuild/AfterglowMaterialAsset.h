#pragma once

#include <memory>
#include <format>

#include "AfterglowShaderAsset.h"
#include "AfterglowPassInterface.h"
#include "VertexStructs.h"
#include "ShaderDefinitions.h"
#include "ComputeDefinitions.h"
#include "UniformBufferObjects.h"
#include "AfterglowUtilities.h"
#include "ExceptionUtilities.h"

class AfterglowMaterial;
class AfterglowStructLayout;
class AfterglowSSBOInfo;

class AfterglowMaterialAsset {
public:
	using ShaderDeclarations = std::unordered_map<shader::Stage, std::string>;
	using ShaderAssets = std::unordered_map<shader::Stage, AfterglowShaderAsset>;

	AfterglowMaterialAsset(const std::string& path);
	AfterglowMaterialAsset(AfterglowMaterialAsset&& rval) noexcept;

	/**
	* @desc: 
	*	This constructor format doesn't need to load material asset from file, 
	*	but it still load shader assets.
	*/ 
	AfterglowMaterialAsset(const AfterglowMaterial& material) noexcept;

	~AfterglowMaterialAsset() noexcept;

	const AfterglowMaterial& material() const noexcept;

	// @desc: 
	// If failed to get material name or asset create from AfterglowMaterial, return empty str "".
	std::string materialName() const;

	const std::string& shaderDeclaration(shader::Stage shaderStage) const;
	const std::string& shaderBody(shader::Stage shaderStage) const;

	// TODO: Overload by shared storage buffer pool
	std::string generateShaderCode(
		shader::Stage shaderStage, 
		util::OptionalRef<AfterglowPassInterface> pass = std::nullopt, 
		util::OptionalRef<std::vector<const AfterglowSSBOInfo*>> externalSSBOInfoRefs = std::nullopt
	) const;

	// TODO: Further to impl.
	// static AfterglowMaterialAsset  serialize(const AfterglowMaterial& material);
	// bool write(const std::string& path);
	 
private:
	template<typename DeclarationType>
	using StageDeclarations = std::map<shader::Stage, DeclarationType>;

	struct UniformMemberDeclaration {
		std::string declaration;
	};

	struct TextureDeclaration {
		uint32_t BindingEndIndex = 0;
		std::string declaration;
	};

	struct StorageBufferDeclaration {
		uint32_t BindingEndIndex = 0;
		std::string declaration;
	};

	template<size_t index = 0>
	std::string vertexInputStructDeclaration(std::type_index vertexTypeIndex);

	// TODO: Serializer
	// TODO: Temporary solution here, use GUI solution in future.
	void initMaterial();
	void initMaterialComputeTask();

	inline void initMaterialStencilInfo(std::string_view srcFaceName, render::StencilInfo& dstStencilInfo);

	void loadShaderAssets();

	// HLSL shader declarations.
	void parseShaderDeclarations();
	inline void fillUniformMemberDeclarations(StageDeclarations<UniformMemberDeclaration>& destMemberDeclarations);
	inline void fillTextureDeclarations(StageDeclarations<TextureDeclaration>& destTextureDeclarations);
	inline void fillStorageBufferDeclarations(
		StageDeclarations<StorageBufferDeclaration>& destStorageBufferDeclarations, 
		const StageDeclarations<TextureDeclaration>& textureDeclarations
	);
	
	// @param bindingIndex: This bindingIndex will be accumulated by attachments.
	inline std::string makeImportAttachmentDeclaration(const AfterglowPassInterface& pass, uint32_t& bindingIndex) const;
	inline std::string makeInputAttachmentDeclaration(const render::InputAttachmentInfos& inputAttachmentInfos, uint32_t& bindingIndex) const;

	template<typename UniformType>
	static inline std::string makeUniformMemberDeclarationContext();

	static inline std::string makeUniformStructDeclaration(
		const std::string& structName, const std::string& memberContext, shader::SetIndex shaderSet, uint32_t bindingIndex
	);

	static inline std::string makeCombinedTextureSamplerDeclaration(
		uint32_t setIndex, 
		uint32_t bindingIndex, 
		const std::string& name, 
		bool isMultiSample = false, 
		const std::string& typeName = "",  /* For Texture2DMS only. */
		compute::SSBOTextureDimension textureDimension = compute::SSBOTextureDimension::Texture2D
	);

	static inline std::string makeStorageBufferDeclaration(
		uint32_t setIndex,
		uint32_t bindingIndex,
		compute::SSBOAccessMode accessMode,
		const std::string& templateName,
		const std::string& typeName,
		const std::string& storageBufferName
	);

	inline std::string makeStorageBufferStructDeclarations();
	static inline std::string makeStorageBufferStructDeclaration(const AfterglowSSBOInfo& ssboInfo);
	static inline std::string makeComputeExternalSSBODeclarations(const std::vector<const AfterglowSSBOInfo*> ssboInfoRefs);
	static inline std::string storageBufferTypeName(const AfterglowSSBOInfo& ssboInfo);

	template<typename VertexType>
	inline std::string vertexInputStructDeclaration();
	// TODO: Also remember to generate its Shader VertexInput Declarations.
	inline std::string vertexInputStructDeclaration(const AfterglowStructLayout& structLayout);

	template <typename VertexType, typename VertexAttributeType>
	static void addVertexInputStructMemberDeclarations(std::string& dest);

	inline std::string makeGlobalCombinedTextureSamplerDeclarations();

	template<size_t tupleIndex = 0>
	static inline std::type_index vertexTypeIndex(uint32_t index);

	struct Impl;
	std::unique_ptr<Impl> _impl;
};

template<size_t index>
inline std::string AfterglowMaterialAsset::vertexInputStructDeclaration(std::type_index vertexTypeIndex) {
	if constexpr (index < std::tuple_size_v<vert::RegisteredVertexTypes>) {
		using Vertex = std::tuple_element_t<index, vert::RegisteredVertexTypes>;
		if (util::TypeIndex<Vertex>() == vertexTypeIndex) {
			return vertexInputStructDeclaration<Vertex>();
		}
		return vertexInputStructDeclaration<index + 1>(vertexTypeIndex);
	}
	else {
		EXCEPT_CLASS_RUNTIME("Not suitable vertex type, check type index value is from vertex type and the vertex type is registered.");
	}
}

template<typename UniformType>
inline std::string AfterglowMaterialAsset::makeUniformMemberDeclarationContext() {
	std::string declaration;
	// Reflection!
	Inreflect<UniformType>::forEachAttribute([&declaration](auto typeInfo){
		declaration += std::format("\t{} {};\n", ubo::HLSLTypeName<decltype(typeInfo)::Attribute>(), typeInfo.name);
	});
	return declaration;
}

template<typename VertexType>
inline std::string AfterglowMaterialAsset::vertexInputStructDeclaration() {
	std::string declaration;
	declaration += "struct VSInput {\n";
	addVertexInputStructMemberDeclarations<VertexType, typename VertexType::First>(declaration);
	// Fixed InstanceID declaration
	declaration += "uint instanceID : SV_InstanceID;\n";
	declaration += "};\n";
	return declaration;
}

template<typename VertexType, typename VertexAttributeType>
inline void AfterglowMaterialAsset::addVertexInputStructMemberDeclarations(std::string& dest) {
	dest += std::format(
		"[[vk::location({})]] {} {} : {};\n", 
		VertexType::template index<VertexAttributeType>(),
		std::format("float{}", VertexAttributeType::numComponents), 
		VertexAttributeType::name, 
		util::UpperCase(VertexAttributeType::name)
	);
	if constexpr (!std::is_same_v<typename VertexType::template Next<VertexAttributeType>, typename VertexType::Empty>) {
		addVertexInputStructMemberDeclarations<VertexType, typename VertexType::template Next<VertexAttributeType>>(dest);
	}
}

template<size_t tupleIndex>
inline std::type_index AfterglowMaterialAsset::vertexTypeIndex(uint32_t index) {
	if constexpr (tupleIndex < std::tuple_size_v<vert::RegisteredVertexTypes>) {
		using Vertex = std::tuple_element_t<tupleIndex, vert::RegisteredVertexTypes>;
		if (index == tupleIndex) {
			return util::TypeIndex<Vertex>();
		}
		return vertexTypeIndex<tupleIndex + 1>(index);
	}
	else {
		EXCEPT_TYPE_INVALID_ARG(AfterglowMaterialAsset, "Invalid index parameter, can not cast it to vertexTypeID.");
	}
}
