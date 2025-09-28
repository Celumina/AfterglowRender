#pragma once

#include <thread>

#include "AfterglowWindow.h"
#include "AfterglowMeshResource.h"
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
	void renderLoop();
	void draw();
	void completeSubmission();

	void evaluateRenderable();
	void evaluateComputable();

	void submitMeshUniforms();

	void recordDraws();

	inline bool recordDraw(
		const std::string& materialName, 
		AfterglowMeshResource& meshResource, 
		uint32_t meshIndex
	);
	inline void recordDispatch(const std::string& materialName, const ubo::MeshUniform& meshUniform);

	void updateGlobalUniform();

	struct Context;
	std::unique_ptr<Context> _context;

	AfterglowWindow& _window;
};

