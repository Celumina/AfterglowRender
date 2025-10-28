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

#include "AfterglowTicker.h"

struct AfterglowRenderer::Impl {
	Impl(AfterglowRenderer& rendererRef, AfterglowWindow& windowRef);

	void renderLoop();
	void draw();
	void completeSubmission();
	
	void evaluateRenderable();
	void evaluateComputable();
	void evaluateUI();

	void submitMeshUniforms();

	void recordDraws();

	inline bool recordDraw(
		const std::string& materialName,
		AfterglowMeshResource& meshResource,
		uint32_t meshIndex
	);
	inline void recordDispatch(const std::string& materialName, const ubo::MeshUniform& meshUniform);

	void updateGlobalUniform();

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

	int32_t ndcMeshIndex = -1;
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

	// Full screen rectangle resource.
	ndcMeshIndex = meshManager->addShapeMesh<shape::NDCRectangle>();
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
	// DEBUG_COST_INFO_BEGIN("UIContext");
	evaluateUI();
	// DEBUG_COST_INFO_END;

	// Wait is cost, wait it as late as possible.
	// DEBUG_COST_INFO_BEGIN("WaitGPU");
	synchronizer->wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	synchronizer->wait(AfterglowSynchronizer::FenceFlag::RenderInFlight);
	// DEBUG_COST_INFO_END;

	// DEBUG_COST_INFO_BEGIN("CompleteSubmission");
	completeSubmission();
	// DEBUG_COST_INFO_END;

