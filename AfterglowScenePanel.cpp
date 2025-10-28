#include "AfterglowScenePanel.h"

#include <stack>

#include "AfterglowSystemUtilities.h"
#include "AfterglowScene.h"
#include "AfterglowComponentRegistery.h"

struct AfterglowScenePanel::Impl {
	Impl(AfterglowScenePanel& owner);

	inline void pushSceneTreeNode(const std::string& name);
	inline void pushSceneTreeNode(AfterglowEntity& entity);
	inline void popSceneTreeNode();
	inline bool shouldShowTreeSubnodes();

	inline void renderSceneHierarchy();
	inline void renderEntityDetails();

	AfterglowScenePanel& owner;
	AfterglowSystemUtilities* sysUtils = nullptr;
	AfterglowPanel* console = nullptr;

	std::weak_ptr<AfterglowScene> scene;
	std::stack<bool> sceneTreeStack;

	AfterglowEntity* activeEntity = nullptr;
	static inline int treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
};

AfterglowScenePanel::AfterglowScenePanel() :
	AfterglowPanel("Scene"), _impl(std::make_unique<Impl>(*this)) {
}

AfterglowScenePanel::~AfterglowScenePanel() {
}

void AfterglowScenePanel::bindSystemUtilities(AfterglowSystemUtilities& sysUtils) noexcept {
	_impl->sysUtils = &sysUtils;
}

void AfterglowScenePanel::bindConsole(AfterglowPanel& console) noexcept {
	_impl->console = &console;
}

void AfterglowScenePanel::renderContext() {
	if (_impl->scene.expired()) {
		updateSceneInfo();
		return;
	}

	ImGui::BeginChild("SceneBorder", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
	static constexpr float defaultColumnProportion = 0.382f;
	float windowWidth = ImGui::GetWindowWidth();

	ImGui::Columns(2, "SceneColumns", true);
	ImGui::SetColumnWidth(0, windowWidth * defaultColumnProportion);
	// Render hierarchy and details.
	_impl->renderSceneHierarchy();
	ImGui::NextColumn();
	_impl->renderEntityDetails();
	ImGui::Columns(1);

	ImGui::EndChild();
}

void AfterglowScenePanel::updateSceneInfo() noexcept {
	_impl->scene = _impl->sysUtils->scene();
	if (!_impl->scene.expired()) {
		setTitle(std::format("Scene {}", _impl->scene.lock()->sceneName()));
	}
}

AfterglowScenePanel::Impl::Impl(AfterglowScenePanel& owner) : owner(owner) {
}

inline void AfterglowScenePanel::Impl::pushSceneTreeNode(const std::string& name) {
	sceneTreeStack.push(ImGui::TreeNodeEx(name.data(), treeNodeFlags));
}

inline void AfterglowScenePanel::Impl::pushSceneTreeNode(AfterglowEntity& entity) {
	int flags = treeNodeFlags;
	if (!scene.lock()->hasChild(entity)) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (&entity == activeEntity) {
		ImGui::PushStyleColor(ImGuiCol_Header, { 0.06f, 0.33f, 0.85f, 0.8f });
		sceneTreeStack.push(ImGui::TreeNodeEx(entity.name().data(), flags));
		ImGui::PopStyleColor();
	}
	else {
		sceneTreeStack.push(ImGui::TreeNodeEx(entity.name().data(), flags));
	}


	if (ImGui::IsItemActivated()) {
		activeEntity = &entity;
		// TODO: Paint Active ComponentInfo
	}
}

inline void AfterglowScenePanel::Impl::popSceneTreeNode() {
	if (sceneTreeStack.top()) {
		ImGui::TreePop();
	}
	sceneTreeStack.pop();
}

inline bool AfterglowScenePanel::Impl::shouldShowTreeSubnodes() {
	return sceneTreeStack.top();
}

inline void AfterglowScenePanel::Impl::renderSceneHierarchy() {
	ImGui::BeginChild("SceneHierarchy", ImGui::GetContentRegionAvail());
	AfterglowEntity* lastEntity = nullptr;
	ImGui::PushStyleColor(ImGuiCol_Header, { 0.0f, 0.0f, 0.0f, 0.0f });
	pushSceneTreeNode("SceneRoot");
	scene.lock()->forEachEntityDFS([this, &lastEntity](AfterglowEntity& entity) {
		// Check if need to pop tree node.
		auto* lastestAncestorEntity = lastEntity;
		while (sceneTreeStack.size() > 1 && entity.parent() != lastestAncestorEntity) {
			lastestAncestorEntity = lastestAncestorEntity->parent();
			popSceneTreeNode();
		}

		if (shouldShowTreeSubnodes()) {
			// Push current tree node.
			pushSceneTreeNode(entity);
		}
		lastEntity = &entity;
		});
	while (!sceneTreeStack.empty()) {
		popSceneTreeNode();
	}
	// _impl->popSceneTreeNode();
	ImGui::PopStyleColor();
	ImGui::EndChild();

}

inline void AfterglowScenePanel::Impl::renderEntityDetails() {
	if (!activeEntity) {
		return;
	}
	ImGui::BeginChild("EntityDetails", ImGui::GetContentRegionAvail());
	ImGui::Text(std::format("EntityName: {}", activeEntity->name()).data());
	ImGui::Text(std::format("EntityID: {}", activeEntity->id()).data());
	

	for (auto& [componentTypeIndex, component] : activeEntity->components()) {
		// owner.renderReflectionContext(component, console);
		reg::AsType(componentTypeIndex, [this, &component]<typename ComponentType>() {
			owner.renderReflectionContext(reinterpret_cast<ComponentType*>(component), console);
		});
	}
	
	ImGui::EndChild();
}
