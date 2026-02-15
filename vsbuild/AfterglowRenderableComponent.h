#pragma once
#include <map>
#include "AfterglowComponent.h"
#include "AfterglowMeshResource.h"
#include "AfterglowPassSetBase.h"

namespace renderable {
	enum class Property : uint32_t {
		Visible = 0,
		VisibleCache = 1, // For Multi-threads
		DynamicCulling = 2,

		EnumCount
	};

	INR_CLASS(Property) {
		INR_ATTRS(
			INR_ENUM(Visible), 
			INR_ENUM(VisibleCache),
			INR_ENUM(DynamicCulling)
		);
	};
}

template<typename DerivedType>
class AfterglowRenderableComponent : public AfterglowComponent<DerivedType> {
public:
	using Component = DerivedType;
	using MaterialID = uint32_t;
	using MaterialNames = std::map<MaterialID, std::string>;

	AfterglowRenderableComponent();

	constexpr inline static auto propertyIndex(renderable::Property property) { return std::underlying_type_t<renderable::Property>(property); }

	// Material interfaces
	const std::string& materialName(uint16_t slotID = 0, uint16_t drawIndex = 0);
	inline const MaterialNames& materialNames() const noexcept { return _materialNames; }

	void setMaterial(const std::string& materialName, uint16_t slotID = 0, uint16_t drawIndex = 0);

	// Mesh resource interfacecs
	inline std::unique_ptr<AfterglowMeshResource>& meshResource() noexcept { return _meshResource; }
	inline const std::unique_ptr<AfterglowMeshResource>& meshResource() const noexcept { return _meshResource; }

	// Draw mesh with different materials.
	inline uint16_t drawCount() const noexcept { return _drawCount; }
	inline void setDrawCount(uint16_t count) noexcept { _drawCount = count; }

	// Instance count for same material batch drawing.
	inline uint32_t instanceCount() const noexcept { return _instanceCount; } 
	inline void setInstanceCount(uint32_t instanceCount) noexcept { _instanceCount = instanceCount; }

	uint8_t property(renderable::Property propertyEnum) const noexcept;
	void setProperty(renderable::Property propertyEnum, uint8_t value) noexcept;

	/**
	* @desc: 
	* 	Load VisibleCache(used in SystemThread) into Visible(used in renderThread).
	*	Ensure value always constance between render thread frame execution.
	*/
	inline void loadVisibleCache() noexcept { setVisible(property(renderable::Property::VisibleCache)); }
	inline bool visible() const noexcept { return property(renderable::Property::Visible); }

	inline bool shouldDraw() const noexcept { return Parent::enabled() && visible() && meshResource(); }

	// @depreated: update by system directly.
	///**
	//* @note: override this CRTP function in derived class.
	//* @return: New visible status.
	//*/ 
	//bool onVisibleUpdate();

	//// Called by system.
	//void updateVisible();

protected:
	constexpr static inline uint16_t materialSlotID(MaterialID materialID) noexcept { return static_cast<uint16_t>(materialID >> 16); }
	constexpr static inline uint16_t drawIndex(MaterialID materialID) noexcept { return static_cast<uint16_t>(materialID); /* truncated to 16 bits */ }
	constexpr static inline MaterialID encodeMaterialID(uint16_t materialSlotID, uint16_t drawIndex) noexcept { return (static_cast<uint32_t>(materialSlotID) << 16) | drawIndex; }

	MaterialNames _materialNames;
	std::unique_ptr<AfterglowMeshResource> _meshResource;
	//std::unique_ptr<AfterglowPassSetBase> _customSubpassSet;
	uint16_t _drawCount = 1;
	uint32_t _instanceCount = 1;

	static inline std::string _emptyMaterialName;

private:
	inline void setVisible(bool visible) { setProperty(renderable::Property::Visible, visible); }
	using Parent = AfterglowComponent<DerivedType>;

};

INR_CRTP_CLASS(AfterglowRenderableComponent, DerivedType) {
	INR_BASE_CLASSES<AfterglowComponent<InreflectDerivedType>>;
	INR_FUNCS(
		INR_FUNC(materialName), 
		INR_FUNC(materialNames), 
		INR_FUNC(setMaterial), 
		INR_OVERLOADED_FUNC(typename std::unique_ptr<AfterglowMeshResource>&(AfterglowRenderableComponent<InreflectDerivedType>::*)() noexcept, meshResource),
		INR_OVERLOADED_FUNC(const typename std::unique_ptr<AfterglowMeshResource>&(AfterglowRenderableComponent<InreflectDerivedType>::*)() const noexcept, meshResource), 
		INR_FUNC(drawCount),
		INR_FUNC(setDrawCount),
		INR_FUNC(instanceCount),
		INR_FUNC(setInstanceCount), 
		INR_FUNC(property), 
		INR_FUNC(setProperty), 
		INR_FUNC(loadVisibleCache),
		INR_FUNC(visible), 
		INR_FUNC(shouldDraw)
	);
};

template<typename DerivedType>
inline AfterglowRenderableComponent<DerivedType>::AfterglowRenderableComponent() {
	setVisible(true);
	setProperty(renderable::Property::VisibleCache, true);
	setProperty(renderable::Property::DynamicCulling, true);
}

template<typename DerivedType>
const std::string& AfterglowRenderableComponent<DerivedType>::materialName(uint16_t slotID, uint16_t drawIndex) {
	if (_materialNames.empty()) {
		return _emptyMaterialName;
	}
	auto iterator = _materialNames.find(encodeMaterialID(slotID, drawIndex));
	if (iterator == _materialNames.end()) {
		// If target material not found, Find first slot for different draw as replece.
		auto firstSlotIterator = _materialNames.find(encodeMaterialID(0, drawIndex));
		// In the worst case, return the first material.
		if (firstSlotIterator == _materialNames.end()) {
			// Here use operator[] instead of .at() to make sure the default name be intialized.
			return _materialNames[encodeMaterialID(0, 0)];
		}
		return firstSlotIterator->second;
	}
	return iterator->second;
}

template<typename DerivedType>
inline void AfterglowRenderableComponent<DerivedType>::setMaterial(const std::string& materialName, uint16_t slotID, uint16_t drawIndex) {
	_materialNames[encodeMaterialID(slotID, drawIndex)] = materialName;
}

template<typename DerivedType>
inline uint8_t AfterglowRenderableComponent<DerivedType>::property(renderable::Property propertyEnum) const noexcept {
	return Parent::_customProperties[propertyIndex(propertyEnum)];
}

template<typename DerivedType>
inline void AfterglowRenderableComponent<DerivedType>::setProperty(renderable::Property propertyEnum, uint8_t value) noexcept {
	Parent::_customProperties[propertyIndex(propertyEnum)] = value;
}

//template<typename DerivedType>
//inline bool AfterglowRenderableComponent<DerivedType>::onVisibleUpdate() {
//	if constexpr (!std::is_same_v<decltype(&Component::onVisibleUpdate), decltype(&AfterglowRenderableComponent::onVisibleUpdate)>) {
//		setVisible(reinterpret_cast<Component*>(this)->onVisibleUpdate());
//	}
//	return visible();
//}
//
//template<typename DerivedType>
//inline void AfterglowRenderableComponent<DerivedType>::updateVisible() {
//	if (!property(renderable::Property::DynamicCulling)) {
//		return;
//	}
//	setVisible(reinterpret_cast<Component*>(this)->onVisibleUpdate());
//}
