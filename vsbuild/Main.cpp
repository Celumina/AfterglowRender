#include <iostream>
#include <stdexcept>

#include "AfterglowApplication.h"
#include "DebugUtilities.h"
//
//#define DEBUG_MODE true
//
//#if DEBUG_MODE 
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// #endif

int main() {
	std::setlocale(LC_ALL, "en_US.utf8");

	try {
		AfterglowApplication application;
		application.run();
	}
	catch (const std::exception& error) {
		DEBUG_FATAL(error.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}