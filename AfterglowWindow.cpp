#include "AfterglowWindow.h"

#include <algorithm>


#include "AfterglowInput.h"
#include "Configurations.h"

struct AfterglowWindow::Context {
	Context(GLFWwindow* glfwWindow);

	void updateCursor();

	static void framebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height);
	static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
	static void cursorPosCallback(GLFWwindow* glfwWindow, double posX, double posY);
	static void scrollCallback(GLFWwindow* glfwWindow, double offsetX, double offsetY);
	static void cursorEnterCallback(GLFWwindow* glfwWindow, int entered);

	GLFWwindow* window;
	AfterglowInput input;
	bool shouldLockCursor = false;
	bool shouldUnlockCursor = false;
};

AfterglowWindow::AfterglowWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, cfg::windowResizable);

	linkData(glfwCreateWindow(cfg::windowWidth, cfg::windowHeight, cfg::windowTitle, nullptr,nullptr));
	glfwSetWindowUserPointer(data(), this);

	_context = std::make_unique<Context>(data());
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
	_context->updateCursor();
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
	return _context->input;
}

const AfterglowInput& AfterglowWindow::input() const {
	return _context->input;
}

void AfterglowWindow::lockCursor() {
	// Store lock status and update it in this->update(), 
	// due to these two functions (Lock / UnlockCursor) would be used in other threads.
	_context->shouldLockCursor = true;
}

void AfterglowWindow::unlockCursor() {
	_context->shouldUnlockCursor = true;
}

AfterglowWindow::Context::Context(GLFWwindow* glfwWindow) : window(glfwWindow) {
	glfwSetWindowSizeCallback(window, framebufferResizeCallback);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetCursorEnterCallback(window, cursorEnterCallback);
}

void AfterglowWindow::Context::updateCursor() {
if (shouldLockCursor) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		shouldLockCursor = false;
	}
	if (shouldUnlockCursor) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		shouldUnlockCursor = false;
	}
}

void AfterglowWindow::Context::framebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_resized = true;
}

void AfterglowWindow::Context::keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_context->input.updateKeyFromGLFW(key, action);
}

void AfterglowWindow::Context::mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_context->input.updateMouseButtonFromGLFW(button, action);
}

void AfterglowWindow::Context::cursorPosCallback(GLFWwindow* glfwWindow, double posX, double posY) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_context->input.updateCursorPositionFromGLFW(posX, posY);
}

void AfterglowWindow::Context::scrollCallback(GLFWwindow* glfwWindow, double offsetX, double offsetY) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_context->input.updateScrollFromGLFW(offsetX, offsetY);
}

void AfterglowWindow::Context::cursorEnterCallback(GLFWwindow* glfwWindow, int entered) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_context->input.updateCursorEnteredFromGLFW(entered);
}
