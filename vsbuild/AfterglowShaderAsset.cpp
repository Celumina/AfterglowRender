#include "AfterglowShaderAsset.h"

#include <fstream>
#include <filesystem>
#include "ExceptionUtilities.h"

AfterglowShaderAsset::AfterglowShaderAsset(const std::string& path) {
	if (!std::filesystem::exists(path)) {
		EXCEPT_CLASS_RUNTIME(std::format("Shader file not exists: \"{}\"",  path));
	}
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		EXCEPT_CLASS_RUNTIME(std::format("Failed to load shader file: \"{}\"", path));
	}
	// std::string(iterator_begin, iterator_end);
	_code = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

const std::string& AfterglowShaderAsset::code() const noexcept {
	return _code;
}
