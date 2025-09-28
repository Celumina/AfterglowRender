#include "AfterglowWindow.h"

#include <algorithm>

#include "Configurations.h"

AfterglowWindow::AfterglowWindow() : 
	_input(), _resized(false), _shouldLockCursor(false), _shouldUnlockCursor(false) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, cfg::windowResizable);

	linkData(glfwCreateWindow(cfg::windowWidth, cfg::windowHeight, cfg::windowTitle, nullptr,nullptr));
	glfwSetWindowUserPointer(data(), this);
	glfwSetWindowSizeCallback(data(), framebufferResizeCallback);

	glfwSetKeyCallback(data(), keyCallback);
	glfwSetMouseButtonCallback(data(), mouseButtonCallback);
	glfwSetCursorPosCallback(data(), cursorPosCallback);
	glfwSetScrollCallback(data(), scrollCallback);
	glfwSetCursorEnterCallback(data(), cursorEnterCallback);
}

AfterglowWindow::~AfterglowWindow() {
	destroy(glfwDestroyWindow, data());
	glfwTerminate();
}

void AfterglowWindow::update() {
	glfwPollEvents();
	// This function will block main thread if window is minimization.
	// Saving resource but block logic.
	waitIdle();

	if (_shouldLockCursor) {
		glfwSetInputMode(data(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		_shouldLockCursor = false;
	}
	if (_shouldUnlockCursor) {
		glfwSetInputMode(data(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		_shouldUnlockCursor = false;
	}
}

bool AfterglowWindow::shouldClose() {
	return glfwWindowShouldClose(data());
}

bool AfterglowWindow::resized() const {
	return _resized;
}

bool AfterglowWindow::drawable() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(data(), &width, &height);
	return width && height;
}

void AfterglowWindow::waitIdle() {
	waitIdle(nullptr);
}

VkExtent2D AfterglowWindow::framebufferSize() {
	int width, height;
	// Get true pixel resolution , instead of screen coordinate.
	// If screen has high DPI, screen coordinate will differ to pixel resolution;
	glfwGetFramebufferSize(*this, &width, &height);

	return VkExtent2D {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
	};
}

std::vector<const char*> AfterglowWindow::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = nullptr;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Range constructor
	// glfwExtensions: begin of array
	// glfwExtensions + glfwExtensionCount: end of array, use pointer operation.
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if constexpr (cfg::enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

AfterglowInput& AfterglowWindow::input() {
	return _input;
}

const AfterglowInput& AfterglowWindow::input() const {
	return _input;
}

void AfterglowWindow::lockCursor() {
	// Store lock status and update it in this->update(), 
	// due to these two functions (Lock / UnlockCursor) would be used in other threads.
	_shouldLockCursor = true;
}

void AfterglowWindow::unlockCursor() {
	_shouldUnlockCursor = true;
}

void AfterglowWindow::framebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_resized = true;
}

void AfterglowWindow::keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_input.updateKeyFromGLFW(key, action);
}

void AfterglowWindow::mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_input.updateMouseButtonFromGLFW(button, action);
}

void AfterglowWindow::cursorPosCallback(GLFWwindow* glfwWindow, double posX, double posY) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_input.updateCursorPositionFromGLFW(posX, posY);
}

void AfterglowWindow::scrollCallback(GLFWwindow* glfwWindow, double offsetX, double offsetY) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_input.updateScrollFromGLFW(offsetX, offsetY);
}

void AfterglowWindow::cursorEnterCallback(GLFWwindow* glfwWindow, int entered) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_input.updateCursorEnteredFromGLFW(entered);
}
