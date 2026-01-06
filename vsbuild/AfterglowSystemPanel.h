#pragma once

#include <memory>
#include "AfterglowPanel.h"

class AfterglowSystemUtilities;

class AfterglowSystemPanel : public AfterglowPanel {
public:
	AfterglowSystemPanel();
	~AfterglowSystemPanel();

	void bindSystemUtilities(AfterglowSystemUtilities& sysUtils);
	void bindConsole(AfterglowPanel& console);

protected: 
	void renderContext() override;
	inline ImVec2 defaultWindowSize() const noexcept override { return { 400.0, 600.0 }; };

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;

};

