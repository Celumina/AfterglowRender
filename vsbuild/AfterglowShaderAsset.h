#pragma once

#include <string>
#include <memory>

class AfterglowShaderAsset {
public:
	AfterglowShaderAsset(const std::string& path);

	const std::string& code() const noexcept;

private:
	std::string _code;
};

