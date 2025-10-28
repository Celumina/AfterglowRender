#pragma once

#include <string>
#include <map>

#include "AfterglowComponent.h"
#include "AssetDefinitions.h"

class AfterglowStaticMeshComponent : public AfterglowComponent<AfterglowStaticMeshComponent> {
public:
	// map<SlotID, MaterialInstanceName>
	using SlotID = uint32_t;
	using MaterialNames = std::map<SlotID, std::string>;

	const std::string& materialName(SlotID id = 0) const;
	const MaterialNames& materialNames() const;

	const std::string& modelPath() const;
	model::ImportFlag modelImportFlags() const;

	const model::AssetInfo& modelAssetInfo() const;
	
	/**
	* @brief: Set material by name, materials were created in material manager.
	* @desc: 
	* 	Material name or material instance name is allowed.
	* 	If input a material name which is not found in material instance pool, 
	* 	material manager will create a same name instance automatically.
	* @usage: If you want to cancel a material in a slot, give a empty material name.
	*/
	void setMaterial(const std::string& materialName, SlotID id = 0);
	void setModel(const std::string& modelPath);

	// TODO: ...
	// @brief: For special effect like "Outline".
	void setOverlayMaterial(const std::string& materialName, SlotID id = 0);

	bool meshDated() const;
	void setMeshDated(bool isDated);

	void addImportFlags(model::ImportFlag flags);
	void removeImportFlags(model::ImportFlag flags);
	
private:
	model::AssetInfo _assetInfo;
	MaterialNames _materialNames;

	bool _meshDated = false;

	static inline std::string emptyMaterialName = "";
};

INR_CLASS(AfterglowStaticMeshComponent) {
	INR_BASE_CLASSES<AfterglowComponent<AfterglowStaticMeshComponent>>;
	INR_FUNCS(
		INR_FUNC(materialName),
		INR_FUNC(materialNames),
		INR_FUNC(modelPath),
		INR_FUNC(modelImportFlags),
		INR_FUNC(modelAssetInfo),
		INR_FUNC(setMaterial), 
		INR_FUNC(setModel),
		// TODO: INR_FUNC(setOverlayMaterial), 
		INR_FUNC(meshDated),
		INR_FUNC(setMeshDated),
		INR_FUNC(addImportFlags),
		INR_FUNC(removeImportFlags)
	);
};