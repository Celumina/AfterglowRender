#include "AfterglowRenderer.h"

#include <limits>
#include <imgui.h>

#include "Configurations.h"

#include "AfterglowInstance.h"
#include "AfterglowSurface.h"
#include "AfterglowWindow.h"
#include "AfterglowDebugMessenger.h"
#include "AfterglowComputeQueue.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowPresentQueue.h"
#include "AfterglowFramebufferManager.h"
#include "AfterglowCommandManager.h"
#include "AfterglowMeshManager.h"
#include "AfterglowAssetMonitor.h"
#include "AfterglowMaterialManager.h"
#include "AfterglowRenderStatus.h"
#include "AfterglowRenderableContext.h"
#include "AfterglowGUI.h"
#include "AfterglowInput.h"
#include "AfterglowComputeTask.h"
#include "AfterglowPhysicalDevice.h"
#include "AfterglowMaterialResource.h"
#include "AfterglowShapeMeshResource.h"
#include "AfterglowMaterialUtilities.h"
#include "RenderConfigurations.h"

#include "AfterglowTicker.h"

struct AfterglowRenderer::Impl {
	Impl(AfterglowRenderer& rendererRef, AfterglowWindow& windowRef);

	void renderLoop();
	void draw();
	void completeSubmission();

	void submitMeshUniforms();

	void recordDraws();
	void recordDispatches();

	inline bool recordDraw(
		const std::string& materialName,
		AfterglowMeshResource& meshResource,
		uint32_t meshIndex
	);
	inline bool recordComputeDraw(const std::string& materialName, const ubo::MeshUniform& meshUniform);
	inline void recordDispatch(const std::string& materialName, const ubo::MeshUniform& meshUniform);

	// TODO: move to system thread.
	inline void updateGlobalUniform();
	inline void updateCamera();

	inline glm::vec4 normalizePlane(const glm::vec4& plane) noexcept;
	inline void updateGlobalUniformFrustumPlanes() noexcept;

	AfterglowWindow& window;

	std::unique_ptr<std::jthread> renderThread;

	// TODO: Make it as const, or add mutex in some methods.
	// Source form system, Renderer never modify it, or thread conflect would happen.
	AfterglowRenderableContext* renderableContext = nullptr;

	AfterglowTicker ticker;
	AfterglowAssetMonitor assetMonitor;

	AfterglowInstance::AsElement instance;
	AfterglowDebugMessenger::AsElement debugMessenger;
	AfterglowSurface::AsElement surface;
	AfterglowPhysicalDevice::AsElement physicalDevice;
	AfterglowDevice::AsElement device;
	std::unique_ptr<AfterglowComputeQueue> computeQueue;
	std::unique_ptr<AfterglowGraphicsQueue> graphicsQueue;
	std::unique_ptr<AfterglowPresentQueue> presentQueue;
	AfterglowSwapchain::AsElement swapchain;
	AfterglowRenderPass::AsElement renderPass;
	std::unique_ptr<AfterglowFramebufferManager> framebufferManager;
	std::unique_ptr<AfterglowCommandManager> commandManager;
	std::unique_ptr<AfterglowMeshManager> meshManager;
	std::unique_ptr<AfterglowMaterialManager> materialManager;
	std::unique_ptr<AfterglowSynchronizer> synchronizer;
	std::unique_ptr<AfterglowRenderStatus> renderStatus;
	std::unique_ptr<AfterglowGUI> ui;
};


AfterglowRenderer::AfterglowRenderer(AfterglowWindow& window) : 
	_impl(std::make_unique<Impl>(*this, window)) {
	// TODO: Init commands, e.g. Look up table.
}

AfterglowRenderer::~AfterglowRenderer() {
}

AfterglowMaterialManager& AfterglowRenderer::materialManager() noexcept {
	return *_impl->materialManager;
}

AfterglowGUI& AfterglowRenderer::ui() noexcept {
	return *_impl->ui;
}

