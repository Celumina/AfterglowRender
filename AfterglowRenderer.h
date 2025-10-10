#pragma once

#include "AfterglowMaterialManager.h"
#include "AfterglowRenderableContext.h"

// VkInstance justa handle like VkInstance_T*, so it don't need to pass a ref.
class AfterglowRenderer {
public:
	using RenderMaterials = std::map<std::string, const AfterglowMaterial&>;

	AfterglowRenderer(AfterglowWindow& window);
	~AfterglowRenderer();

	AfterglowMaterialManager& materialManager();

	void bindRenderableContext(AfterglowRenderableContext& context);

	void startRenderThread();
	void stopRenderThread();

private:
	struct Context;
	std::unique_ptr<Context> _context;
};

