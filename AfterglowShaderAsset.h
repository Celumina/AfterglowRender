#pragma once

#include <string>
#include <memory>

class AfterglowShaderAsset {
public:
	AfterglowShaderAsset(const std::string& path);

	const std::string& code();

private:
	std::string _code;
};

