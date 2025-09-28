#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "AfterglowProxyObject.h"

#include "AfterglowInput.h"

class AfterglowWindow : public AfterglowProxyObject<AfterglowWindow, GLFWwindow*> {
	AFTERGLOW_PROXY_STORAGE_ONLY
public:
	AfterglowWindow();
	~AfterglowWindow();

	void update();
	bool shouldClose();

	bool resized() const;

	bool drawable();
	void waitIdle();

	// Wait idle with a callback function.
	template<typename FuncType>
	void waitIdle(FuncType&& callback);

	VkExtent2D framebufferSize();

	std::vector<const char*> getRequiredExtensions();

	AfterglowInput& input();
	const AfterglowInput& input() const;

	void lockCursor();
	void unlockCursor();

private:
	static void framebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height);
	static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
	static void cursorPosCallback(GLFWwindow* glfwWindow, double posX, double posY);
	static void scrollCallback(GLFWwindow* glfwWindow, double offsetX, double offsetY);
	static void cursorEnterCallback(GLFWwindow* glfwWindow, int entered);

	AfterglowInput _input;
	bool _resized;
	bool _shouldLockCursor;
	bool _shouldUnlockCursor;
};

template<typename FuncType>
inline void AfterglowWindow::waitIdle(FuncType&& callback) {
	// Handling minimizataion.
	if (!_resized) {
		return;
	}

	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(data(), &width, &height);
	// Wait until new valid size is appear. If not do that, hide window will cause error.
	if (width == 0 || height == 0) {
		return; 
	}
	if constexpr (!std::is_same<std::nullptr_t, FuncType>::value) {
		callback();
		_resized = false;
	}
}
