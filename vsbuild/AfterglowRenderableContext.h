#pragma once
#include "AfterglowComponentPool.h"
#include "Inreflect.h"

// @warning: Potential dangling pointer issue if the system doesn't update component after the componentArray reallocation.
struct AfterglowRenderableContext {
	// System Host renderable info.
	AfterglowComponentPool& componentPool;
	AfterglowCameraComponent* camera = nullptr;
	AfterglowDirectionalLightComponent* diectionalLight = nullptr;

};

INR_CLASS(AfterglowRenderableContext) {
	INR_ATTRS (
		// Ref member not support yet:  INR_ATTR(componentPool), 
		INR_ATTR(camera), 
		INR_ATTR(diectionalLight)
	);
};