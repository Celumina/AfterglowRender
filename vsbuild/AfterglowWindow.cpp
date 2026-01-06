#include "AfterglowWindow.h"

#include <algorithm>

#include "AfterglowInput.h"
#include "AfterglowGUI.h"
#include "Configurations.h"

struct AfterglowWindow::Impl {
	Impl(GLFWwindow* glfwWindow);

	void updateCursor();
	
	static void windowResizeCallback(GLFWwindow* glfwWindow, int width, int height);
	static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
	static void cursorPosCallback(GLFWwindow* glfwWindow, double posX, double posY);
	static void scrollCallback(GLFWwindow* glfwWindow, double offsetX, double offsetY);
	static void cursorEnterCallback(GLFWwindow* glfwWindow, int entered);
	static void charCallback(GLFWwindow* glfwWindow, unsigned int chr);

	GLFWwindow* window;
	int32_t windowWidth;
	int32_t windowHeight;
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

	_impl = std::make_unique<Impl>(data());
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
	_impl->updateCursor();
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

VkExtent2D AfterglowWindow::framebufferSize() const {
	int width, height;
	// Get true pixel resolution , instead of screen coordinate.
	// If screen has high DPI, screen coordinate will differ to pixel resolution;
	glfwGetFramebufferSize(_impl->window, &width, &height);

	return VkExtent2D {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};
}

VkExtent2D AfterglowWindow::windowSize() const {
	return VkExtent2D {
		static_cast<uint32_t>(_impl->windowWidth),
		static_cast<uint32_t>(_impl->windowHeight)
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
	return _impl->input;
}

const AfterglowInput& AfterglowWindow::input() const {
	return _impl->input;
}

//void AfterglowWindow::setPresented(bool presented) {
//	_presented = presented;
//}

void AfterglowWindow::lockCursor() {
	// Store lock status and update it in this->update(), 
	// due to these two functions (Lock / UnlockCursor) would be used in other threads.
	_impl->shouldLockCursor = true;
}

void AfterglowWindow::unlockCursor() {
	_impl->shouldUnlockCursor = true;
}

void AfterglowWindow::bindUI(AfterglowGUI& ui) {
	_ui = &ui;
}

AfterglowWindow::Impl::Impl(GLFWwindow* glfwWindow) : window(glfwWindow) {
	glfwSetWindowSizeCallback(window, windowResizeCallback);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetCursorEnterCallback(window, cursorEnterCallback);
	glfwSetCharCallback(window, charCallback);

	glfwGetWindowSize(window, &windowWidth, &windowHeight);
}

void AfterglowWindow::Impl::updateCursor() {
if (shouldLockCursor) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		shouldLockCursor = false;
	}
	if (shouldUnlockCursor) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		shouldUnlockCursor = false;
	}
}

void AfterglowWindow::Impl::windowResizeCallback(GLFWwindow* glfwWindow, int width, int height) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	glfwGetWindowSize(glfwWindow, &window->_impl->windowWidth, &window->_impl->windowHeight);	
	window->_resized = true;

	// Wait until present submit successfully, see AfterglowPresentQueue.cpp
	// while (!window->_presented) {}
}

void AfterglowWindow::Impl::keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_impl->input.updateKeyFromGLFW(key, action);
	window->_ui->keyCallback(key, scancode, action, mods);
}

void AfterglowWindow::Impl::mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_impl->input.updateMouseButtonFromGLFW(button, action);
	window->_ui->mouseButtonCallback(button, action, mods);
}

void AfterglowWindow::Impl::cursorPosCallback(GLFWwindow* glfwWindow, double posX, double posY) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_impl->input.updateCursorPositionFromGLFW(posX, posY);
	window->_ui->cursorPosCallback(posX, posY);
}

void AfterglowWindow::Impl::scrollCallback(GLFWwindow* glfwWindow, double offsetX, double offsetY) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_impl->input.updateScrollFromGLFW(offsetX, offsetY);
	window->_ui->scrollCallback(offsetX, offsetY);
}

void AfterglowWindow::Impl::cursorEnterCallback(GLFWwindow* glfwWindow, int entered) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_impl->input.updateCursorEnteredFromGLFW(entered);
	window->_ui->cursorEnterCallback(entered);
}

void AfterglowWindow::Impl::charCallback(GLFWwindow* glfwWindow, unsigned int chr) {
	AfterglowWindow* window = reinterpret_cast<AfterglowWindow*>(glfwGetWindowUserPointer(glfwWindow));
	window->_ui->charCallback(chr);
}