void AfterglowRenderer::bindRenderableContext(AfterglowRenderableContext& context)  noexcept {
	_impl->renderableContext = &context;
}

void AfterglowRenderer::startRenderThread() {
	if (!_impl->renderThread) {
		_impl->renderThread = std::make_unique<std::jthread>([&]() { _impl->renderLoop(); });
	}
}

void AfterglowRenderer::stopRenderThread() {
	//_renderThread->request_stop();
	//_renderThread->join();
	// Reset for restart.
	_impl->renderThread.reset();
	// Operations in drawFrame() are asychronous, so we should wait for logical device to finish operations.
	(*_impl->device).waitIdle();
}

AfterglowTicker& AfterglowRenderer::ticker() noexcept {
	return _impl->ticker;
}

const VkPhysicalDeviceProperties& AfterglowRenderer::physicalDeviceProperties() const noexcept {
	return (*_impl->physicalDevice).properties();
}

float AfterglowRenderer::aspectRatio() const noexcept {
	return (*_impl->swapchain).aspectRatio();
}

AfterglowRenderer::Impl::Impl(AfterglowRenderer& rendererRef, AfterglowWindow& windowRef) :
	window(windowRef) {
	// Make sure the same vulkan environment is used in different devices.
	_putenv_s("VK_LAYER_PATH", cfg::layerPath);

	// Monitor render-relatived assets.
	instance.recreate();

	// Setup debug after create VkInstance.
	// This object(AfterglowDebugMessenger) is unused.
	if constexpr (cfg::enableValidationLayers) {
		debugMessenger.recreate(instance);
	}

	surface.recreate(instance, window);
	physicalDevice.recreate(instance, surface);
	device.recreate(physicalDevice);

	// Initialize queues
	computeQueue = std::make_unique<AfterglowComputeQueue>(device);
	graphicsQueue = std::make_unique<AfterglowGraphicsQueue>(device);
	presentQueue = std::make_unique<AfterglowPresentQueue>(device);

	swapchain.recreate(device, window, surface);
	renderPass.recreate(swapchain);

	framebufferManager = std::make_unique<AfterglowFramebufferManager>(renderPass);
	commandManager = std::make_unique<AfterglowCommandManager>(renderPass);
	auto& commandPool = commandManager->commandPool();

	meshManager = std::make_unique<AfterglowMeshManager>(commandPool, *graphicsQueue);

	materialManager = std::make_unique<AfterglowMaterialManager>(
		commandPool, *graphicsQueue, renderPass, assetMonitor
	);

	synchronizer = std::make_unique<AfterglowSynchronizer>(device);

	renderStatus = std::make_unique<AfterglowRenderStatus>(rendererRef);
	ui = std::make_unique<AfterglowGUI>(
		window,
		instance,
		device,
		swapchain,
		*graphicsQueue,
		materialManager->descriptorPool(),
		renderPass
	);
	ui->bindRenderStatus(*renderStatus);
	window.bindUI(*ui);
}

void AfterglowRenderer::Impl::renderLoop() {
	if (!renderableContext) {
		DEBUG_CLASS_ERROR("RenderableContext is not binded.");
		return;
	}

	while (!window.shouldClose()) {
		// DEBUG_INFO(std::format("FPS: {}", clock.fps()));
		draw();
	}
}

