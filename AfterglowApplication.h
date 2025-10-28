#pragma once
#include "AfterglowWindow.h"
#include "AfterglowRenderer.h"
#include "AfterglowSystem.h"

class AfterglowApplication {
public:
	AfterglowApplication();
	void run();

private:
	AfterglowWindow _window;
	AfterglowRenderer _renderer;
	AfterglowSystem _system;
};

