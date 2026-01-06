#pragma once

#include <vector>
#include <mutex>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "AfterglowProxyObject.h"


class AfterglowInput;
class AfterglowGUI;

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

	VkExtent2D framebufferSize() const;
	VkExtent2D windowSize() const;

	std::vector<const char*> getRequiredExtensions();

	AfterglowInput& input();
	const AfterglowInput& input() const;

	// void setPresented(bool presented);

	void lockCursor();
	void unlockCursor();

	void bindUI(AfterglowGUI& ui);

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
	AfterglowGUI* _ui = nullptr;

	bool _resized = false;
	// TODO: Not good, try to find a more effective way to keep rendering when the window size is changed.
	// bool _presented = true;
};

template<typename FuncType>
inline void AfterglowWindow::waitIdle(FuncType&& callback) {
	// Handling minimizataion.
	if (!_resized) {
		return;
	}
	// _presented = false;
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