void AfterglowRenderer::Impl::draw() {
	// DEBUG_COST_BEGIN("UIContext");
	// Evaluate UI.
	commandManager->recordUIDraw(ui->update());
	// DEBUG_COST_END;

	// TODO: Move to AfterglowSystem.
	// DEBUG_COST_BEGIN("GlobalUBO");
	updateCamera();
	updateGlobalUniform();
	// DEBUG_COST_END;

	// Update and submit mesh uniforms.
	// Components is updated in SystemThread, thus lock it before accessing.
	renderableContext->componentPool.lockedAccess([](auto& renderer){
		renderer.meshManager->updateUniforms(*renderer.renderableContext);
		// Double lock for materialManager and componentPool.
		renderer.materialManager->lockedAccess([](auto& renderer) {
			renderer.submitMeshUniforms();
		}, renderer);
	}, *this);

	// MeshManager calculate mesh uniform only, vertex data fill from materialManager.
	// DEBUG_COST_BEGIN("MaterialManager");
	materialManager->updateMaterials(framebufferManager->imageWriteInfos(), *synchronizer);
	// DEBUG_COST_END;

	// @note: Processing task before waitting GPU as much as possible.

	// Wait is cost, wait it as late as much as possible.
	// DEBUG_COST_BEGIN("WaitGPU");
	synchronizer->wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	synchronizer->wait(AfterglowSynchronizer::FenceFlag::RenderInFlight);
	// DEBUG_COST_END;

	//DEBUG_COST_BEGIN("CompleteSubmission");
	completeSubmission();	/* ---- Here seperating old frameIndex and new frameInedx. ----- */
	//DEBUG_COST_END;

	// Compute Submission: 
	// DEBUG_COST_BEGIN("ComputeContext");
	renderableContext->componentPool.lockedAccess([](auto& renderer) {
		renderer.recordDispatches();
	}, *this);
	materialManager->lockedAccess([](auto& renderer) {
		renderer.commandManager->applyComputeCommands();
	}, * this);

	synchronizer->reset(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	computeQueue->submit(commandManager->computeCommandBuffers(), *synchronizer);
	// DEBUG_COST_END;

	// Render Submission: 
	if (!window.drawable()) {
		// TODO: Optional compute task type for window relativity.
		computeQueue->cancelSemaphore(*synchronizer);
		return;
	}

	int imageIndex = framebufferManager->acquireNextImage(*synchronizer);
	if (imageIndex == AfterglowFramebufferManager::AcquireState::Invalid) {
		// If have not graphics queue respond, cancel the compute queue semaphore.
		computeQueue->cancelSemaphore(*synchronizer);
		return;
	}

	// Lock material manager (System <-> Renderer)
	materialManager->lockedAccess([](auto& renderer, int imageIndex){
		// DEBUG_COST_BEGIN("RecordDraws");
		// Locking for both materialManager and componentPool.
		renderer.renderableContext->componentPool.lockedAccess([](auto& renderer) {
			renderer.recordDraws();
		}, renderer);
		// DEBUG_COST_END;
		// DEBUG_COST_BEGIN("ApplyDraw");
		renderer.commandManager->applyDrawCommands(renderer.framebufferManager->framebuffer(imageIndex));
		// DEBUG_COST_END;
	}, *this, imageIndex);
	
	// DEBUG_COST_BEGIN("SubmitPresent");
	synchronizer->reset(AfterglowSynchronizer::FenceFlag::RenderInFlight);
	graphicsQueue->submit(commandManager->drawCommandBuffers(), *synchronizer);
	presentQueue->submit(window, *framebufferManager, *synchronizer, imageIndex);
	// DEBUG_COST_END;
}

void AfterglowRenderer::Impl::completeSubmission() {
	//DEBUG_COST_BEGIN("UpdateMeshResource");
	renderableContext->componentPool.lockedAccess([](auto& renderer) {
		renderer.meshManager->updateResources(*renderer.renderableContext);
	}, *this);
	//DEBUG_COST_END;

	//DEBUG_COST_BEGIN("UpdateMaterialResource");
	materialManager->updateResources();
	//DEBUG_COST_END;

	//DEBUG_COST_BEGIN("UpdateAssetModitor");
	assetMonitor.update(ticker.clock());
	//DEBUG_COST_END;

	(*device).updateCurrentFrameIndex();
	ticker.tick();
}

void AfterglowRenderer::Impl::submitMeshUniforms() {
	auto& componentPool = renderableContext->componentPool;
	auto& staticMeshes = componentPool.components<AfterglowStaticMeshComponent>();
	// Scene meshes uniform
	for (const auto& staticMesh : staticMeshes) {
		if (!staticMesh.enabled() || !staticMesh.meshResource()) {
			continue;
		}
		auto& materialNames = staticMesh.materialNames();
		for (const auto& [slotID, materialName] : materialNames) {
			materialManager->submitMeshUniform(materialName, staticMesh.meshResource()->meshUniform());
		}
	}
	// Postprocess shape mesh uniform
	auto* postProcess = renderableContext->postProcess;
	if (postProcess && postProcess->enabled() && postProcess->shapeResource()) {
		materialManager->submitMeshUniform(postProcess->postProcessMaterialName(), postProcess->shapeResource()->meshUniform());
	}
	else {
		materialManager->submitMeshUniform(mat::EmptyPostProcessMaterialName(), postProcess->shapeResource()->meshUniform());
	}

	// Submit compute meshes uniform
	// TODO: Here exists redundant mesh uniform with static mesh...
	auto& computeComponents = componentPool.components<AfterglowComputeComponent>();
	for (const auto& computeComponent : computeComponents) {
		if (!computeComponent.enabled()) {
			continue;
		}
		materialManager->submitMeshUniform(computeComponent.computeMaterialName(), computeComponent.meshUniform());
	}
}

void AfterglowRenderer::Impl::recordDraws() {
	auto& componentPool = renderableContext->componentPool;
	auto& staticMeshes = componentPool.components<AfterglowStaticMeshComponent>();
	// Record static meshes
	for (auto& staticMesh : staticMeshes) {
		if (!staticMesh.enabled() || !staticMesh.meshResource()) {
			continue;
		}
		for (uint32_t index = 0; index < staticMesh.meshResource()->indexBuffers().size(); ++index) {
			auto& materialName = staticMesh.materialName(index);
			if (!recordDraw(materialName, *staticMesh.meshResource(), index)) {
				DEBUG_CLASS_ERROR(std::format(
					"[Entity {}] Invalid static mesh material: \"{}\"\
					\nSo the static mesh material was reset to the error material name.", 
					staticMesh.entity().name(), materialName
				));
				staticMesh.setMaterial(mat::ErrorMaterialName(),index);
			}
		}
	}

	// Record internal postProcess shape meshes.
	auto* postProcess = renderableContext->postProcess;
	if (postProcess && postProcess->enabled() && postProcess->shapeResource()) {
		if (!recordDraw(postProcess->postProcessMaterialName(), *postProcess->shapeResource(), 0)) {
			DEBUG_CLASS_ERROR(std::format("Invalid postprocess material: {}\n", postProcess->postProcessMaterialName()));
		}
	}
	else if (postProcess->shapeResource()) {
		recordDraw(mat::EmptyPostProcessMaterialName(), *postProcess->shapeResource(), 0);
	}

	// Record compute meshes' draw.
	auto& computeComponents = renderableContext->componentPool.components<AfterglowComputeComponent>();
	for (const auto& computeComponent : computeComponents) {
		if (!computeComponent.enabled()) {
			continue;
		}
		const auto& materialName = computeComponent.computeMaterialName();
		recordComputeDraw(materialName, computeComponent.meshUniform());
	}
}

void AfterglowRenderer::Impl::recordDispatches() {
	auto& computeComponents = renderableContext->componentPool.components<AfterglowComputeComponent>();
	for (const auto& computeComponent : computeComponents) {
		if (!computeComponent.enabled()) {
			continue;
		}
		const auto& materialName = computeComponent.computeMaterialName();
		const auto* materialInstance = materialManager->materialInstance(materialName);
		const AfterglowMaterial* material = nullptr;
		if (materialInstance) {
			material = &materialInstance->parentMaterial();
		}
		else {
			// ComputeTask from .mat directly.
			material = materialManager->material(materialName);
		}

		if (!material || !material->hasComputeTask()) {
			DEBUG_CLASS_WARNING(std::format(
				"Compute material is not found or its compute task is not initialized: \"{}\"",
				materialName
			));
			continue;
		}

		// Record dispatch
		recordDispatch(materialName, computeComponent.meshUniform());
	}
}

inline bool AfterglowRenderer::Impl::recordDraw (
	const std::string& materialName, 
	AfterglowMeshResource& meshResource, 
	uint32_t meshIndex) {
	
	auto* setRefs = materialManager->descriptorSetReferences(materialName, meshResource.meshUniform());
	auto* matResource = materialManager->materialResource(materialName);
	if (!matResource) {
		DEBUG_CLASS_ERROR(std::format("Material not found: \"{}\"", materialName));
		return false;
	}
	if (!setRefs) {
		DEBUG_CLASS_ERROR("DescriptorSetReferences not found, make sure submit mesh uniform before record draw.");
		return false;
	}

	uint32_t instanceCount = 1;
	AfterglowStorageBuffer* indirectBuffer = nullptr;
	auto& material = matResource->materialLayout().material();
	if (material.hasComputeTask()) {
		instanceCount = material.computeTask().instanceCount();
		indirectBuffer = matResource->indirectStorageBuffer();
	}

	commandManager->recordDraw(
		*matResource, 
		*setRefs, 
		meshResource.vertexBufferHandles()[meshIndex], 
		&*meshResource.indexBuffers()[meshIndex],
		indirectBuffer, 
		instanceCount
	);
	return true;
}

inline bool AfterglowRenderer::Impl::recordComputeDraw(const std::string& materialName, const ubo::MeshUniform& meshUniform) {
	auto* matResource = materialManager->materialResource(materialName);
	if (!matResource) {
		DEBUG_CLASS_ERROR("Compute material was not found: " + materialName + "\n");
		return false;
	}
	auto* storageVertexBuffer = matResource->vertexInputStorageBuffer();
	if (!storageVertexBuffer) {
		return false;
	}
	auto* setRefs = materialManager->descriptorSetReferences(materialName, meshUniform);
	if (!setRefs) {
		DEBUG_CLASS_ERROR("Compute material setReferences were not found: " + materialName + "\n");
		return false;
	}
	auto& computeTask = matResource->materialLayout().material().computeTask();
	// SSBO mesh with index buffer support
	commandManager->recordDraw(
		*matResource,
		*setRefs,
		*computeTask.vertexInputSSBOInfo(),
		*storageVertexBuffer,
		computeTask.indexInputSSBOInfo(),
		matResource->indexInputStorageBuffer(), 
		matResource->indirectStorageBuffer()
	);
	return true;
}

inline void AfterglowRenderer::Impl::recordDispatch(const std::string& materialName, const ubo::MeshUniform& meshUniform) {
	//auto* setRefs = materialManager->computeDescriptorSetReferences(materialName, meshUniform);
	auto* setRefs = materialManager->descriptorSetReferences(materialName, meshUniform);
	if (!setRefs) {
		DEBUG_CLASS_WARNING(std::format("Compute material set references is not found: \"{}\"", materialName));
		return;
	}
	auto* matResource = materialManager->materialResource(materialName);

	auto& computeTask = matResource->materialLayout().material().computeTask();
	auto frameIndex = (*device).currentFrameIndex();
	if (computeTask.queryDispatchable(frameIndex)) {
		commandManager->recordCompute(*matResource, *setRefs);
	}
}

void AfterglowRenderer::Impl::updateGlobalUniform() {
	auto& globalUniform = materialManager->globalUniform();

	auto& camera = *renderableContext->camera;
	// Object-independent matrices.
	globalUniform.view = camera.view();
	globalUniform.projection = camera.perspective();
	globalUniform.viewProjection = globalUniform.projection * globalUniform.view;
	globalUniform.invView = camera.invView();
	globalUniform.invProjection = camera.invPerspective();
	globalUniform.invViewProjection = glm::inverse(globalUniform.viewProjection);

	// Camera check was done before this function.
	auto& cameraTransform = camera.entity().get<AfterglowTransformComponent>();
	globalUniform.cameraPosition = glm::vec4(cameraTransform.globalTranslation(), 1.0);
	globalUniform.cameraVector = glm::vec4(cameraTransform.globalDirection(), 0.0);

	if (renderableContext->diectionalLight) {
		auto& diectionalLight = renderableContext->diectionalLight;
		globalUniform.dirLightColor = glm::vec4(
			(1.0f / 255.0f) * diectionalLight->color().r(),
			(1.0f / 255.0f) * diectionalLight->color().g(),
			(1.0f / 255.0f) * diectionalLight->color().b(),
			diectionalLight->intensity()
		);
		auto& directionalLightTransform = diectionalLight->entity().get<AfterglowTransformComponent>();
		globalUniform.dirLightDirection = glm::vec4(directionalLightTransform.globalDirection(), diectionalLight->intensity());
	}

	globalUniform.screenResolution = glm::vec2((*swapchain).extent().width, (*swapchain).extent().height);
	globalUniform.invScreenResolution = glm::vec2(1.0) / globalUniform.screenResolution;
	globalUniform.cursorPosition = window.input().cursorPosition();
	// Here replace as render clock instead of logic clock (For uniform shader animation).
	globalUniform.time = static_cast<float>(ticker.clock().timeSec());
	globalUniform.deltaTime = static_cast<float>(ticker.clock().deltaTimeSec());
	globalUniform.cameraNear = camera.near();
	globalUniform.cameraFar = camera.far();
	globalUniform.cameraFov = camera.fovY();

	updateGlobalUniformFrustumPlanes();
}

inline void AfterglowRenderer::Impl::updateCamera() {
	auto* camera = renderableContext->camera;
	if (!camera) {
		DEBUG_CLASS_ERROR("Not camera exists in the RenderableContext.");
		throw std::runtime_error("Not camera exists in the RenderableContext.");
	}
	// TODO: bad, thread unsafety, replace it. 
	camera->setAspectRatio((*swapchain).aspectRatio());
	// @deprecated: Move to task thread.
	// camera->updateMatrices();
}

inline glm::vec4 AfterglowRenderer::Impl::normalizePlane(const glm::vec4& plane) noexcept {
	float length = glm::length(glm::vec3{ plane });
	if (length < 1e-6f) {
		return plane;
	}
	return plane / length;
}

inline void AfterglowRenderer::Impl::updateGlobalUniformFrustumPlanes() noexcept {
	auto& globalUniform = materialManager->globalUniform();
	auto& viewProjection = globalUniform.viewProjection;

	// [Gil Gribb & Klaus Hartmann, Fast Extraction of Viewing Frustum Planes from the WorldView - Projection Matrix]
	// Extract planes using normal glm indexing (col-major)

	glm::vec4 row0 = { viewProjection[0][0], viewProjection[1][0], viewProjection[2][0], viewProjection[3][0] };
	glm::vec4 row1 = { viewProjection[0][1], viewProjection[1][1], viewProjection[2][1], viewProjection[3][1] };
	glm::vec4 row2 = { viewProjection[0][2], viewProjection[1][2], viewProjection[2][2], viewProjection[3][2] };
	glm::vec4 row3 = { viewProjection[0][3], viewProjection[1][3], viewProjection[2][3], viewProjection[3][3] };

	globalUniform.frustumPlaneL = normalizePlane(row0 + row3); 

	globalUniform.frustumPlaneR = normalizePlane(-row0 + row3); 

	globalUniform.frustumPlaneB = normalizePlane(row1 + row3); 

	globalUniform.frustumPlaneT = normalizePlane(-row1 + row3); 

	if constexpr (cfg::reverseDepth) {
		globalUniform.frustumPlaneN = normalizePlane(-row2 + row3);

		globalUniform.frustumPlaneF = normalizePlane(row2 + row3);
	}
	else {
		globalUniform.frustumPlaneN = normalizePlane(row2 + row3);

		globalUniform.frustumPlaneF = normalizePlane(-row2 + row3);
	}
}
