#include "AfterglowGUI.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "AfterglowWindow.h"
#include "AfterglowInstance.h"
#include "AfterglowRenderPass.h"
#include "AfterglowSwapchain.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowDescriptorPool.h"
#include "AfterglowPhysicalDevice.h"
#include "AfterglowConsolePanel.h"
#include "GlobalAssets.h"
#include "Configurations.h"
#include "DebugUtilities.h"


struct AfterglowGUI::Impl {
	Impl(ImGui_ImplVulkan_InitInfo&& initInfo);
	~Impl();

	void showConsolePanel();

	ImGui_ImplVulkan_InitInfo vulkanInitInfo;
	ImFontConfig fontConfig;

	AfterglowConsolePanel consolePanel;
};

AfterglowGUI::Impl::Impl(ImGui_ImplVulkan_InitInfo&& initInfo) :
	vulkanInitInfo(initInfo) {

}

AfterglowGUI::Impl::~Impl() {
}

void AfterglowGUI::Impl::showConsolePanel() {
	if (ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false)) {
		*consolePanel.isOpenHandle() = !consolePanel.isOpen();
	}
	consolePanel.show();
}

AfterglowGUI::AfterglowGUI(AfterglowWindow& window) : _window(window) {
}

AfterglowGUI::~AfterglowGUI() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

ImDrawData* AfterglowGUI::update() {
	applyInputCallbacks();

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	// Insert GUI context here
	//static bool showDemoWindow = true;
	//ImGui::ShowDemoWindow(&showDemoWindow);

	_impl->showConsolePanel();

	ImGui::Render();
	return ImGui::GetDrawData();
}

void AfterglowGUI::bindRenderContext(
	AfterglowInstance& instance, 
	AfterglowDevice& device, 
	AfterglowSwapchain& swapchain, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowDescriptorPool& descriptorPool, 
	AfterglowRenderPass& renderPass, 
	AfterglowRenderStatus& renderStatus
) {
	_impl = std::make_unique<Impl>(ImGui_ImplVulkan_InitInfo{
		.ApiVersion = cfg::apiVersion,
		.Instance = instance,
		.PhysicalDevice = device.physicalDevice(),
		.Device = device,
		.QueueFamily = device.physicalDevice().graphicsFamilyIndex(),
		.Queue = graphicsQueue,
		.DescriptorPool = descriptorPool,
		.MinImageCount = cfg::maxFrameInFlight,
		.ImageCount = static_cast<uint32_t>(swapchain.images().size()),
		.RenderPass = renderPass,
		.Subpass = renderPass.subpassContext().subpassIndex(std::string(inreflect::EnumName(render::Domain::UserInterface))),
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT // In UserInterface domain, attachment had been resolved.
	});
	_impl->consolePanel.bindRenderStatus(renderStatus);
	if (_sysUtils) {
		_impl->consolePanel.bindSystemUtilities(*_sysUtils);
	}

	// Initialize ImGUI
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Modify UI style here:
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::StyleColorsLight();
	style.ScaleAllSizes(1.2f);
	style.Colors[ImGuiCol_WindowBg] = { 1.0f, 1.0f, 1.0f, 0.8f };
	style.WindowPadding = { 16.0f, 16.0f };
	style.WindowRounding = 16.0f;
	style.WindowTitleAlign = { 0.5f, 0.5f };
	style.ChildRounding = 12.0f;
	style.PopupRounding = 16.0f;
	style.FramePadding = { 8.0f, 5.0f };
	style.FrameRounding = 16.0f;
	style.GrabRounding = 12.0f;

	// Default font
	ImFont* font = io.Fonts->AddFontFromFileTTF(
		font::defaultFontPath,
		14.0f,
		&_impl->fontConfig,
		0
	);
	if (font) {
		io.FontDefault = font;
	}
	ImGui_ImplGlfw_InitForVulkan(_window, false);
	ImGui_ImplVulkan_Init(&_impl->vulkanInitInfo);
}

void AfterglowGUI::bindSystemUtilities(AfterglowSystemUtilities& sysUtils) noexcept {
	_sysUtils = &sysUtils;
	if (_impl) {
		_impl->consolePanel.bindSystemUtilities(sysUtils);
	}
}

void AfterglowGUI::keyCallback(int keycode, int scancode, int action, int mods) {
	 std::lock_guard inputLock{ _inputMutex };
	_keyCaches.emplace_back(keycode, scancode, action, mods);
}

void AfterglowGUI::mouseButtonCallback(int button, int action, int mods) {
	 std::lock_guard inputLock{ _inputMutex };
	_mouseButtonCaches.emplace_back(button, action, mods);
}

void AfterglowGUI::cursorPosCallback(double x, double y) {
	 std::lock_guard inputLock{ _inputMutex };
	_cursorPosCaches.emplace_back(x, y);
}

void AfterglowGUI::scrollCallback(double xoffset, double yoffset) {
	 std::lock_guard inputLock{ _inputMutex };
	_scrollCaches.emplace_back(xoffset, yoffset);
}

void AfterglowGUI::cursorEnterCallback(int entered) {
	 std::lock_guard inputLock{ _inputMutex };
	_cursorEnterCaches.emplace_back(entered);
}

void AfterglowGUI::charCallback(unsigned int c) {
	 std::lock_guard inputLock{ _inputMutex };
	_charCaches.emplace_back(c);
}

void AfterglowGUI::applyInputCallbacks() {
	// To prevent the thread interference with winow, apply callbacks in the same thread.
	std::lock_guard inputLock{ _inputMutex };

	for (const auto& cache : _keyCaches) {
		ImGui_ImplGlfw_KeyCallback(_window, cache.keycode, cache.scancode, cache.action, cache.mods);
	}
	_keyCaches.clear();

	for (const auto& cache : _mouseButtonCaches) {
		ImGui_ImplGlfw_MouseButtonCallback(_window, cache.button, cache.action, cache.mods);
	}
	_mouseButtonCaches.clear();

	for (const auto& cache : _cursorPosCaches) {
		ImGui_ImplGlfw_CursorPosCallback(_window, cache.x, cache.y);
	}
	_cursorPosCaches.clear();

	for (const auto& cache : _scrollCaches) {
		ImGui_ImplGlfw_ScrollCallback(_window, cache.xoffset, cache.yoffset);
	}
	_scrollCaches.clear();

	for (const auto& cache : _cursorEnterCaches) {
		ImGui_ImplGlfw_CursorEnterCallback(_window, cache.cutsorEntered);
	}
	_cursorEnterCaches.clear();

	for (const auto& cache : _charCaches) {
		ImGui_ImplGlfw_CharCallback(_window, cache.c);
	}
	_charCaches.clear();
}
