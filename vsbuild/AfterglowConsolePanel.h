#pragma once

#include "AfterglowPanel.h"
#include <stdint.h>
#include <deque>

class AfterglowRenderStatus;
class AfterglowSystemUtilities;

class AfterglowConsolePanel : public AfterglowPanel {
public:
	AfterglowConsolePanel();
	~AfterglowConsolePanel();

	void maximumOutputTextCount(uint32_t count) noexcept;
	void bindRenderStatus(AfterglowRenderStatus& renderStatus) noexcept;
	void bindSystemUtilities(AfterglowSystemUtilities& sysUtils) noexcept;

protected:
	void renderContext() override;
	void processMessages(const Messages& messages) override;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};
