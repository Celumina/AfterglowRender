#pragma once
#include "AfterglowPanel.h"
#include <memory>

class AfterglowSystemUtilities;

class AfterglowScenePanel : public AfterglowPanel {
public:
	AfterglowScenePanel();
	~AfterglowScenePanel();

	void bindSystemUtilities(AfterglowSystemUtilities& sysUtils) noexcept;
	void bindConsole(AfterglowPanel& console) noexcept;

protected:
	void renderContext() override;
	inline ImVec2 defaultWindowSize() const noexcept override { return { 800.0, 600.0 }; };

private:
	void updateSceneInfo() noexcept;

	struct Impl;
	std::unique_ptr<Impl> _impl;

};

