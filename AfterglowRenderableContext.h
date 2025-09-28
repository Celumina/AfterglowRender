#pragma once
#include "AfterglowComponentPool.h"
#include "Inreflect.h"

struct AfterglowRenderableContext {
	// System renderable info.
	AfterglowComponentPool& componentPool;
	AfterglowCameraComponent* camera;
	AfterglowDirectionalLightComponent* diectionalLight;
	AfterglowPostProcessComponent* postProcess;
};

INR_CLASS(AfterglowRenderableContext) {
	INR_ATTRS (
		// Ref member not support yet:  INR_ATTR(componentPool), 
		INR_ATTR(camera), 
		INR_ATTR(diectionalLight),
		INR_ATTR(postProcess)
	);
};