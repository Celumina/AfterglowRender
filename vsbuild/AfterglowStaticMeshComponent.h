#pragma once

#include <string>
#include <map>
#include <memory>

#include "AfterglowComponent.h"
#include "AssetDefinitions.h"
#include "AfterglowRenderableComponent.h"

//class AfterglowMeshResource;

class AfterglowStaticMeshComponent : public AfterglowRenderableComponent<AfterglowStaticMeshComponent> {
public:
	// map<SlotID, MaterialInstanceName>
	//using SlotID = uint32_t;
	//using MaterialNames = std::map<SlotID, std::string>;

	//AfterglowStaticMeshComponent();
	//~AfterglowStaticMeshComponent();

	//AfterglowStaticMeshComponent(AfterglowStaticMeshComponent&& rval) noexcept;
	//AfterglowStaticMeshComponent& operator=(AfterglowStaticMeshComponent&& rval) noexcept;

	//AfterglowStaticMeshComponent(const AfterglowStaticMeshComponent& other);
	//AfterglowStaticMeshComponent& operator=(const AfterglowStaticMeshComponent& other);

	//const std::string& materialName(SlotID id = 0) const;
	//const MaterialNames& materialNames() const noexcept;

	const std::string& modelPath() const noexcept;
	model::ImportFlag modelImportFlags() const noexcept;

	const model::AssetInfo& modelAssetInfo() const noexcept;
	
	/**
	* @brief: Set material by name, materials were created in material manager.
	* @desc: 
	* 	Material name or material instance name is allowed.
	* 	If input a material name which is not found in material instance pool, 
	* 	material manager will create a same name instance automatically.
	* @usage: If you want to cancel a material in a slot, give a empty material name.
	*/
	//void setMaterial(const std::string& materialName, SlotID id = 0);
	void setModel(const std::string& modelPath);

	// @brief: For special effect like "Outline".
	//void setOverlayMaterial(const std::string& materialName, SlotID id = 0);

	bool meshDated() const noexcept;
	void setMeshDated(bool isDated) noexcept;

	void addImportFlags(model::ImportFlag flags) noexcept;
	void removeImportFlags(model::ImportFlag flags) noexcept;

	//inline const std::unique_ptr<AfterglowMeshResource>& meshResource() const noexcept { return _meshResource; }
	//inline std::unique_ptr<AfterglowMeshResource>& meshResource() noexcept { return _meshResource; }
	
private:
	model::AssetInfo _assetInfo = {};
	//MaterialNames _materialNames;

	/**
	* @note: Initialize by MeshManager.
	* @see: AfterglowMeshManager.h
	*/
	//std::unique_ptr<AfterglowMeshResource> _meshResource;
	// @note: MeshManager would modify this flag.
	bool _meshDated = false;

	//static inline std::string emptyMaterialName = "";
};

INR_CLASS(AfterglowStaticMeshComponent) {
	INR_BASE_CLASSES<AfterglowRenderableComponent<AfterglowStaticMeshComponent>>;
	INR_FUNCS(
		//INR_FUNC(materialName),
		//INR_FUNC(materialNames),
		INR_FUNC(modelPath),
		INR_FUNC(modelImportFlags),
		INR_FUNC(modelAssetInfo),
		//INR_FUNC(setMaterial), 
		INR_FUNC(setModel),
		// TODO: INR_FUNC(setOverlayMaterial), 
		INR_FUNC(meshDated),
		INR_FUNC(setMeshDated),
		INR_FUNC(addImportFlags),
		INR_FUNC(removeImportFlags)
		//INR_OVERLOADED_FUNC(std::unique_ptr<AfterglowMeshResource>& (AfterglowStaticMeshComponent::*)(), meshResource),
		//INR_OVERLOADED_FUNC(const std::unique_ptr<AfterglowMeshResource>& (AfterglowStaticMeshComponent::*)() const, meshResource)
	);
};