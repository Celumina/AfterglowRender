#pragma once
#include "ShaderDefinitions.h"
#include "ComputeDefinitions.h"
#include "AfterglowStructLayout.h"

class AfterglowSSBOInfo {
public:
	AfterglowSSBOInfo();
	AfterglowSSBOInfo(
		const std::string& name, 
		shader::Stage stage, 
		compute::SSBOUsage usage, 
		compute::SSBOAccessMode accessMode, 
		compute::SSBOTextureMode textureMode, 
		compute::SSBOTextureDimension textureDimension, 
		compute::SSBOTextureSampleMode textureSampleMode, 
		compute::SSBOInitMode initMode, 
		const std::string& initResource, 
		const AfterglowStructLayout& elementLayout, 
		uint64_t numElements
	);

	// TODO: Copy and move.

	const std::string& name() const noexcept;
	void rename(const std::string& name);

	shader::Stage stage() const noexcept;
	void setStage(shader::Stage stage) noexcept;

	compute::SSBOUsage usage() const noexcept;
	void setUsage(compute::SSBOUsage usage) noexcept;

	compute::SSBOAccessMode accessMode() const noexcept;
	void setAccessMode(compute::SSBOAccessMode accessMode) noexcept;

	// [Optional] If the imageFormat is enabled,  .rgba is used to read the buffer(like a texture) directly.
	// Also, custom element layout will not effect anymore.
	compute::SSBOTextureMode textureMode() const noexcept;
	void setTextureMode(compute::SSBOTextureMode textureMode) noexcept;

	compute::SSBOTextureDimension textureDimension() const noexcept;
	void setTextureDimension(compute::SSBOTextureDimension textureDimension) noexcept;

	compute::SSBOTextureSampleMode textureSampleMode() const noexcept;
	void setTextureSampleMode(compute::SSBOTextureSampleMode sampleMode) noexcept;

	compute::SSBOInitMode initMode() const noexcept;
	void setInitMode(compute::SSBOInitMode initMode) noexcept;

	const std::string& initResource() const noexcept;
	void setInitResource(const std::string& resourceName);

	// Total byte size == elementLayout.byteSize() * numElements;
	AfterglowStructLayout& elementLayout() noexcept;
	const AfterglowStructLayout& elementLayout() const noexcept;
	void setElementLayout(AfterglowStructLayout&& elementLayout) noexcept;

	uint64_t numElements() const noexcept;
	void setNumElements(uint64_t numElements) noexcept;

	bool isBuffer() const noexcept;
	bool isTexture1D() const noexcept;
	bool isTexture2D() const noexcept;
	bool isTexture3D() const noexcept;

	const std::string& hlslTemplateName() const;

	// Assume that indices were packed in unsigned int[4] per element (for memory alignment).
	uint64_t numUnpackedIndices() const noexcept;

private:
	void alignNumElements() noexcept;

	std::string _name;
	shader::Stage _stage = shader::Stage::Compute;
	compute::SSBOUsage _usage = compute::SSBOUsage::Buffer;
	compute::SSBOAccessMode _accessMode = compute::SSBOAccessMode::ReadWrite;
	compute::SSBOTextureMode _textureMode = compute::SSBOTextureMode::Unused;
	compute::SSBOTextureDimension _textureDimension = compute::SSBOTextureDimension::Texture2D;
	compute::SSBOTextureSampleMode _textureSampleMode = compute::SSBOTextureSampleMode::LinearRepeat;

	compute::SSBOInitMode _initMode = compute::SSBOInitMode::Zero;
	std::string _initResource;

	AfterglowStructLayout _elementLayout;
	uint64_t _numElements = 0;
};