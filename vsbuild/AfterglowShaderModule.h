#pragma once

#include <string>
// #include <shaderc/shaderc.hpp>

#include  "ShaderDefinitions.h"
#include "AfterglowDevice.h"

// TODO: Persistence spirv data (make spirv file asset).

class AfterglowShaderModule : public AfterglowProxyObject<AfterglowShaderModule, VkShaderModule, VkShaderModuleCreateInfo>{
public:
	using CodeBytes = std::vector<uint32_t>;
	// @brief: Input hlsl code to compile spirv.
	AfterglowShaderModule(
		AfterglowDevice& device, 
		shader::Stage shaderStage, 
		const std::string& shaderCode, 
		const std::string& shaderName = "Null"
		);
	~AfterglowShaderModule();

	// TODO: Waiting for vulkan extension support.
	//static shaderc::Compiler& compiler();
	//static shaderc::CompileOptions& compileOptions();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	inline void compile(shader::Stage shaderStage, const std::string& shaderCode, const std::string& shaderName = "Null");
	const wchar_t* shaderModelName(shader::Stage shaderStage);

	// shaderc_shader_kind shaderKind(shader::Stage shaderStage);

	AfterglowDevice& _device;
	CodeBytes _bytes;
};

