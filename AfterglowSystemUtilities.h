#pragma once
#include <memory>

#include "AfterglowUtilities.h"
#include "UniformBufferObjects.h"
#include "Inreflect.h"

class AfterglowSystem;

class LocalClock;
class AfterglowMaterialInstance;
class AfterglowInput;
class AfterglowEntity;
class AfterglowScene;
class AfterglowMaterial;

// For action entity call system functions.
// TODO: If invoke from RenderThread (GUI), pick up a lock (Or handle it in GUI impl).
// And Material Thing just call from renderthread, safe.
class AfterglowSystemUtilities {
public:
	friend class AfterglowSystem;
	using TypeIndexArray = std::vector<std::type_index>;

	AfterglowSystemUtilities(AfterglowSystem* system);
	~AfterglowSystemUtilities();

	/* System interfaces */ 
	// @brief: return the active scene of system.
	std::weak_ptr<AfterglowScene> scene() const noexcept;

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
	AfterglowMaterial& createMaterial(const std::string& name, util::OptionalRef<AfterglowMaterial> sourceMaterial = std::nullopt) const;

	// @brief: Create materialInstance by name, if name exists, replace old material by new one.
	// @desc: If parent not in this manager, manager will create a empty same named material for the instance.
	// @return: Material Insrtance handle;
	// @thread_safety
	AfterglowMaterialInstance& createMaterialInstance(const std::string& name, const std::string& parentMaterialName) const;

	// @return: Material handle;
	AfterglowMaterial* material(const std::string& name) const;

	// @return: MaterialInstance  handle;
	AfterglowMaterialInstance* materialInstance(const std::string& name) const;

	const ubo::GlobalUniform& globalUniform() const noexcept;

	// Ticker methods
	double fps() const noexcept;
	double timeSec()  const noexcept;
	double deltaTimeSec() const noexcept;
	float maximumFPS() const noexcept;

	void setMaximumFPS(float fps) noexcept;

private: 
	struct Impl;
	std::unique_ptr<Impl> _impl;
};


INR_CLASS(AfterglowSystemUtilities) {
	INR_FUNCS (
		// INR_FUNC(scene), 
		INR_OVERLOADED_FUNC(AfterglowEntity&(AfterglowSystemUtilities::*)(const std::string&, std::type_index, util::OptionalRef<AfterglowEntity>) const, createEntity),
		INR_OVERLOADED_FUNC(AfterglowEntity&(AfterglowSystemUtilities::*)(const std::string&, const AfterglowSystemUtilities::TypeIndexArray&, util::OptionalRef<AfterglowEntity>) const, createEntity),
		INR_FUNC(destroyEntity), 
		// INR_FUNC(input),
		INR_FUNC(lockCursor), 
		INR_FUNC(unlockCursor), 
		INR_FUNC(registerMaterialAsset),
		INR_FUNC(registerMaterialInstanceAsset),
		INR_FUNC(unregisterMaterialAsset),
		INR_FUNC(unregisterMaterialInstanceAsset),
		INR_FUNC(createMaterial),
		INR_FUNC(createMaterialInstance),
		INR_FUNC(material),
		INR_FUNC(materialInstance),
		// INR_FUNC(globalUniform),
		INR_FUNC(fps), 
		INR_FUNC(timeSec),
		INR_FUNC(deltaTimeSec),
		INR_FUNC(maximumFPS),
		INR_FUNC(setMaximumFPS)
	);
};