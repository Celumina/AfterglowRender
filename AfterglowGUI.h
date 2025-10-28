#pragma once

#include <memory>
#include <vector>
#include <mutex>

class AfterglowWindow;
class AfterglowInstance;
class AfterglowDevice;
class AfterglowSwapchain;
class AfterglowGraphicsQueue;
class AfterglowDescriptorPool;
class AfterglowRenderPass;

class AfterglowRenderStatus;
class AfterglowSystemUtilities;

struct ImDrawData;

class AfterglowGUI {
public:
	AfterglowGUI(
		AfterglowWindow& window, 
		AfterglowInstance& instance, 
		AfterglowDevice& device, 
		AfterglowSwapchain& swapchain, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowDescriptorPool& descriptorPool,
		AfterglowRenderPass& renderPass
	);
	~AfterglowGUI();

	// @brief: Immediate mode GUI update every frame.
	ImDrawData* update();

	void bindRenderStatus(AfterglowRenderStatus& renderStatus) noexcept;
	void bindSystemUtilities(AfterglowSystemUtilities& sysUtils) noexcept;

	/**
	* @thread_safety callbacks
	*/ 
	void keyCallback(int keycode, int scancode, int action, int mods);
	void mouseButtonCallback(int button, int action, int mods);
	void cursorPosCallback(double x, double y);
	void scrollCallback(double xoffset, double yoffset);
	void cursorEnterCallback(int entered);
	void charCallback(unsigned int c);

private:
	struct Impl;

	// Cache stack to avoid input lost.
	struct KeyCallbackCache {
		int keycode;
		int scancode;
		int action;
		int mods;
	};

	struct MouseButtonCallbackCache {
		int button;
		int action;
		int mods;
	};

	struct CursorPosCallbackCache {
		double x;
		double y;
	};

	struct ScrollCallbackCache {
		double xoffset;
		double yoffset;
	};

	struct EnterCallbackCache {
		int cutsorEntered;
	};

	struct CharCallbackCache {
		unsigned int c;
	};

	void applyInputCallbacks();

	std::unique_ptr<Impl> _impl;
	std::vector<KeyCallbackCache> _keyCaches;
	std::vector<MouseButtonCallbackCache> _mouseButtonCaches;
	std::vector<CursorPosCallbackCache> _cursorPosCaches;
	std::vector<ScrollCallbackCache> _scrollCaches;
	std::vector<EnterCallbackCache> _cursorEnterCaches;
	std::vector<CharCallbackCache> _charCaches;

	// Seems it doesn't required anymore.
	// std::mutex _inputMutex;

	AfterglowWindow& _window;
};

