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

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

