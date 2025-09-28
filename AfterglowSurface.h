#pragma once
#include "AfterglowProxyObject.h"
#include "AfterglowWindow.h"
#include "AfterglowInstance.h"

// VkSurfaceKHR is Special that it is no CreateInfo, and required for extern function to create surface.
class AfterglowSurface : public AfterglowProxyObject<AfterglowSurface, VkSurfaceKHR> {
	AFTERGLOW_PROXY_STORAGE_ONLY
public:
	AfterglowSurface(AfterglowInstance& instance, AfterglowWindow& window);
	~AfterglowSurface();

private:
	AfterglowInstance& _instance;
};

