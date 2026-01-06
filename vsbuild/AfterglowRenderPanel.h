#pragma once

#include <memory>
#include "AfterglowPanel.h"

class AfterglowRenderStatus;

class AfterglowRenderPanel : public AfterglowPanel {
public:
	AfterglowRenderPanel();
	~AfterglowRenderPanel();

	void bindRenderStatus(AfterglowRenderStatus& renderStatus);
	void bindConsole(AfterglowPanel& console);

protected:
	void renderContext() override;
	inline ImVec2 defaultWindowSize() const noexcept override { return { 400.0, 600.0 }; };

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

