#pragma once
#include "AfterglowObject.h"
#include "AfterglowSharedMeshPool.h"
#include "AfterglowMeshResource.h"
#include "AfterglowShape.h"

#include "AfterglowComponentBase.h"

struct AfterglowRenderableContext;
class AfterglowTransformComponent;
class AfterglowComputeComponent;
class AfterglowCameraComponent;
class AfterglowStaticMeshComponent;

class AfterglowMeshManager : public AfterglowObject {
public:
	AfterglowMeshManager(
		AfterglowCommandPool& commandPool, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowSynchronizer& synchronizer
	);

	AfterglowDevice& device() noexcept;

	void updateUniforms(AfterglowRenderableContext& renderableContext);

	// @warning: Make sure invoke it after the GPU draw was completed.
	void updateResources(AfterglowRenderableContext& renderableContext);

private:
	static inline void fillMeshUniform(const AfterglowTransformComponent& transform, ubo::MeshUniform& destMeshUnifrom);
	static inline void fillMeshUniformAABB(AfterglowMeshResource& resource);
	
	/*
	* @param staticMesh: dest static mesh component.
	* @param compute: optional dest compute component.
	*/
	void updateMeshUniformResourceInfo(AfterglowStaticMeshComponent& staticMesh, AfterglowComputeComponent* compute = nullptr);

	AfterglowSharedMeshPool _meshPool;
};
