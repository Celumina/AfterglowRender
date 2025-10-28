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

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;

};

