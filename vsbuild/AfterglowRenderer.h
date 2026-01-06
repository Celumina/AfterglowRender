#pragma once

#include <map>
#include <string>
#include <memory>

struct AfterglowRenderableContext;
class AfterglowMaterialManager;
class AfterglowWindow;
class AfterglowMaterial;
class AfterglowTicker;
class AfterglowGUI;

struct VkPhysicalDeviceProperties;

// VkInstance justa handle like VkInstance_T*, so it don't need to pass a ref.
class AfterglowRenderer {
public:
	using RenderMaterials = std::map<std::string, const AfterglowMaterial&>;

	AfterglowRenderer(AfterglowWindow& window);
	~AfterglowRenderer();

	AfterglowMaterialManager& materialManager() noexcept;
	AfterglowGUI& ui() noexcept;

	void bindRenderableContext(AfterglowRenderableContext& context) noexcept;

	void startRenderThread();
	void stopRenderThread();

	AfterglowTicker& ticker() noexcept;
	const VkPhysicalDeviceProperties& physicalDeviceProperties() const noexcept;

	float aspectRatio() const noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

