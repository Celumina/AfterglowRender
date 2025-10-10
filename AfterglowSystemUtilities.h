#pragma once
#include <memory>

#include "AfterglowEntity.h"
#include "AfterglowInput.h"
#include "AfterglowMaterialInstance.h"
#include "UniformBufferObjects.h"

#include "LocalClock.h"

class AfterglowSystem;

// For action entity call system functions.
class AfterglowSystemUtilities {
public:
	friend class AfterglowSystem;
	using TypeIndexArray = std::vector<std::type_index>;

	AfterglowSystemUtilities(AfterglowSystem* system);
	~AfterglowSystemUtilities();

	/* System interfaces */ 
	AfterglowEntity& createEntity(const std::string& name, std::type_index componentTypeIndex, util::OptionalRef<AfterglowEntity> parent = std::nullopt) const;
	AfterglowEntity& createEntity(const std::string& name, const TypeIndexArray& componentTypeIndices, util::OptionalRef<AfterglowEntity> parent = std::nullopt) const;

	bool destroyEntity(AfterglowEntity& entity) const;

	// Input context
	const AfterglowInput& input() const;

	void lockCursor() const;
	void unlockCursor() const;

	/* Material Manager Interfaces */
	// @return: created material name of this asset.
	std::string registerMaterialAsset(const std::string& materialPath) const;
	// @return: created material instance name of this asset.
	std::string registerMaterialInstanceAsset(const std::string& materialInstancePath) const;

	// TODO: Potential thread interference.
	void unregisterMaterialAsset(const std::string& materialPath) const;
	void unregisterMaterialInstanceAsset(const std::string& materialInstancePath) const;

	// @brief: Create material by name, if name exists, replace old material by new one.
	// @return: Material handle;
	// @thread_safety
	AfterglowMaterial& createMaterial(const std::string& name, const AfterglowMaterial& sourceMaterial = AfterglowMaterial::emptyMaterial()) const;

	// @brief: Create materialInstance by name, if name exists, replace old material by new one.
	// @desc: If parent not in this manager, manager will create a empty same named material for the instance.
	// @return: Material Insrtance handle;
	// @thread_safety
	AfterglowMaterialInstance& createMaterialInstance(const std::string& name, const std::string& parentMaterialName) const;

	// @return: Material handle;
	AfterglowMaterial* material(const std::string& name) const;

	// @return: MaterialInstance  handle;
	AfterglowMaterialInstance* materialInstance(const std::string& name) const;

	const ubo::GlobalUniform& globalUniform() const;

	const LocalClock& clock() const;

private: 
	struct Context;
	std::unique_ptr<Context> _context;
};

