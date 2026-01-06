#include "AfterglowShaderModule.h"

#include <codecvt>

// Damn it, my cross platform!!!
#include "AfterglowDxcIncludeHandler.h"
#include <wrl\implements.h>

// TODO: GLSL support.

#include "Configurations.h"

AfterglowShaderModule::AfterglowShaderModule(
	AfterglowDevice& device, 
	shader::Stage shaderStage, 
	const std::string& shaderCode, 
	const std::string& shaderName
	)  :
	_device(device) {

	compile(shaderStage, shaderCode, shaderName);

	//// TODO: Waiting for support
	//shaderc_shader_kind kind = shaderKind(shaderStage);

	//shaderc::SpvCompilationResult compileResult = compiler().CompileGlslToSpv(
	//	shaderCode.data(), 
	//	shaderCode.size(), 
	//	kind, 
	//	shaderName.c_str(), 
	//	compileOptions()
	//);

	//if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success) {
	//	DEBUG_ERROR("Source code:\n" + shaderCode);
	//	throw runtimeError((std::string("Failed to compile shader: \n") + compileResult.GetErrorMessage()).data());
	//}
	//_bytes = {compileResult.begin(), compileResult.end()};
}

AfterglowShaderModule::~AfterglowShaderModule() {
	destroy(vkDestroyShaderModule, _device, data(), nullptr);
}

// TODO: Waiting for vulkan extension support.
//shaderc::Compiler& AfterglowShaderModule::compiler() {
//	static shaderc::Compiler compiler;
//	return compiler;
//}
//
//shaderc::CompileOptions& AfterglowShaderModule::compileOptions() {
//	static shaderc::CompileOptions options;
//	static bool areCompileOptionsConfigured;
//	if (!areCompileOptionsConfigured) {
//		areCompileOptionsConfigured = true;
//		options.SetSourceLanguage(shaderc_source_language_hlsl);
//		options.SetAutoSampledTextures(true);
//	#ifdef _DEBUG
//		options.SetOptimizationLevel(shaderc_optimization_level_zero);
//	#else
//		options.SetOptimizationLevel(shaderc_optimization_level_performance);
//	#endif
//	}
//	return options;
//}

void AfterglowShaderModule::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info().codeSize = _bytes.size() * 4;
	info().pCode = _bytes.data();
}

void AfterglowShaderModule::create() {
	if (vkCreateShaderModule(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create shader module.");
	}
}

void AfterglowShaderModule::compile(shader::Stage shaderStage, const std::string& shaderCode, const std::string& shaderName) {
	Microsoft::WRL::ComPtr<IDxcLibrary> pLibrary;
	Microsoft::WRL::ComPtr<IDxcCompiler> pCompiler;
	DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> pSource;
	pLibrary->CreateBlobWithEncodingFromPinned(shaderCode.data(), shaderCode.size(), CP_UTF8, &pSource);

	auto* targetShaderModelName = shaderModelName(shaderStage);
	auto shaderEntryName = util::ToWstring(cfg::shaderEntryName);

	std::vector<LPCWSTR> args = {
		L"-E", shaderEntryName.data(),
		L"-T", targetShaderModelName,
		L"-spirv",
		L"-fvk-use-dx-layout",

		#ifndef _DEBUG
		L"-O3",  /* Higher performance for release version */
		#endif
	};

	AfterglowDxcIncludeHandler pIncludeHandler;

	Microsoft::WRL::ComPtr<IDxcOperationResult> pResult;
	pCompiler->Compile(
		pSource.Get(),
		util::ToWstring(shaderName).data(),
		shaderEntryName.data(),
		targetShaderModelName, 
		args.data(), static_cast<uint32_t>(args.size()), 
		nullptr, 0, 
		&pIncludeHandler,
		&pResult
	);

	HRESULT status;
	pResult->GetStatus(&status);
	if (FAILED(status)) {
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> pErrors;
		pResult->GetErrorBuffer(&pErrors);
		DEBUG_CLASS_ERROR("Shader Code: \n" + shaderCode);
		DEBUG_CLASS_ERROR("Compilation Info: \n" + std::string(static_cast<const char*>(pErrors->GetBufferPointer()), pErrors->GetBufferSize()));
		throw runtimeError("Shader compilation failed.");
	}

	Microsoft::WRL::ComPtr<IDxcBlob> pBlob;
	pResult->GetResult(&pBlob);
	uint32_t* spirvData = (uint32_t*)pBlob->GetBufferPointer();
	size_t spirvSize = pBlob->GetBufferSize() / sizeof(uint32_t);

	_bytes = {spirvData, spirvData + spirvSize};
}

const wchar_t* AfterglowShaderModule::shaderModelName(shader::Stage shaderStage) {
	switch (shaderStage) {
	case shader::Stage::Vertex:
		return L"vs_6_0";
	case shader::Stage::Fragment:
		return L"ps_6_0";
	case shader::Stage::Compute:
		return L"cs_6_0";
	default: 
		throw runtimeError("Not supported shader stage.");
	}
}

//shaderc_shader_kind AfterglowShaderModule::shaderKind(shader::Stage shaderStage) {
//	switch (shaderStage) {
//	case shader::Stage::Vertex:
//		return shaderc_vertex_shader;
//	case shader::Stage::Fragment:
//		return shaderc_fragment_shader;
//	}
//	throw runtimeError("Not supported shader stage.");
//}
