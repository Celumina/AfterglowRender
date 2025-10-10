#include "AfterglowSurface.h"
#include "AfterglowWindow.h"

AfterglowSurface::AfterglowSurface(AfterglowInstance& instance, AfterglowWindow& window) : 
	_instance(instance) {
	if (glfwCreateWindowSurface(instance, window, nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create window surface.");
	}
}

AfterglowSurface::~AfterglowSurface() {
	destroy(vkDestroySurfaceKHR, _instance, data(), nullptr);
}
