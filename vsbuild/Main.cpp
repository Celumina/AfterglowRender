#include <iostream>
#include <stdexcept>

#include "AfterglowApplication.h"
#include "Inreflect.h"
//
//#define DEBUG_MODE true
//
//#if DEBUG_MODE 
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// #endif

int main() {
	std::setlocale(LC_ALL, "en_US.utf8");
	AfterglowApplication application;

	try {
		application.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}