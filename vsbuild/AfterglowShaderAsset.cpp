#include "AfterglowShaderAsset.h"

#include <fstream>
#include <stdexcept>
#include <filesystem>
#include "DebugUtilities.h"

AfterglowShaderAsset::AfterglowShaderAsset(const std::string& path) {
	if (!std::filesystem::exists(path)) {
		DEBUG_CLASS_ERROR(std::format("Shader file not exists: \"{}\"",  path));
		throw std::runtime_error("[AfterglowShderAsset] Shader file not exists.");
	}
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		DEBUG_CLASS_ERROR(std::format("Failed to load shader file: \"{}\"", path));
		throw std::runtime_error("[AfterglowShderAsset] Failed to load shader file.");
	}
	// std::string(iterator_begin, iterator_end);
	_code = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

const std::string& AfterglowShaderAsset::code() const noexcept {
	return _code;
}