	// Compute Submission: 
	// DEBUG_COST_INFO_BEGIN("ComputeContext");
	evaluateComputable();
	commandManager->applyComputeCommands();
	synchronizer->reset(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	computeQueue->submit(commandManager->computeCommandBuffers(), *synchronizer);
	// DEBUG_COST_INFO_END;

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

	// DEBUG_COST_INFO_BEGIN("RenderContext");
	// Lock material manager (System <-> Renderer)
	/* LOCK */ std::unique_lock materialManagerLock{materialManager->mutex()};
	// DEBUG_COST_INFO_BEGIN("Renderable");
	// TODO: A not very large amount of entities would makes render thread slowly...
	evaluateRenderable();
	// DEBUG_COST_INFO_END;
	// DEBUG_COST_INFO_BEGIN("ApplyDraw");
	commandManager->applyDrawCommands(framebufferManager->framebuffer(imageIndex));
	// DEBUG_COST_INFO_END;
	/* UNLOCK */ materialManagerLock.unlock();
	// DEBUG_COST_INFO_END;

	// DEBUG_COST_INFO_BEGIN("SubmitPresent");
	synchronizer->reset(AfterglowSynchronizer::FenceFlag::RenderInFlight);
	graphicsQueue->submit(commandManager->drawCommandBuffers(), *synchronizer);
	presentQueue->submit(window, *framebufferManager, *synchronizer, imageIndex);
	// DEBUG_COST_INFO_END;
}

void AfterglowRenderer::Impl::completeSubmission() {
	assetMonitor.update(ticker.clock());
	(*device).updateCurrentFrameIndex();
	ticker.tick();
}

void AfterglowRenderer::Impl::evaluateRenderable() {
	// Camera
	if (!renderableContext->camera) {
		DEBUG_CLASS_ERROR("Have not camera in the RenderableContext.");
		return;
	}
	// TODO: bad, thread unsafety, replace it. 
	renderableContext->camera->setAspectRatio((*swapchain).aspectRatio());
	renderableContext->camera->updateMatrices();
	
	// PostProcess
	// TODO: Handle post process disable.
	if (renderableContext->postProcess) {
		meshManager->shapeMesh(ndcMeshIndex).materialName = 
			&renderableContext->postProcess->postProcessMaterialName();
	}

	updateGlobalUniform();

	// Lock component pool
	// TODO: Find a elegant way to make sure thread safety.
	// TODO: Replace as a manual mutex lock. 
	auto commandPoolLock = renderableContext->componentPool.lock();

	// Update active meshes and load new meshes.
	// DEBUG_COST_INFO_BEGIN("MeshManager");
	meshManager->update(renderableContext->componentPool, *renderableContext->camera);
	// DEBUG_COST_INFO_END;

	// Must before the material update.
	// DEBUG_COST_INFO_BEGIN("MeshUniform");
	submitMeshUniforms();
	// DEBUG_COST_INFO_END;

	// Update material and submit parameters.
	// TODO: Here cause that compute sync error, if wait over here, that the error would occur.
	// DEBUG_COST_INFO_BEGIN("MaterialManager");
	materialManager->update(framebufferManager->imageWriteInfos(), *synchronizer);
	// DEBUG_COST_INFO_END;

	// avg: 154 μs; -> clear multi vertex support: 112.5 μs;
	// DEBUG_COST_INFO_BEGIN("RecordDraws");
	recordDraws();
	// DEBUG_COST_INFO_END;

	// Unlock commandPool here, always require staticMesh. try to optimize this.
}


void AfterglowRenderer::Impl::evaluateComputable() {
	// Lock component pool (System <-> Render)
	// TODO: Find a elegant way to make sure thread safety.
	// TODO: Replace as a manual mutex lock. 
	auto commandPoolLock = renderableContext->componentPool.lock();
	// MeshManager just calculate mesh uniform, vertex data fill from materialManager.
	// Append drawable mesh uniform from computes

	meshManager->updateComputeMesheInfos(renderableContext->componentPool, *renderableContext->camera);
	// TODO: Try to unlock in here.

	materialManager->updateCompute();

	auto& computeMeshInfos = meshManager->computeMeshInfos();
	for (const auto& [id, computeMeshInfo] : computeMeshInfos) {
		const auto& materialName = *computeMeshInfo.materialName;
		const auto* material = materialManager->material(materialName);
		if (!material || !material->hasComputeTask()) {
			DEBUG_CLASS_WARNING(std::format(
				"Compute material is not found or its compute task is not initialized: \"{}\"",
				materialName
			));
			continue;
		}
		// Submit compute mesh uniform
		materialManager->submitMeshUniform(materialName, computeMeshInfo.uniform);

		// Record dispatch
		recordDispatch(materialName, computeMeshInfo.uniform);
	}
}

void AfterglowRenderer::Impl::evaluateUI() {
	commandManager->recordUIDraw(ui->update());

}

void AfterglowRenderer::Impl::submitMeshUniforms() {
	auto& meshResources = meshManager->meshResources();
	// Scene meshes
	for (const auto& [id, meshResource] : meshResources) {
		auto& staticMesh = meshResource.staticMesh();
		auto& materialNames = staticMesh.materialNames();
		for (const auto& [slotID, materialName] : materialNames) {
			materialManager->submitMeshUniform(materialName, meshResource.meshUniform());
		}
	}
	// Internal shape meshes
	auto& shapeMeshes = meshManager->shapeMeshes();
	for (const auto& shapeMesh : shapeMeshes) {
		// Shape meshes do not evaluate any mesh uniform info, so it just place hold.
		materialManager->submitMeshUniform(*shapeMesh.materialName, shapeMesh.resource.meshUniform());
	}
}

void AfterglowRenderer::Impl::recordDraws() {
	auto& meshResources = meshManager->meshResources();
	
	// Record scene meshes
	for (auto& [id, meshResource] : meshResources) {
		auto& staticMesh = meshResource.staticMesh();
		for (uint32_t index = 0; index < meshResource.indexBuffers().size(); ++index) {
			auto& materialName = staticMesh.materialName(index);
			if (!recordDraw(materialName, meshResource, index)) {
				DEBUG_CLASS_ERROR(std::format(
					"[Entity {}] Invalid static mesh material: \"{}\"\
					\nSo the static mesh material was reset to the error material name.", 
					staticMesh.entity().name(), materialName
				));
				staticMesh.setMaterial(AfterglowMaterialManager::errorMaterialInstanceName(),index);
			}
		}
	}
	// Record internal shape meshes.
	auto& shapeMeshes = meshManager->shapeMeshes();
	for (auto& shapeMesh : shapeMeshes) {
		const auto& materialName = *shapeMesh.materialName;
		if (!recordDraw(materialName, shapeMesh.resource, 0)) {
			DEBUG_CLASS_ERROR("Invalid shape mesh material: " + materialName + "\n");
		}
	}

	// Record compute meshes.
	const auto& computeMeshInfos = meshManager->computeMeshInfos();
	for (const auto& [id, computeMeshInfo] : computeMeshInfos) {
		const auto& materialName = *computeMeshInfo.materialName;
		auto* matResource = materialManager->materialResource(materialName);
		if (!matResource) {
			DEBUG_CLASS_ERROR("Compute material not found: " + materialName + "\n");
			continue;
		}
		auto* buffer = matResource->vertexInputStorageBuffer();
		if (!buffer) {
			continue;
		}
		auto* setRefs = materialManager->descriptorSetReferences(
			materialName, computeMeshInfo.uniform
		);
		// SSBO mesh with index buffer support
		commandManager->recordDraw(
			*matResource, 
			*setRefs,
			*matResource->materialLayout().material().computeTask().vertexInputSSBO(), 
			*buffer
		);
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
	auto& material = matResource->materialLayout().material();
	if (material.hasComputeTask()) {
		instanceCount = material.computeTask().instanceCount();
	}

	commandManager->recordDraw(
		*matResource, 
		*setRefs, 
		meshResource.vertexBufferHandles()[meshIndex], 
		meshResource.indexBuffers()[meshIndex], 
		instanceCount
	);
	return true;
}

inline void AfterglowRenderer::Impl::recordDispatch(const std::string& materialName, const ubo::MeshUniform& meshUniform) {
	auto* setRefs = materialManager->computeDescriptorSetReferences(materialName, meshUniform);
	if (!setRefs) {
		DEBUG_CLASS_WARNING(std::format("Compute material set references is not found: \"{}\"", materialName));
		return;
	}
	auto* matResource = materialManager->materialResource(materialName);

	auto& computeTask = matResource->materialLayout().material().computeTask();
	auto frequency = computeTask.dispatchFrequency();
	auto status = computeTask.dispatchStatus();
	if (frequency == AfterglowComputeTask::DispatchFrequency::Never) {
		// TODO: Prove a manual method to submit compute task.
		return;
	}
	else if (frequency == AfterglowComputeTask::DispatchFrequency::Once 
		&& status == AfterglowComputeTask::DispatchStatus::OnceCompleted) {
		return;
	}
	// To make sure compute shader initialization was completed.
	else if (matResource->materialLayout().initSSBOsInitialized() 
		&& status == AfterglowComputeTask::DispatchStatus::None) {
		computeTask.setDispatchStatus(AfterglowComputeTask::DispatchStatus::Initialized);
	}
	else if (frequency == AfterglowComputeTask::DispatchFrequency::Once 
		&& status == AfterglowComputeTask::DispatchStatus::Initialized) {
		computeTask.setDispatchStatus(AfterglowComputeTask::DispatchStatus::OnceCompleted);
	}
	commandManager->recordCompute(*matResource, *setRefs);
}

void AfterglowRenderer::Impl::updateGlobalUniform() {
	auto& globalUniform = materialManager->globalUniform();

	// Camera check was done before this function.
	auto& cameraTransform = renderableContext->camera->entity().get<AfterglowTransformComponent>();
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
	globalUniform.cameraNear = renderableContext->camera->near();
	globalUniform.cameraFar = renderableContext->camera->far();
}